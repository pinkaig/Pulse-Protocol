/******************************************************************************/
/**
* @file        ComboSystem.cs
* @project     Pulse Protocol
* @author      Carrie Lam (70%)
*              Chloe Lau (30%)
* @brief       Validates player key input against the enemy weakness combo, 
*              handling beat-timing judgement (PERFECT/GREAT/MISS) and one-key-per-beat
*              gating.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/

using System;
using System.Collections.Generic;
using ScriptAPI;

public class ComboSystem : Script
{
    // =========================
    // Global Access (Singleton)
    // =========================
    public static ComboSystem? Instance { get; private set; }

    // =========================
    // Config
    // =========================
    public int maxComboLength = 4;

    // Timing windows (seconds): Distance to the nearest beat boundary.
    // At 120 BPM (beatDuration = 0.5s), max possible delta is 0.25s.
    // Setting greatWindow < 0.25s creates a genuine MISS dead-zone in the middle of each beat.
    // PERFECT ≤ 0.06s (~3-4 frames @ 60fps)
    // GREAT   ≤ 0.125s (~7-8 frames @ 60fps)
    // MISS    >  0.125s  (the central 50% of each beat interval pressing mid-beat) :(
    public float perfectWindow = 0.08f;
    public float greatWindow   = 0.15f;
    // Shifts the entire timing window earlier by this many seconds.
    // Increase to make the window feel more responsive / require pressing slightly before the beat.
    public float InputEarlyShift = 0.17f;


    // DEBUG: Set to true to auto-set a test weakness and accept all input regardless of turn state
    public bool DebugTestMode = false;
    public string DebugTestWeakness = "WASD";

    // =========================
    // State
    // =========================
    private List<char> currentCombo = new List<char>();
    private List<string> hitResults = new List<string>();

    private int perfectStreak = 0;

    public bool PlayerDidSuccessfulCombo { get; private set; }
    private bool comboCompleted = false;  // Lock out further input after combo is evaluated

    // =========================
    // Beat Timing
    // =========================
    private int lastBeatIndex = -1;

    // Cache beat duration so we can measure distance to NEXT beat
    private float cachedBeatDuration = 0.5f; // fallback default (~120 BPM)

    // per slot time gate (replaced one key per beat gate)
    private float lastAcceptedPressTime = -999f;
    private float minTimeBetweenPresses = 0.25f;

    // private int lastInputBeat = -1;

    // =========================
    // Enemy Weakness (injected)
    // =========================
    private string currentEnemyWeakness = "";

    // =========================
    // Initialization tracking
    // =========================
    private bool initialized = false;

    // =========================
    // Update Loop
    // =========================
    public override void Update()
    {
        // Console.WriteLine($"[CS:DEBUG] initialized={initialized}, Instance==this:{Instance == this}, weakness='{currentEnemyWeakness}'");

   
        if (Instance != null && Instance != this && !initialized)
        {
            string preservedWeakness = Instance.currentEnemyWeakness;
            Instance = this;
            if (!string.IsNullOrEmpty(preservedWeakness))
            {
                currentEnemyWeakness = preservedWeakness;
                maxComboLength = preservedWeakness.Length;
                Console.WriteLine($"[ComboSystem] Preserved weakness from old instance: {preservedWeakness}");
            }
        }

        if (Instance == null)
        {
            Instance = this;
            Console.WriteLine("[ComboSystem] Singleton instance set");
        }

        // First-time initialization
        if (!initialized)
        {
            Instance = this;
     
            initialized = true;
            Console.WriteLine("[ComboSystem] Initialized");
            return;
        }

        
        if (Instance != this)
            return;

        if (Conductor.Instance == null)
            return;


        cachedBeatDuration = Conductor.Instance!.GetBeatDuration();

        int currentBeat = Conductor.Instance!.GetCurrentBeat();
        if (currentBeat != lastBeatIndex)
            lastBeatIndex = currentBeat;
    }

    // =========================
    // Public API
    // =========================
    public void SetEnemyWeakness(string weaknessCombo)
    {
        currentEnemyWeakness = weaknessCombo;
        maxComboLength = weaknessCombo.Length;
        ResetCombo();

        // calc min time between presses from slotoffsets so fast patterns with 0.25 gaps are not blocked
        float[] offsets = EnemyRhythmController.Instance?.SlotOffsets ?? new float[] { 0.0f, 0.5f, 1.0f, 1.5f };
        if (offsets.Length >= 2)
        {
            float minGap = float.MaxValue;
            for (int i = 1; i < offsets.Length; i++) {
                float gap = offsets[i] - offsets[i - 1];
                if (gap < minGap) minGap = gap;
            }
            // Gate must never block two consecutive valid presses within their great windows.
            // Minimum valid time between press N (at worst: +greatWindow late) and press N+1
            // (at best: -greatWindow early) = minGap - 2*greatWindow.
            // Floor at 0.05s to prevent bounce-registration from a single physical keypress.
            minTimeBetweenPresses = Math.Max(0.05f, minGap - 2f * greatWindow);
        }
        else {
            minTimeBetweenPresses = cachedBeatDuration * 0.8f;
        }

        Console.WriteLine($"[ComboSystem] Weakness set to: {weaknessCombo} (length: {maxComboLength})");
    }

    public void RegisterInput(char key)
    {
        // Block input if combo already completed (waiting for TurnManager to reset)
        if (comboCompleted)
        {
            Console.WriteLine($"[ComboSystem] [{key}] IGNORED — combo already completed, waiting for reset");
            return;
        }

        if (string.IsNullOrEmpty(currentEnemyWeakness))
        {
            Console.WriteLine("[ComboSystem] ERROR: No weakness set! Cannot register input.");
            return;
        }

        float currentSongTime = Conductor.Instance!.GetVisualAdjustedSongTime();

        // Measure delta against this slot's specific expected press time.
        // The old "% beatDuration" only worked when all offsets were beat multiples.
        // Syncopated patterns like Bat (0.25s offsets) fall between beats and always
        // gave MISS with that approach — this fixes it.
        float[] slotOffsets = EnemyRhythmController.Instance?.SlotOffsets ?? new float[] { 0f, 0.5f, 1f, 1.5f };
        int slotIndex = currentCombo.Count;
        float delta;
        if (TurnManager.Instance != null && slotIndex < slotOffsets.Length)
        {
            float expectedTime = TurnManager.Instance.GetPlayerInputStartTime() + slotOffsets[slotIndex] - InputEarlyShift;
            delta = Math.Abs(currentSongTime - expectedTime);
        }
        else
        {
            // Fallback: nearest beat boundary
            float beatDuration = Conductor.Instance!.GetBeatDuration();
            float timeSince = (currentSongTime % beatDuration + beatDuration) % beatDuration;
            delta = Math.Min(timeSince, beatDuration - timeSince);
        }

        // Clamp to zero in case of floating-point drift
        if (delta < 0f) delta = 0f;

        // =========================
        // One-key-per-beat gate
        // =========================
        float timeSinceLastPress = currentSongTime - lastAcceptedPressTime;
        if (timeSinceLastPress < minTimeBetweenPresses)
        {
            Console.WriteLine($"[ComboSystem] [{key}] IGNORED - too soon after last press ({timeSinceLastPress:F3}s < {minTimeBetweenPresses:F3}s)");
            return;
        }

        // =========================
        // Judgement
        // =========================
        string judgement;
        if (delta <= perfectWindow)
            judgement = "PERFECT";
        else if (delta <= greatWindow)
            judgement = "GREAT";
        else
            judgement = "MISS";

        // =========================
        // MISS rejection
        // =========================
        
        if (judgement == "MISS")
        {
            Console.WriteLine($"[ComboSystem] [{key}] MISS — pressed too far from beat (Δ={delta:F3}s). Combo failed.");
            ScoreManager.Instance?.AddScore("MISS");
            if (ComboDisplayManager.Instance != null)
            {
                ComboDisplayManager.Instance.ShowJudgement("MISS");
                ComboDisplayManager.Instance.ShowGapMissKey(key);
            }
            VisualBeatSystem.Instance?.FlashJudgement("MISS");
            PlayerDidSuccessfulCombo = false;
            comboCompleted = true;
            return;
        }

        lastAcceptedPressTime = currentSongTime;
        hitResults.Add(judgement);
        currentCombo.Add(key);

        // === WRONG KEY CHECK ===
        // Wrong key = immediate combo fail, no matter the timing
        string visualJudgement = judgement;
        if (currentCombo.Count <= currentEnemyWeakness.Length)
        {
            char expectedKey = currentEnemyWeakness[currentCombo.Count - 1];
            if (key != expectedKey)
            {
                visualJudgement = "MISS";
                Console.WriteLine($"[ComboSystem] [{key}] MISS — wrong key (expected '{expectedKey}'). Combo failed.");
                ScoreManager.Instance?.AddScore("MISS");
                if (ComboDisplayManager.Instance != null)
                {
                    ComboDisplayManager.Instance.ShowJudgement("MISS");
                }
                VisualBeatSystem.Instance?.FlashJudgement("MISS");
                PlayerDidSuccessfulCombo = false;
                comboCompleted = true;
                return;
            }
        }

        ScoreManager.Instance?.AddScore(visualJudgement);

        Console.WriteLine($"[ComboSystem] [{key}] Timing: {judgement} (Δ={delta:F3}s from slot {slotIndex} expected time)");
        Console.WriteLine($"[ComboSystem]   Progress: {currentCombo.Count}/{maxComboLength} | Expected: {(currentCombo.Count <= currentEnemyWeakness.Length ? currentEnemyWeakness[currentCombo.Count - 1].ToString() : "?")} | Got: {key}");


        if (ComboDisplayManager.Instance != null)
        {
            ComboDisplayManager.Instance.ShowJudgement(visualJudgement);
        }
        else
        {
            Console.WriteLine("[ComboSystem] WARNING: ComboDisplayManager not found!");
        }

        VisualBeatSystem.Instance?.FlashJudgement(visualJudgement);

        // === PERFECT STREAK ===
        if (judgement == "PERFECT")
        {
            perfectStreak++;
            if (perfectStreak >= 5)
            {
                Console.WriteLine("[ComboSystem] 🔥 PERFECT STREAK x5!");
                perfectStreak = 0;
            }
        }
        else
        {
            perfectStreak = 0;
        }

        if (currentCombo.Count >= maxComboLength)
            EvaluateCombo();
    }

    public bool HasCompletedCombo()
    {
        return comboCompleted;
    }

    public void ResetCombo()
    {
        PlayerDidSuccessfulCombo = false;
        currentCombo.Clear();
        hitResults.Clear();
        comboCompleted = false;
        lastAcceptedPressTime = -999f;
    }

    // =========================
    // Evaluation
    // =========================
    private void EvaluateCombo()
    {
        string comboString = new string(currentCombo.ToArray());
        bool sequenceCorrect = (comboString == currentEnemyWeakness);
        bool timingOk = true;

        foreach (string r in hitResults)
        {
            if (r == "MISS")
            {
                timingOk = false;
                break;
            }
        }

        Console.WriteLine($"[ComboSystem] Entered: {comboString} | Expected: {currentEnemyWeakness}");

        if (sequenceCorrect && timingOk)
        {
            Console.WriteLine("[ComboSystem] ✅ COMBO SUCCESS!");
            PlayerDidSuccessfulCombo = true;
            GetInput().RumbleGamepad(1.0f, 1.0f, 0.3f);
        }
        else
        {
            Console.WriteLine("[ComboSystem] ❌ COMBO FAILED!");
            Console.WriteLine($"[ComboSystem] Sequence match: {sequenceCorrect}, Timing OK: {timingOk}");
            PlayerDidSuccessfulCombo = false;
        }

        // Lock out further input until TurnManager calls ResetCombo()
        comboCompleted = true;

        // In debug test mode, auto-reset so we can test again immediately
        if (DebugTestMode)
        {
            Console.WriteLine("[ComboSystem] [DEBUG] Auto-resetting combo for next test...");
            ResetCombo();
        }
    }

    // =========================
    // Debug / Utility
    // =========================
    public string GetCurrentComboString()
    {
        return new string(currentCombo.ToArray());
    }

    public int GetComboProgress()
    {
        return currentCombo.Count;
    }

    public string GetExpectedCombo()
    {
        return currentEnemyWeakness;
    }


    public void OnDestroy()
    {
        if (Instance == this)
            Instance = null;
    }
}