using System;
using ScriptAPI;

public class InputBridge : Script
{
    // =========================
    // Config
    // =========================
    public int KeyW = 87;
    public int KeyA = 65;
    public int KeyS = 83;
    public int KeyD = 68;

    public int DebugAllowedExtraKey = 75;
    public bool DebugTreatExtraKeyAsValid = true;
    public bool DebugBypassTurnLock = false;
    public bool DebugLogEveryFrame = false;

    // PS4 gamepad button constants (GLFW_GAMEPAD_BUTTON_*)
    // Triangle(Y=3)=W, Circle(X=2)=A, Cross(A=0)=S, Square(B=1)=D
    private const int GP_TRIANGLE = 3; // W
    private const int GP_CIRCLE = 2; // A
    private const int GP_SQUARE = 1; // D
    private const int GP_CROSS  = 0; // S


    // =========================
    // Global Access (Singleton)
    // =========================
    public static InputBridge? Instance { get; private set; }

    // =========================
    // State
    // =========================
    private bool initialized = false;
    private bool prevW = false;
    private bool prevA = false;
    private bool prevS = false;
    private bool prevD = false;
    private bool prevInvalidKeyDown = false;
    private bool breakSfxPlayed = false;
    private bool anyWrongKey = false;    // true if any key this turn was wrong; drives last-beat SFX
    private int nextSlot = 0;            // advances ONLY on player press
    private bool prevIsPlayerTurn = false;

    public int NextSlot => nextSlot;
    public bool BreakSfxPlayed => breakSfxPlayed;

    // =========================
    // Update Loop
    // =========================

    public override void Update()
    {
        if (!initialized) { Initialize(); return; }

        if (StageClearElement.IsCleared)
        {
            prevW = false;
            prevA = false;
            prevS = false;
            prevD = false;
            prevInvalidKeyDown = false;
            return;
        }

        // Reset per-turn state when player turn starts
        bool isPlayerTurn = TurnManager.Instance?.IsPlayerTurn ?? false;
        if (isPlayerTurn && !prevIsPlayerTurn)
        {
            nextSlot = 0;
            breakSfxPlayed = false;
            anyWrongKey = false;
            Console.WriteLine("[InputBridge] Per-turn reset fired");
        }
        prevIsPlayerTurn = isPlayerTurn;

        InputComponent input = GetInput();

        if (!DebugBypassTurnLock && Application.IsPaused()) return;

        if (!DebugBypassTurnLock)
        {
            if (TurnManager.Instance == null || TurnManager.Instance.IsInputLocked)
            {
                prevW = input.IsKeyPressed(KeyW);
                prevA = input.IsKeyPressed(KeyA);
                prevS = input.IsKeyPressed(KeyS);
                prevD = input.IsKeyPressed(KeyD);
                return;
            }
        }

        // Keyboard OR gamepad face buttons (additive — either input works)
        bool curW = input.IsKeyPressed(KeyW) || input.IsGamepadButtonPressed(GP_TRIANGLE);
        bool curA = input.IsKeyPressed(KeyA) || input.IsGamepadButtonPressed(GP_SQUARE);
        bool curS = input.IsKeyPressed(KeyS) || input.IsGamepadButtonPressed(GP_CROSS);
        bool curD = input.IsKeyPressed(KeyD) || input.IsGamepadButtonPressed(GP_CIRCLE);

        bool tapW = curW && !prevW;
        bool tapA = curA && !prevA;
        bool tapS = curS && !prevS;
        bool tapD = curD && !prevD;

        if (DebugLogEveryFrame && (tapW || tapA || tapS || tapD))
            Console.WriteLine($"[InputBridge:DEBUG] Frame={Time.FrameCount} Tap: W={tapW} A={tapA} S={tapS} D={tapD}");

        if (tapW) OnKeyPressed('W');
        if (tapA) OnKeyPressed('A');
        if (tapS) OnKeyPressed('S');
        if (tapD) OnKeyPressed('D');

        prevW = curW;
        prevA = curA;
        prevS = curS;
        prevD = curD;

        CheckInvalidKeys(input);
        CheckMissedTurn();
    }

    // =========================
    // Initialization
    // =========================
    private void Initialize()
    {
        Instance = this;
        Console.WriteLine("[InputBridge] ===== INITIALIZED =====");
        Console.WriteLine("[InputBridge] Using C# edge detection (IsKeyPressed + prev state)");
        Console.WriteLine("[InputBridge] TurnManager lock is ACTIVE");

        if (DebugBypassTurnLock)
            Console.WriteLine("[InputBridge] ⚠️ DEBUG MODE: TurnManager lock BYPASSED");
        if (DebugLogEveryFrame)
            Console.WriteLine("[InputBridge] ⚠️ DEBUG MODE: Logging raw input every frame");

        initialized = true;
    }

    // =========================
    // Input Handling
    // =========================
    private void OnKeyPressed(char key)
    {
        if (StageClearElement.IsCleared) return;
        if (ComboSystem.Instance == null) return;

        // Re-sync: if ComboSystem was reset (progress=0, not yet completed) but nextSlot
        // is still non-zero from a previous turn, the per-turn edge-detection reset was
        // missed. Correct it here so the first key of the new turn starts at slot 0.
        if (nextSlot > 0
            && ComboSystem.Instance.GetComboProgress() == 0
            && !ComboSystem.Instance.HasCompletedCombo())
        {
            nextSlot = 0;
            breakSfxPlayed = false;
            anyWrongKey = false;
        }

        AudioComponent audio = GetAudio();
        string expectedCombo = ComboSystem.Instance.GetExpectedCombo();
        if (string.IsNullOrEmpty(expectedCombo)) return;

        int slotIndex = nextSlot;
        nextSlot++;

        // Clamp to prevent out-of-bounds if the player keeps pressing after the combo ends
        if (slotIndex >= expectedCombo.Length)
        {
            nextSlot = expectedCombo.Length;
            return;
        }

        bool isLastKey = slotIndex == expectedCombo.Length - 1;
        bool isCorrectKey = key == expectedCombo[slotIndex];
        if (!isCorrectKey) anyWrongKey = true;   // wrong key = miss

        bool wasCompleted = ComboSystem.Instance.HasCompletedCombo();
        int progressBefore = ComboSystem.Instance.GetComboProgress();
        ComboSystem.Instance.RegisterInput(key);
        bool timingAccepted = ComboSystem.Instance.GetComboProgress() > progressBefore
                              || ComboSystem.Instance.PlayerDidSuccessfulCombo;

        // If ComboSystem silently ignored the press (too soon — minTimeBetweenPresses gate),
        // undo the slot advance so nextSlot stays in sync with ComboSystem.currentCombo.Count.
        bool wasIgnored = !timingAccepted && !wasCompleted && !ComboSystem.Instance.HasCompletedCombo();
        if (wasIgnored)
        {
            nextSlot--;
            return;
        }

        // Timing miss (correct key but outside window) also counts as a miss
        if (!wasCompleted && !timingAccepted) anyWrongKey = true;

        if (wasCompleted)
        {
            // Combo already ended on a previous slot — ComboSystem won't process this press,
            // so compute timing and judgement independently.
            string judgement = "MISS";
            bool timingOk    = false;
            if (Conductor.Instance != null && TurnManager.Instance != null)
            {
                float songTime      = Conductor.Instance.GetVisualAdjustedSongTime();
                float[] offsets     = EnemyRhythmController.Instance?.SlotOffsets ?? new float[] { 0f, 0.5f, 1f, 1.5f };
                float perfectWindow = ComboSystem.Instance?.perfectWindow ?? 0.08f;
                float greatWindow   = ComboSystem.Instance?.greatWindow   ?? 0.15f;
                if (slotIndex < offsets.Length)
                {
                    float earlyShift  = ComboSystem.Instance?.InputEarlyShift ?? 0f;
                    float expectedTime = TurnManager.Instance.GetPlayerInputStartTime() + offsets[slotIndex] - earlyShift;
                    float delta = Math.Abs(songTime - expectedTime);
                    if      (delta <= perfectWindow) { judgement = "PERFECT"; timingOk = true; }
                    else if (delta <= greatWindow)   { judgement = "GREAT";   timingOk = true; }
                }
            }

            // Always play click SFX on every key including last
            audio.PlaySFX(isCorrectKey ? "SFX_W_Key" : "SFX_Invalid_Key");
            ComboDisplayManager.Instance?.ShowJudgement(isCorrectKey && timingOk ? judgement : "MISS");
            ComboDisplayManager.Instance?.ForceShowPlayerKeyFeedback(slotIndex, key, isCorrectKey && timingOk, expectedCombo[slotIndex]);

            // Last beat: establish SFX if all keys were correct, break SFX if any were wrong
            if (isLastKey && slotIndex > 0)
            {
                if (!breakSfxPlayed)
                {
                    Console.WriteLine($"[InputBridge] LAST BEAT (wasCompleted path) anyWrongKey={anyWrongKey} isCorrectKey={isCorrectKey} slotIndex={slotIndex}");
                    if (!timingOk) anyWrongKey = true; // timing miss in wasCompleted path
                    audio.PlaySFX(!anyWrongKey ? "SFX_ComboEstablished_1" : "SFX_ComboStreakBroken");
                    breakSfxPlayed = true;
                }
                else
                {
                    Console.WriteLine($"[InputBridge] LAST BEAT BLOCKED (wasCompleted path) breakSfxPlayed already true! anyWrongKey={anyWrongKey}");
                }
            }
            return;
        }

        // Click SFX:
        // - Beats 1–(N-1): key correctness determines W_Key vs Invalid_Key
        // - Last beat: timing determines W_Key vs Invalid_Key (out of window = Invalid even if correct key)
        if (isLastKey)
            audio.PlaySFX(timingAccepted ? "SFX_W_Key" : "SFX_Invalid_Key");
        else
            audio.PlaySFX(isCorrectKey ? "SFX_W_Key" : "SFX_Invalid_Key");

        // Show green only if the key was correct AND timing was within the window
        ComboDisplayManager.Instance?.ForceShowPlayerKeyFeedback(slotIndex, key, isCorrectKey && timingAccepted, expectedCombo[slotIndex]);

        // Last beat: establish SFX if all keys were correct, break SFX if any were wrong
        if (isLastKey)
        {
            Console.WriteLine($"[InputBridge] LAST BEAT (normal path) anyWrongKey={anyWrongKey} isCorrectKey={isCorrectKey} timingAccepted={timingAccepted} PlayerDidSuccessfulCombo={ComboSystem.Instance.PlayerDidSuccessfulCombo}");
            if (!anyWrongKey)
            {
                audio.PlaySFX("SFX_ComboEstablished_1");
                breakSfxPlayed = true;

                // Damage + attack only when timing was also good
                if (ComboSystem.Instance.PlayerDidSuccessfulCombo)
                {
                    float playerDamage = PlayerHealth.Instance != null ? PlayerHealth.Instance.GetHealth().damage : 10;
                    int comboLen = expectedCombo.Length;
                    if (comboLen == 6) playerDamage = 10f;
                    else if (comboLen == 8) playerDamage = 15f;
                    EnemyRhythmController.Instance?.TakeDamage(playerDamage);
                    PlayerAnimationController.Instance?.PlayAttack(expectedCombo);
                }
            }
            else if (slotIndex > 0)
            {
                audio.PlaySFX("SFX_ComboStreakBroken");
                breakSfxPlayed = true;
            }
        }
    }

    // =========================
    // Missed Turn Detection
    // =========================
    private void CheckMissedTurn()
    {
        // Fire break SFX at the last beat moment if the player pressed no keys this turn
        if (nextSlot > 0 || breakSfxPlayed) return;
        if (ComboSystem.Instance == null || TurnManager.Instance == null || Conductor.Instance == null) return;

        string expectedCombo = ComboSystem.Instance.GetExpectedCombo();
        if (string.IsNullOrEmpty(expectedCombo)) return;

        float[] offsets = EnemyRhythmController.Instance?.SlotOffsets ?? new float[] { 0f, 0.5f, 1f, 1.5f };
        int lastSlot = expectedCombo.Length - 1;
        if (lastSlot >= offsets.Length) return;

        float earlyShift  = ComboSystem.Instance.InputEarlyShift;
        float greatWindow = ComboSystem.Instance.greatWindow;
        float lastBeatTime = TurnManager.Instance.GetPlayerInputStartTime() + offsets[lastSlot] - earlyShift;
        float currentTime  = Conductor.Instance.GetVisualAdjustedSongTime();

        if (currentTime > lastBeatTime + greatWindow)
        {
            Console.WriteLine($"[InputBridge] CheckMissedTurn fired: songTime={currentTime:F3} lastBeatTime={lastBeatTime:F3} playerInputStart={TurnManager.Instance.GetPlayerInputStartTime():F3}");
            AudioComponent audio = GetAudio();
            audio.PlaySFX("SFX_ComboStreakBroken");
            breakSfxPlayed = true;
        }
    }

    public void OnDestroy()
    {
        if (Instance == this)
            Instance = null;
    }

    // =========================
    // Invalid Key Handling
    // =========================
    private void CheckInvalidKeys(InputComponent input)
    {
        if (StageClearElement.IsCleared)
        {
            prevInvalidKeyDown = false;
            return;
        }

        int[] invalidKeys =
        {
            81, 69, 82, 84, 89, 85, 73, 79, 80,
            70, 71, 72, 74, 75, 76, 90, 88, 67, 86,
            66, 78, 77
        };

        bool anyInvalidDown = false;
        foreach (int keyCode in invalidKeys)
        {
            if (DebugTreatExtraKeyAsValid && keyCode == DebugAllowedExtraKey) continue;
            if (input.IsKeyPressed(keyCode)) { anyInvalidDown = true; break; }
        }

        if (anyInvalidDown && !prevInvalidKeyDown)
        {
            AudioComponent audio = GetAudio();
            audio.PlaySFX("SFX_Invalid_Key");
            Console.WriteLine("[InputBridge] Invalid key pressed");
        }

        prevInvalidKeyDown = anyInvalidDown;
    }
}
