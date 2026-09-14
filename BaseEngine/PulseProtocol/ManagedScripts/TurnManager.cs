/******************************************************************************/
/**
* @file        TurnManager.cs
* @project     Pulse Protocol
* @author      Carrie Lam (70%)
*              Chloe Lau (30%)
* @brief       Controls the beat-driven turn loop between enemy and player phases
*              (Countdown, EnemyDisplay, PlayerInput, Resolution), coordinating
*              phase transitions and combo resolution each session. Extended to
*              drive beat-synced outline flash and scale pulses, with a large
*              simultaneous pop on the prep beat and per-slot staggered pulses
*              during player input beats.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/

using System;
using ScriptAPI;

public enum TurnPhase
{
    Countdown,      // First 4 beats - enemy idle
    EnemyDisplay,   // Next 4 beats - show combo, player locked
    PlayerInput,    // 5 effective beats - player inputs combo
    Resolution      // Resolve success/failure
}

public class TurnManager : Script
{
    // =========================
    // Global Access (Singleton)
    // =========================
    public static TurnManager? Instance { get; private set; }

    // =========================
    // Config
    // =========================
    public int CountdownBeats = 4;
    public int EnemyDisplayBeats = 4;  // with -1 backdating: 3 beats shown in switch + initial beat N = 4 enemy beats total. Transition fires on beat 4, player slot 0 fires on beat 5 (no gap).
    public int PlayerInputBeats = 5;   // with -1 backdating: beats 2-5 flash slots 0-3, beat 5 also resolves.
    public float PulseLeadTime = 0.17f; // Seconds to fire the slot pulse BEFORE the beat, so it acts as an anticipatory cue

    // Boss overrides: set by BossEnemyController to extend phases for longer combos. Cleared after each turn.
    public static int? EnemyDisplayBeatsOverride = null;
    public static int? PlayerInputBeatsOverride  = null;

    public string EnemyTurnSFX = "SFX_Enemy_Ping";
    public string PlayerTurnSFX = "SFX_Player_Ping";

    // =========================
    // State
    // =========================
    private TurnPhase currentPhase = TurnPhase.Countdown;
    private int phaseStartBeat = -1;
    private int lastBeat = -1;
    private bool initialized = false;
    private bool gameStarted = false;
    private bool prevGameReady = false; // used to detect new play sessions on reused script objects
    private int resolutionPendingFrames = 0; // counts down: 2→1→0 then resolves, giving InputBridge 2 frames to process last-beat input regardless of entity update order
    private int comboAtLastSuccess = -1;     // saved after a successful kill; restored at the start of the next turn to survive the bar-alignment wait

    // Time-based player slot pulses (mirrors SlotOffsets, runs every frame)
    private bool playerInputPhaseActive = false;
    private float playerInputStartTime = 0f;
    private bool[] slotPulsed = new bool[4];
    private bool[] gapSlotPulsed = new bool[4];

    public bool IsPlayerTurn => currentPhase == TurnPhase.PlayerInput;
    public bool IsInputLocked => currentPhase != TurnPhase.PlayerInput;

    // =========================
    // Update Loop
    // =========================
    public override void Update()
    {
        if (StageClearElement.IsCleared)
        {
            playerInputPhaseActive = false;
            resolutionPendingFrames = 0;
            return;
        }

        // Session restart detection for reused script objects (engine does not always
        // recreate C# Script instances on Stop→Play). GameStart.Ready flips from true
        // (end of last session) to false (LevelCountdown resets it at start of new session).
        // That flip means a new play session has begun and we must re-initialize.
        if (initialized && prevGameReady && !GameStart.Ready)
        {
            Console.WriteLine("[TurnManager] New play session detected — resetting state");
            initialized = false;
        }

        prevGameReady = GameStart.Ready;

        if (!initialized)
        {
            Initialize();
            return;
        }

        if (Conductor.Instance == null)
            return;

        int currentBeat = Conductor.Instance!.GetCurrentBeat();

        // Deferred resolution: counts down 2 frames before resolving so InputBridge
        // always gets at least one full frame to process last-beat input regardless
        // of which entity updates first.
        if (resolutionPendingFrames > 0)
        {
            resolutionPendingFrames--;
            if (resolutionPendingFrames == 0)
            {
                ResolvePlayerInput();
            }
            return;
        }

        bool killingBlowDealt = ComboSystem.Instance?.HasCompletedCombo() == true
            && (EnemyRhythmController.Instance?.IsPendingDeath == true
                || EnemyRhythmController.Instance?.IsDying == true);

        // Immediately resolve when killing blow is confirmed — don't wait for remaining player beats.
        // This gets us into Resolution phase (input locked, slots dimmed) without delay.
        if (killingBlowDealt && currentPhase == TurnPhase.PlayerInput && resolutionPendingFrames == 0)
            resolutionPendingFrames = 2;

        if (playerInputPhaseActive && GameStart.Ready
            && currentPhase == TurnPhase.PlayerInput
            && !killingBlowDealt)
        {
            float elapsed = Conductor.Instance.GetVisualAdjustedSongTime() - playerInputStartTime;
            float[] offsets = EnemyRhythmController.Instance?.SlotOffsets ?? new float[] { 0.0f, 0.5f, 1.0f, 1.5f };
            int[] visualSlots = EnemyRhythmController.Instance?.VisualSlotIndices ?? new int[] { 0, 1, 2, 3 };
            float beatDur = Conductor.Instance.SecondsPerBeat;
            for (int i = 0; i < slotPulsed.Length && i < offsets.Length; i++)
            {
                if (!slotPulsed[i] && elapsed >= offsets[i] - PulseLeadTime)
                {
                    slotPulsed[i] = true;
                    int vs = (visualSlots != null && i < visualSlots.Length) ? visualSlots[i] : i;
                    PlayerSlotBeatPulse.TriggerFlash(vs, beatDur);
                    PlayerSlotBeatPulse.TriggerScale(vs, beatDur);
                    ComboDisplayManager.TriggerPlayerSlotPulse(vs, beatDur);
                    if (i == 0)
                        GetAudio().PlaySFX(PlayerTurnSFX);

                }
            }

            // Gap slots: scale pulse only (no yellow flash) at slot_index * beatDur
            for (int s = 0; s < 4; s++)
            {
                if (gapSlotPulsed[s]) continue;
                bool isActive = false;
                if (visualSlots != null)
                    for (int j = 0; j < visualSlots.Length; j++)
                        if (visualSlots[j] == s) { isActive = true; break; }
                if (isActive) continue;
                if (elapsed >= s * beatDur - PulseLeadTime)
                {
                    gapSlotPulsed[s] = true;
                    PlayerSlotBeatPulse.TriggerScale(s, beatDur);
                    ComboDisplayManager.TriggerPlayerSlotPulse(s, beatDur);
                }
            }
        }

        // Only process on new beats
        if (currentBeat == lastBeat)
            return;

        if (!GameStart.Ready) return;

        // Anchor the phase start only once FMOD is confirmed driving the beat clock.
        // LevelBGM runs after TurnManager in entity order, so on the GameStart.Ready frame
        // TurnManager would otherwise anchor against a stale pre-BGM DT beat. Waiting for
        // IsFMODSynced ensures the clock has been reset AND FMOD is actually reporting
        // position before we lock in phaseStartBeat.
        if (!gameStarted)
        {
            if (!Conductor.Instance.IsFMODSynced) return;
            gameStarted = true;
            lastBeat = currentBeat;
            StartEnemyDisplay();
            return;
        }

        lastBeat = currentBeat;
        int beatsInPhase = currentBeat - phaseStartBeat;

        // Phase transitions
        switch (currentPhase)
        {
            case TurnPhase.Countdown:
                if (beatsInPhase >= CountdownBeats)
                {
                    StartEnemyDisplay();
                    beatsInPhase = currentBeat - phaseStartBeat;
                    goto case TurnPhase.EnemyDisplay;
                }
                break;

            case TurnPhase.EnemyDisplay:
                if (beatsInPhase >= (EnemyDisplayBeatsOverride ?? EnemyDisplayBeats))
                {
                    ComboDisplayManager.Instance?.StartPlayerCountdownEarly();
                    StartPlayerInput(); // unlock input on the same beat
                    beatsInPhase = currentBeat - phaseStartBeat;
                    goto case TurnPhase.PlayerInput;
                }
                else
                {
                    Console.WriteLine($"[TurnManager] Enemy Display Beat {beatsInPhase + 1}/{EnemyDisplayBeats}");
                }
                break;

            case TurnPhase.PlayerInput:
                {
                    if (beatsInPhase == 1)
                        TurnAnnouncerUI.Instance?.ShowGo();

                    if (beatsInPhase >= (PlayerInputBeatsOverride ?? PlayerInputBeats))
                    {
                        // Defer one frame so InputBridge can process any key on this beat first.
                        resolutionPendingFrames = 2;
                    }
                    else
                    {
                        Console.WriteLine($"[TurnManager] Player Input Beat {beatsInPhase + 1}/{PlayerInputBeatsOverride ?? PlayerInputBeats}");
                    }
                    break;
                }

            case TurnPhase.Resolution:
                if (EnemyRhythmController.Instance == null
                    || EnemyRhythmController.Instance!.IsDying
                    || EnemyRhythmController.Instance!.IsPendingDeath)
                {
                    Console.WriteLine("[TurnManager] Resolution: waiting for death animation / next enemy...");
                    break;
                }
                // Wait for bar alignment so every enemy's phase always starts on beat 1.
                if (currentBeat % CountdownBeats != 0)
                {
                    Console.WriteLine($"[TurnManager] Resolution: waiting for bar alignment (beat {currentBeat}, need multiple of {CountdownBeats})...");
                    break;
                }
                StartEnemyDisplay();
                beatsInPhase = currentBeat - phaseStartBeat;
                goto case TurnPhase.EnemyDisplay;
        }
    }

    // =========================
    // Initialization
    // =========================
    private void Initialize()
    {
        // Set singleton (handle scene reloads)
        if (Instance != null && Instance != this)
        {
            Console.WriteLine("[TurnManager] Replacing stale singleton instance");
        }
        Instance = this;

        // Full state reset: GameStart.Ready is a static bool that persists between
        // play sessions. If it's stale-true, TurnManager would skip the countdown.
        // Resetting it here forces LevelCountdown to re-run its 3-2-1-GO sequence.
        GameStart.Ready = false;
        currentPhase = TurnPhase.Countdown;
        phaseStartBeat = -1;
        lastBeat = -1;
        gameStarted = false;
        playerInputPhaseActive = false;
        resolutionPendingFrames = 0;
        comboAtLastSuccess = -1;
        EnemyDisplayBeatsOverride = null;
        PlayerInputBeatsOverride  = null;
        for (int i = 0; i < slotPulsed.Length; i++) slotPulsed[i] = false;

        // Clear ComboSystem state so lastInputBeat (from a previous session) can't
        // accidentally match a beat in the new session and block the first key press.
        ComboSystem.Instance?.ResetCombo();

        // Reset static slot detection so new instances can re-register on level reload.
        PlayerSlotBeatPulse.ResetDetection();

        Console.WriteLine("[TurnManager] Initializing...");

        if (Conductor.Instance != null)
        {
            phaseStartBeat = Conductor.Instance!.GetCurrentBeat();
            lastBeat = phaseStartBeat;
        }

        Console.WriteLine("[TurnManager] Starting countdown phase (4 beats)");

        initialized = true;
    }

    // =========================
    // Phase Transitions
    // =========================
    private void StartEnemyDisplay()
    {
        currentPhase = TurnPhase.EnemyDisplay;
        phaseStartBeat = Conductor.Instance!.GetCurrentBeat() - 1;

        if (comboAtLastSuccess >= 0)
        {
            ScoreManager.Instance?.RestoreCombo(comboAtLastSuccess);
            comboAtLastSuccess = -1;
        }

        // Return player to idle at the start of every enemy turn.
        PlayerAnimationController.Instance?.ForceIdle();

        Console.WriteLine("[TurnManager] === ENEMY TURN ===");
        Console.WriteLine($"[TurnManager] Displaying combo: {EnemyRhythmController.Instance?.WeaknessCombo ?? "UNKNOWN"}");
        Console.WriteLine("[TurnManager] Player input LOCKED");

        GetAudio().PlaySFX(EnemyTurnSFX);

        if (ComboSystem.Instance != null)
        {
            ComboSystem.Instance.ResetCombo();
        }

        // Reset enemy movement for new turn (may randomize WeaknessCombo)
        if (EnemyRhythmController.Instance != null)
        {
            EnemyRhythmController.Instance.ResetMovementForNewTurn();
        }

        // Re-sync weakness AFTER ResetMovementForNewTurn so ComboSystem always has the
        // current (possibly newly randomized) combo — fixes green+MISS on turn 2+.
        if (ComboSystem.Instance != null && EnemyRhythmController.Instance != null)
            ComboSystem.Instance.SetEnemyWeakness(EnemyRhythmController.Instance.WeaknessCombo);

        // BEAT REVEAL!
        if (ComboDisplayManager.Instance != null && EnemyRhythmController.Instance != null)
            ComboDisplayManager.Instance.StartComboReveal(EnemyRhythmController.Instance.WeaknessCombo);
    }

    private void StartPlayerInput()
    {
        currentPhase = TurnPhase.PlayerInput;
        phaseStartBeat = Conductor.Instance!.GetCurrentBeat() - 1;

        ComboSystem.Instance?.ResetCombo();
        resolutionPendingFrames = 0;

        Console.WriteLine("[TurnManager] === PLAYER TURN ===");
        Console.WriteLine($"[TurnManager] Input the combo: {EnemyRhythmController.Instance?.WeaknessCombo ?? "UNKNOWN"}");
        Console.WriteLine("[TurnManager] Player input UNLOCKED");
        {
            var dbgOffsets = EnemyRhythmController.Instance?.SlotOffsets;
            string dbgType = EnemyRhythmController.Instance?.GetType().Name ?? "null";
            string dbgOff  = dbgOffsets != null ? string.Join(", ", dbgOffsets) : "null";
            Console.WriteLine($"[TurnManager] SlotOffsets from {dbgType}: [{dbgOff}]");
        }

        // Enemy freezes during player input (handled in EnemyRhythmController.HandleBeat)

        // Just fade out enemy slots, don't stop the reveal
        if (ComboDisplayManager.Instance != null)
            ComboDisplayManager.Instance.FadeOutEnemySlots();

        // Snap playerInputStartTime to the next visual beat boundary — this is the beat
        // immediately after the enemy's last beat, so slot 0 fires with no gap.
        float beatDuration = Conductor.Instance!.SecondsPerBeat;
        float visualNow = Conductor.Instance!.GetVisualAdjustedSongTime();
        playerInputStartTime = (float)Math.Floor(visualNow / beatDuration) * beatDuration + beatDuration;
        playerInputPhaseActive = true;
        int slotCount = EnemyRhythmController.Instance?.SlotOffsets?.Length ?? 4;
        slotPulsed = new bool[slotCount];
        gapSlotPulsed = new bool[4];
    }

    private void ResolvePlayerInput()
    {
        currentPhase = TurnPhase.Resolution;
        playerInputPhaseActive = false;

        bool success = false;
        if (ComboSystem.Instance != null)
        {
            success = ComboSystem.Instance.PlayerDidSuccessfulCombo;
        }

        Console.WriteLine($"[TurnManager] === RESOLUTION ===");

        if (success)
        {
            // Damage was already applied immediately in InputBridge.OnKeyPressed()
            // when the combo was completed — no need to apply it again here.
            Console.WriteLine("[TurnManager] Player succeeded! (damage applied at combo completion)");

            comboAtLastSuccess = ScoreManager.Instance?.Combo ?? -1;
        }
        else
        {
            comboAtLastSuccess = -1;
            Console.WriteLine("[TurnManager] ? Player failed!");
            Console.WriteLine("[TurnManager] Enemy attacks and moves remaining steps!");

            // Only play break SFX here if InputBridge never played it already.
            // InputBridge plays it immediately when a wrong key / MISS fires (comboCompleted = true).
            // If comboCompleted is still false, the player pressed nothing or pressed partially
            // without triggering an early fail — play it here instead.
            if ((ComboSystem.Instance == null || !ComboSystem.Instance.HasCompletedCombo())
                && !(InputBridge.Instance?.BreakSfxPlayed ?? false))
            {
                AudioComponent audio = GetAudio();
                audio.PlaySFX("SFX_ComboStreakBroken");
            }

            // Enemy executes punishment
            if (EnemyRhythmController.Instance != null)
            {
                EnemyRhythmController.Instance.ExecuteFailurePunishment();
            }
        }

        // Penalize any keys the player never entered as MISS (no-input penalty)
        if (ComboSystem.Instance != null && ScoreManager.Instance != null)
        {
            int entered  = ComboSystem.Instance.GetComboProgress();
            int expected = ComboSystem.Instance.GetExpectedCombo().Length;
            for (int i = entered; i < expected; i++)
                ScoreManager.Instance.AddScore("MISS");
        }

        // Clear boss beat overrides so normal enemies are never affected next turn
        EnemyDisplayBeatsOverride = null;
        PlayerInputBeatsOverride  = null;

        // Reset for next turn
        if (ComboSystem.Instance != null)
        {
            ComboSystem.Instance.ResetCombo();
        }
    }

    // =========================
    // Public API
    // =========================
    public TurnPhase GetCurrentPhase()
    {
        return currentPhase;
    }

    // Returns the visual time at which slot 0 is expected to be pressed.
    // ComboSystem uses this to measure timing against each slot's specific beat
    // so syncopated patterns (e.g. Bat's 0.25s offsets) work correctly.
    public float GetPlayerInputStartTime() => playerInputStartTime;

    public int GetBeatsInCurrentPhase()
    {
        if (Conductor.Instance == null)
            return 0;

        return Conductor.Instance!.GetCurrentBeat() - phaseStartBeat;
    }

    public int GetBeatsRemainingInPhase()
    {
        int beatsInPhase = GetBeatsInCurrentPhase();

        switch (currentPhase)
        {
            case TurnPhase.Countdown:
                return CountdownBeats - beatsInPhase;
            case TurnPhase.EnemyDisplay:
                return EnemyDisplayBeats - beatsInPhase;
            case TurnPhase.PlayerInput:
                return PlayerInputBeats - beatsInPhase;
            default:
                return 0;
        }
    }

    public void OnDestroy()
    {
        if (Instance == this)
            Instance = null;
    }
}
