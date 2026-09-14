/******************************************************************************/
/**
* @file        ComboDisplayManager.cs
* @project     Pulse Protocol
* @author      Carrie Lam (70%)
*              Chloe Lau (30%)
* @brief       Manages beat-synced reveal, staggered fade-out, and player input feedback
*              for combo key slot UI during enemy encounters. Updated to keep player
*              slots dimly visible during enemy turn instead of hiding them, with smooth fade
*              transitions between turn phases and per-slot question mark scale pulse support.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/

using System;
using System.Collections.Generic;
using ScriptAPI;

public class ComboDisplayManager : Script
{
    public static ComboDisplayManager? Instance { get; private set; }

    public bool DebugLogging = false;
    public float FadeOutDuration = 0.1f; // making this faster to counter INPUT NOW showing on MEMORIZE enemy last key 
    public float FadeStaggerDelay = 0.05f;

    private bool initialized = false;
    private int lastInputProgress = 0;

    private static Dictionary<int, ComboKeySlot> enemySlots = new Dictionary<int, ComboKeySlot>();
    private static Dictionary<int, ComboKeySlot> playerSlots = new Dictionary<int, ComboKeySlot>();
    private static ComboJudgementDisplay? judgementScript = null;

    private bool isRevealing = false;
    private string revealCombo = "";
    private int revealIndex = 0;
    private float revealStartTime = 0f;
    private float[] revealSlotOffsets = new float[] { 0.0f, 0.5f, 1.0f, 1.5f };
    public float RevealLeadTime = 0.1f; // Seconds to pop enemy keys BEFORE their beat (anticipatory cue)
    private int[] currentVisualSlots = new int[] { 0, 1, 2, 3 };

    // Maps key index → UI slot position (allows visual gaps for rhythm clarity)
    private int VisualSlot(int keyIndex)
    {
        if (keyIndex < currentVisualSlots.Length) return currentVisualSlots[keyIndex];
        return keyIndex;
    }

    private bool pendingCountdownStart = false;
    private int countdownCurrentSlot = 0;
    private bool isCountingDown = false;

    // time-based slot offset tracking
    private float countdownStartTime = 0f;
    private float[] currentSlotOffsets = new float[] { 0.0f, 0.5f, 1.0f, 1.5f };
    private const float ScaleAnimatedTime = 0.15f;

    private bool isFadingOut = false;
    private int fadeOutCount = 0;
    private float fadeStaggerTimer = 0f;

    // Gap slot reveals — pop the empty PNG on its beat during enemy reveal phase
    private bool isGapRevealing = false;
    private int[] gapVisualSlots = new int[0];
    private float[] gapRevealOffsets = new float[0];
    private int gapRevealIndex = 0;

    private string currentWeakness = "";
    private bool _inputPageFlipped = false;

    // =========================
    // Registration
    // =========================
    public static void RegisterKeySlot(int slotIndex, ComboKeySlot script)
    {
        if (script.GetIsEnemySlot())
        {
            enemySlots[slotIndex] = script;
            // Console.WriteLine($"[ComboDisplayManager] Registered EnemySlot {slotIndex}");
        }
        else
        {
            playerSlots[slotIndex] = script;
            // Console.WriteLine($"[ComboDisplayManager] Registered PlayerSlot {slotIndex}");
        }
    }

    public static void UnregisterKeySlot(int slotIndex, bool isEnemySlot)
    {
        if (isEnemySlot) { if (enemySlots.ContainsKey(slotIndex)) enemySlots.Remove(slotIndex); }
        else             { if (playerSlots.ContainsKey(slotIndex)) playerSlots.Remove(slotIndex); }
    }

    public static void RegisterJudgement(ComboJudgementDisplay script)
    {
        judgementScript = script;
        // Console.WriteLine("[ComboDisplayManager] Registered Judgement Display");
    }

    public static void UnregisterJudgement() { judgementScript = null; }

    // =========================
    // Update
    // =========================
    public override void Update()
    {
        if (!initialized) { Initialize(); return; }

        if (StageClearElement.IsCleared)
        {
            CancelStageClearVisuals();
            return;
        }

        if (isRevealing) { HandleBeatReveal(); }
        if (isGapRevealing) { HandleGapReveal(); }

        if (isFadingOut) HandleStaggerFade();

        if (pendingCountdownStart)
        {
            pendingCountdownStart = false;
            isCountingDown = true;
            countdownStartTime = Conductor.Instance != null ? Conductor.Instance.GetAdjustedSongTime() : 0f;
            currentSlotOffsets = EnemyRhythmController.Instance?.SlotOffsets ?? new float[] { 0.0f, 0.5f, 1.0f, 1.5f };

            if (playerSlots.ContainsKey(0) && currentWeakness.Length > 0)
                playerSlots[0].StartCountdown(currentWeakness[0]);
            countdownCurrentSlot = 1;
        }

        if (isCountingDown && Conductor.Instance != null) {
            float elapsed = Conductor.Instance.GetAdjustedSongTime() - countdownStartTime;

            int countdownCap = Math.Min(currentWeakness.Length, _inputPageFlipped ? currentWeakness.Length : 4);
            while (countdownCurrentSlot < countdownCap && countdownCurrentSlot < currentSlotOffsets.Length) {
                float targetTime = currentSlotOffsets[countdownCurrentSlot] - ScaleAnimatedTime;
                if (elapsed < targetTime) break;

                int slotIdx = _inputPageFlipped ? countdownCurrentSlot : countdownCurrentSlot;
                int alreadyPressed = ComboSystem.Instance != null ? ComboSystem.Instance.GetComboProgress() : 0;
                if (countdownCurrentSlot >= alreadyPressed && playerSlots.ContainsKey(VisualSlot(countdownCurrentSlot)))
                    playerSlots[VisualSlot(countdownCurrentSlot)].StartCountdown(currentWeakness[countdownCurrentSlot]);

                countdownCurrentSlot++;
            }

            if (countdownCurrentSlot >= countdownCap)
                isCountingDown = false;
        }

        // if (isCountingDown && Conductor.Instance != null)
        // {
        //     float songTime = Conductor.Instance.GetAdjustedSongTime();
        //     float beatDuration = Conductor.Instance.GetBeatDuration();
        //     float timeSinceBeat = (songTime % beatDuration + beatDuration) % beatDuration;
        //     float timeUntilNextBeat = beatDuration - timeSinceBeat;

        //     // Trigger next slot when we're ScaleAnimDuration away from the next beat
        //     if (timeUntilNextBeat <= 0.25f && countdownCurrentSlot < currentWeakness.Length)
        //     {
        //         int currentBeat = Conductor.Instance.GetCurrentBeat();
        //         if (currentBeat > lastCountdownBeat)
        //         {
        //             lastCountdownBeat = currentBeat;
        //             // show me blank placeholder
        //             // if player presses wrong flash them (with sprite)
             
        //             int alreadyPressed = ComboSystem.Instance != null ? ComboSystem.Instance.GetComboProgress() : 0;
        //             if (countdownCurrentSlot >= alreadyPressed && playerSlots.ContainsKey(countdownCurrentSlot))
        //                 playerSlots[countdownCurrentSlot].StartCountdown(currentWeakness[countdownCurrentSlot]);
        //             countdownCurrentSlot++;

        //             if (countdownCurrentSlot >= currentWeakness.Length)
        //                 isCountingDown = false;
        //         }
        //     }
        // }

        if (ComboSystem.Instance == null) return;

        // Track progress for page-flip logic below.
        // Sprite feedback is handled exclusively by ForceShowPlayerKeyFeedback (called from
        // InputBridge on the same frame as the key press), which has both key-correctness and
        // timing-window info. A secondary feedback block here ran one frame late and could
        // override the correct result, so it has been removed.
        int currentProgress = ComboSystem.Instance!.GetComboProgress();
        lastInputProgress = currentProgress;

        // Page-flip for 6/8-key combos: when player finishes page 1 (4 keys), show page 2 slots
        if (!_inputPageFlipped && currentWeakness.Length > 4 && currentProgress >= 4)
        {
            _inputPageFlipped = true;
            int page2Count = currentWeakness.Length - 4;

            // Show anticipation for page-2 keys
            for (int i = 4; i < currentWeakness.Length; i++)
            {
                int vs = VisualSlot(i); // wraps: 4→0, 5→1, 6→2, 7→3
                if (playerSlots.ContainsKey(vs))
                    playerSlots[vs].ShowBlankDim(currentWeakness[i]);
            }
            // Clear slots that have no page-2 key (e.g. slots 2,3 for a 6-key combo)
            for (int s = page2Count; s < 4; s++)
            {
                if (playerSlots.ContainsKey(s)) playerSlots[s].ShowEmpty();
            }

            // Restart countdown for page-2 slots, offset relative to page-2 start
            if (EnemyRhythmController.Instance?.SlotOffsets != null)
            {
                float[] allOffsets = EnemyRhythmController.Instance.SlotOffsets;
                float page2StartOffset = allOffsets[4];
                currentSlotOffsets = new float[page2Count];
                for (int i = 0; i < page2Count; i++)
                    currentSlotOffsets[i] = allOffsets[4 + i] - page2StartOffset;
            }
            countdownCurrentSlot = 4;
            countdownStartTime = Conductor.Instance != null ? Conductor.Instance.GetAdjustedSongTime() : 0f;
            isCountingDown = true;

            Console.WriteLine("[ComboDisplayManager] Page flip: showing page-2 keys");
        }
    }

    // =========================
    // Initialization
    // =========================
    private void Initialize()
    {
        Instance = this;
        lastInputProgress = 0;
        isRevealing = false;
        isFadingOut = false;
        currentWeakness = "";
        revealStartTime = 0f;
        revealSlotOffsets = new float[] { 0.0f, 0.5f, 1.0f, 1.5f };
        currentVisualSlots = new int[] { 0, 1, 2, 3 };

        pendingCountdownStart = false;
        countdownCurrentSlot = 0;
        isCountingDown = false;

        isGapRevealing = false;
        gapVisualSlots = new int[0];
        gapRevealOffsets = new float[0];
        gapRevealIndex = 0;

        enemySlots.Clear();
        playerSlots.Clear();
        judgementScript = null;

        Console.WriteLine("[ComboDisplayManager] Initialized");
        initialized = true;
    }

    // =========================
    // Beat Reveal (Enemy Row)
    // =========================
    public void StartComboReveal(string weakness)
    {
        revealCombo = weakness;
        currentWeakness = weakness;
        revealIndex = 0;
        isRevealing = true;
        isFadingOut = false;
        _inputPageFlipped = false;

        if (Conductor.Instance != null)
        {
            revealStartTime = Conductor.Instance.GetVisualAdjustedSongTime();
        }
        else
        {
            revealStartTime = 0f;
        }
        revealSlotOffsets = EnemyRhythmController.Instance?.SlotOffsets ?? new float[] { 0.0f, 0.5f, 1.0f, 1.5f };
        currentVisualSlots = EnemyRhythmController.Instance?.VisualSlotIndices ?? new int[] { 0, 1, 2, 3 };

        // Build gap slot reveal list: slots 0-3 not in currentVisualSlots, offset = slot * beatDur
        float bd = Conductor.Instance != null ? Conductor.Instance.SecondsPerBeat : 0.5f;
        var gapSlotList = new System.Collections.Generic.List<(int slot, float offset)>();
        for (int s = 0; s < 4; s++)
        {
            bool isUsed = false;
            for (int j = 0; j < currentVisualSlots.Length; j++)
                if (currentVisualSlots[j] == s) { isUsed = true; break; }
            if (!isUsed) gapSlotList.Add((s, s * bd));
        }
        gapSlotList.Sort((a, b) => a.offset.CompareTo(b.offset));
        gapVisualSlots  = new int[gapSlotList.Count];
        gapRevealOffsets = new float[gapSlotList.Count];
        for (int i = 0; i < gapSlotList.Count; i++)
        {
            gapVisualSlots[i]   = gapSlotList[i].slot;
            gapRevealOffsets[i] = gapSlotList[i].offset;
        }
        gapRevealIndex  = 0;
        isGapRevealing  = gapSlotList.Count > 0;

        ClearEnemySlotsInstant();
        float beatDur = Conductor.Instance != null ? Conductor.Instance.SecondsPerBeat : 0.5f;

        // Show gap player slots empty immediately — they stay visible the whole enemy turn,
        // no hide/pop-in. Only the enemy gap slots pop in on their beat.
        for (int i = 0; i < 4; i++)
        {
            if (!playerSlots.ContainsKey(i)) continue;
            bool isUsed = false;
            for (int j = 0; j < weakness.Length && j < 4; j++)
                if (VisualSlot(j) == i) { isUsed = true; break; }
            if (!isUsed) playerSlots[i].ShowEmpty();
        }

        for (int i = 0; i < weakness.Length && i < 4; i++)
        {
            int vs = VisualSlot(i);
            if (playerSlots.ContainsKey(vs))
                playerSlots[vs].ShowBlankFadeToDim(weakness[i], beatDur);
        }

        Console.WriteLine($"[ComboDisplayManager] Starting beat reveal for: {weakness}");
    }

    public void StopComboReveal()
    {
        isRevealing = false;
        lastInputProgress = 0;

        if (revealIndex > 0)
        {
            isFadingOut = true;
            fadeOutCount = 0;
            fadeStaggerTimer = 0f;
        }

        // countdown is now started early via StartPlayerCountdownEarly()
        pendingCountdownStart = false;
        countdownCurrentSlot = 0;
        isCountingDown = false;
    }

    private void HandleBeatReveal()
    {
        if (Conductor.Instance == null) return;

        // Use visual time (same clock as VisualBeatSystem) so keys stay in sync with border pulse
        float elapsed = Conductor.Instance.GetVisualAdjustedSongTime() - revealStartTime;

        while (revealIndex < revealCombo.Length && revealIndex < revealSlotOffsets.Length)
        {
            if (elapsed < revealSlotOffsets[revealIndex] - RevealLeadTime) break;

            char key = revealCombo[revealIndex];
            int vs = VisualSlot(revealIndex);
            if (enemySlots.ContainsKey(vs))
            {
                enemySlots[vs].ShowKeyPop(key);
                Console.WriteLine($"[ComboDisplayManager] Reveal at t={elapsed:F3}s: enemy slot {vs} (key {revealIndex}) = '{key}'");
            }
            revealIndex++;
        }

        if (revealIndex >= revealCombo.Length)
        {
            isRevealing = false;

            // For combos > 4 where page 2 doesn't fill all 4 slots:
            // slots that had page-1 keys but no page-2 key still show the old key just clear them.
            if (revealCombo.Length > 4 && revealCombo.Length % 4 != 0)
            {
                int page2Count = revealCombo.Length % 4;
                for (int s = page2Count; s < 4; s++)
                {
                    if (enemySlots.ContainsKey(s)) enemySlots[s].ShowEmpty();
                }
            }

            Console.WriteLine("[ComboDisplayManager] Beat reveal complete");
        }
    }

    private void HandleGapReveal()
    {
        if (Conductor.Instance == null) return;
        float elapsed = Conductor.Instance.GetVisualAdjustedSongTime() - revealStartTime;

        while (gapRevealIndex < gapVisualSlots.Length)
        {
            if (elapsed < gapRevealOffsets[gapRevealIndex] - RevealLeadTime) break;
            int s = gapVisualSlots[gapRevealIndex];
            if (enemySlots.ContainsKey(s)) enemySlots[s].ShowEmptyPop();
            // Player gap slots are already showing empty from StartComboReveal — no pop needed
            gapRevealIndex++;
        }

        if (gapRevealIndex >= gapVisualSlots.Length)
            isGapRevealing = false;
    }

    // =========================
    // Stagger Fade (Enemy Row)
    // =========================
    private void HandleStaggerFade()
    {
        if (fadeOutCount >= 4)
        {
            isFadingOut = false;
            Console.WriteLine("[ComboDisplayManager] Stagger fade complete");
            return;
        }

        fadeStaggerTimer += Time.DeltaTime;

        if (fadeStaggerTimer >= FadeStaggerDelay)
        {
            fadeStaggerTimer = 0f;
            // Fade slot by index directly so gap slots (empty key) also disappear
            if (enemySlots.ContainsKey(fadeOutCount))
            {
                enemySlots[fadeOutCount].FadeOutAndHide(FadeOutDuration);
                Console.WriteLine($"[ComboDisplayManager] Fading out enemy slot {fadeOutCount}");
            }
            fadeOutCount++;
        }
    }

    // =========================
    // Display Control
    // =========================
    public void ShowJudgement(string judgement)
    {
        if (judgementScript != null)
            judgementScript.ShowJudgement(judgement);
        else
            Console.WriteLine("[ComboDisplayManager] ⚠️ Judgement display not registered!");
    }

    // Called from InputBridge for every key press so slots always show feedback,
    // even for presses that ComboSystem rejected (e.g. after an early wrong key).
    public void ForceShowPlayerKeyFeedback(int pressIndex, char pressedKey, bool isCorrect, char expectedKey)
    {
        int vs = VisualSlot(pressIndex);
        if (!playerSlots.ContainsKey(vs)) return;
        if (isCorrect)
            playerSlots[vs].ShowCorrect(pressedKey);
        else
        {
            playerSlots[vs].ShowWrong(pressedKey, expectedKey);
            VFXAPI.ShakeCamera(0.01f, 5.0f);
        }
    }

    public void ShowGapMissKey(char key)
    {
        if (gapVisualSlots.Length == 0) return;
        if (TurnManager.Instance == null || Conductor.Instance == null) return;

        float currentTime = Conductor.Instance.GetVisualAdjustedSongTime();
        float playerStart = TurnManager.Instance.GetPlayerInputStartTime();
        float beatDur = Conductor.Instance.SecondsPerBeat;
        float window = ComboSystem.Instance != null ? ComboSystem.Instance.greatWindow : 0.15f;

        for (int i = 0; i < gapVisualSlots.Length; i++)
        {
            float expectedTime = playerStart + gapVisualSlots[i] * beatDur;
            if (Math.Abs(currentTime - expectedTime) <= window)
            {
                int s = gapVisualSlots[i];
                if (playerSlots.ContainsKey(s))
                    playerSlots[s].ShowWrong(key, key);
                return;
            }
        }
    }

    public void HideAllSlots()
    {
        lastInputProgress = ComboSystem.Instance?.GetComboProgress() ?? lastInputProgress;
        ClearEnemySlotsInstant();
        ClearPlayerSlots();
    }

    // Called when enemy death animation starts.
    // Hides the enemy row, stops all ongoing animations, and leaves player slots
    // in the dimmed state (same as during enemy turn) so the UI doesn't go blank.
    public void DimSlotsForDeath()
    {
        isRevealing = false;
        isGapRevealing = false;
        isFadingOut = false;
        isCountingDown = false;
        pendingCountdownStart = false;
        lastInputProgress = ComboSystem.Instance?.GetComboProgress() ?? lastInputProgress;

        ClearEnemySlotsInstant();
        ShowDimmedPlayerSlots(currentWeakness);
    }

    private void ClearPlayerSlots()
    {
        if (DebugLogging) Console.WriteLine("[ComboDisplayManager] Clearing player slots");
        for (int i = 0; i < 4; i++)
        {
            if (playerSlots.ContainsKey(i))
                playerSlots[i].HideInstant();
        }
    }

    private void ShowDimmedPlayerSlots(string weakness)
    {
        for (int i = 0; i < weakness.Length && i < 4; i++)
        {
            int vs = VisualSlot(i);
            if (playerSlots.ContainsKey(vs))
                playerSlots[vs].ShowBlankDim(weakness[i]);
        }
    }

    private void ClearEnemySlotsInstant()
    {
        for (int i = 0; i < 4; i++)
        {
            if (enemySlots.ContainsKey(i))
                enemySlots[i].HideInstant();
        }
    }

    // =========================
    // Helper Funcs
    // =========================
    public static void TriggerPlayerSlotPulse(int slotIndex, float duration)
    {
        if (playerSlots.ContainsKey(slotIndex))
            playerSlots[slotIndex].TriggerScalePulse(duration);
    }

    public static bool IsSlotRegistered(int slotIndex, ComboKeySlot slot)
    {
        if (slot.GetIsEnemySlot())
            return enemySlots.ContainsKey(slotIndex) && enemySlots[slotIndex] == slot;
        else
            return playerSlots.ContainsKey(slotIndex) && playerSlots[slotIndex] == slot;
    }

    public static bool IsJudgementRegistered(ComboJudgementDisplay script) => judgementScript == script;

    public void OnDestroy()
    {
        if (Instance == this)
        {
            Instance = null;
            enemySlots.Clear();
            playerSlots.Clear();
            judgementScript = null;
        }
    }

    public void StartPlayerCountdownEarly()
    {
        if (currentWeakness.Length == 0) return;

        currentSlotOffsets = EnemyRhythmController.Instance?.SlotOffsets ?? new float[] { 0.0f, 0.5f, 1.0f, 1.5f };
        countdownStartTime = Conductor.Instance != null ? Conductor.Instance.GetAdjustedSongTime() : 0f;

        // Show empty PNG on gap player slots; active slots get ShowBlankDim
        for (int i = 0; i < 4; i++)
        {
            if (!playerSlots.ContainsKey(i)) continue;
            bool isUsed = false;
            for (int j = 0; j < currentWeakness.Length && j < 4; j++)
                if (VisualSlot(j) == i) { isUsed = true; break; }
            if (!isUsed) playerSlots[i].ShowEmpty();
        }

        for (int i = 0; i < Math.Min(currentWeakness.Length, 4); i++)
        {
            int vs = VisualSlot(i);
            if (playerSlots.ContainsKey(vs))
                playerSlots[vs].ShowBlankDim(currentWeakness[i]);
        }

        countdownCurrentSlot = Math.Min(currentWeakness.Length, 4);
        isCountingDown = false;
        pendingCountdownStart = false;
        lastInputProgress = 0;
        
        Console.WriteLine("[ComboDisplayManager] All player slots revealed simultaneously");
    }

    public void FadeOutEnemySlots()
    {
        lastInputProgress = 0;
        if (revealIndex > 0)
        {
            isFadingOut = true;
            fadeOutCount = 0;
            fadeStaggerTimer = 0f;
        }
    }

    private void CancelStageClearVisuals()
    {
        isRevealing = false;
        isGapRevealing = false;
        isFadingOut = false;
        isCountingDown = false;
        pendingCountdownStart = false;
        revealIndex = 0;
        gapRevealIndex = 0;
        fadeOutCount = 0;
        countdownCurrentSlot = 0;
        lastInputProgress = 0;

        ClearEnemySlotsInstant();
    }
}
