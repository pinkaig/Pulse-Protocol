using System;
using ScriptAPI;

public class VisualLatencyCalibrator : Script
{
    public static VisualLatencyCalibrator? Instance { get; private set; }

    // -------------------------------------------------------------------------
    // Tunables
    // -------------------------------------------------------------------------
    public string PingSFXName  = "SFX_Border_Beat"; // audio click SFX (same as VisualBeatSystem uses)
    public float  AdjustStepMs = 5f;                // ms shifted per key press / held-key step
    public float  MaxOffsetMs  = 200f;              // clamp range in both directions
    public string NextScene    = "MainMenu";

    // GLFW key codes
    private const int KeyRight = 262;
    private const int KeyLeft  = 263;
    private const int KeyEnter = 257;
    private const int KeySpace = 32;

    // Gamepad button constants — see InputConstants.cs
    private const int GP_SQUARE = InputConstants.GP_SQUARE;  // confirm / start
    private const int GP_R1     = InputConstants.GP_R1;      // flash earlier
    private const int GP_L1     = InputConstants.GP_L1;      // flash later

    // -------------------------------------------------------------------------
    // Internal state
    // -------------------------------------------------------------------------
    private bool  adjusting      = false;
    private bool  done           = false;
    private float beatInterval   = 0.5f;   // set from Conductor.SecondsPerBeat on start
    private float nextAudioTime  = 0f;     // raw songTime of next audio tick
    private float previewOffset  = 0f;     // written live to Conductor.DisplayLatencyOffset each frame
    private float originalOffset = 0f;     // saved on start so Skip() can revert
    private float timeSinceStart = 0f;     // guard against immediate re-confirm

    // Held-key auto-repeat
    private float rightHeld = 0f;
    private float leftHeld  = 0f;
    private const float HoldDelay = 0.1f;
    private const float HoldRate  = 0.06f;
    private bool audioMixApplied = false;

    public override void Update()
    {
        Instance = this;
        if (done)
        {
            NavigationButtons.ReturnToSettingsAfterCalibration();
            return;
        }

        if (!audioMixApplied)
        {
            audioMixApplied = true;
            AudioComponent audio = GetAudio();
            audio.SetMasterVolume(UserSettingsStore.GetSliderValue("Volume", 1.0f));
            audio.SetMusicVolume(UserSettingsStore.GetSliderValue("Music", 1.0f));
            audio.SetSfxVolume(UserSettingsStore.GetSliderValue("SFX", 0.25f));
        }

        // Allow Conductor to tick in this scene
        GameStart.Ready = true;

        if (Conductor.Instance == null) return;

        float spb = Conductor.Instance.SecondsPerBeat;
        if (spb <= 0f) return; // Conductor not yet initialized

        float songTime = Conductor.Instance.GetSongTime();
        float dt       = Time.DeltaTime;

        
        if (!adjusting)
        {
            var inp = GetInput();
            if (inp.IsKeyTriggered(KeyEnter) || inp.IsKeyTriggered(KeySpace)
                || inp.IsGamepadButtonTriggered(GP_SQUARE))
            {
                beatInterval = spb;

                // Align the audio click grid to the visual beat grid so that
                // when previewOffset = 0, click and border peak are simultaneous.
                //
                // Visual peaks when:
                //   cachedAdjustedTime + DisplayLatencyOffset = N * spb
                //   → rawSongTime = N * spb + (MusicLatencyOffset + BeatStartOffset) − previewOffset
                //
                // rawToAdj = rawSongTime − cachedAdjustedTime = MusicLatencyOffset + BeatStartOffset
                //
                float adjNow     = Conductor.Instance.GetAdjustedSongTime();
                float rawToAdj   = songTime - adjNow;   // = MusicLatencyOffset + BeatStartOffset

                // Next visual beat in adjusted-time space (skip the current beat with +epsilon)
                float nextVisAdj = (float)Math.Ceiling(adjNow / spb + 0.001f) * spb;
                float nextVisRaw = nextVisAdj + rawToAdj; // same instant in raw songTime (at offset=0)

                nextAudioTime  = nextVisRaw;  // first click fires with the first visual peak
                originalOffset = Conductor.CalibratedDisplayOffset;
                previewOffset  = originalOffset;
                timeSinceStart = 0f;
                adjusting      = true;

                Console.WriteLine($"[VisualLatencyCalibrator] Starting at {60f / spb:F0} BPM. " +
                                  $"Current offset: {previewOffset * 1000f:F0} ms. " +
                                  $"RIGHT = border earlier | LEFT = border later | ENTER = confirm.");
            }
            return;
        }

        timeSinceStart += dt;

       
        Conductor.Instance.DisplayLatencyOffset = previewOffset;

        while (songTime >= nextAudioTime)
        {
            GetAudio().PlaySFX(PingSFXName);
            nextAudioTime += beatInterval;
        }

        float stepS = AdjustStepMs / 1000f;
        float maxS  = MaxOffsetMs  / 1000f;
        var   input = GetInput();

        bool rightDown = input.IsKeyPressed(KeyRight) || input.IsGamepadButtonPressed(GP_R1);
        bool leftDown  = input.IsKeyPressed(KeyLeft)  || input.IsGamepadButtonPressed(GP_L1);

        rightHeld = rightDown ? rightHeld + dt : 0f;
        leftHeld  = leftDown  ? leftHeld  + dt : 0f;

        bool stepRight = input.IsKeyTriggered(KeyRight) || input.IsGamepadButtonTriggered(GP_R1)
                         || (rightHeld > HoldDelay && rightHeld % HoldRate < dt);
        bool stepLeft  = input.IsKeyTriggered(KeyLeft)  || input.IsGamepadButtonTriggered(GP_L1)
                         || (leftHeld  > HoldDelay && leftHeld  % HoldRate < dt);

        if (stepRight)
        {
            previewOffset = Math.Min(maxS, previewOffset + stepS);
            Console.WriteLine($"[VisualLatencyCalibrator] Offset: {previewOffset * 1000f:+0.0;-0.0;0} ms  (border earlier)");
        }
        if (stepLeft)
        {
            previewOffset = Math.Max(-maxS, previewOffset - stepS);
            Console.WriteLine($"[VisualLatencyCalibrator] Offset: {previewOffset * 1000f:+0.0;-0.0;0} ms  (border later)");
        }

        if (timeSinceStart > 1.0f &&
            (input.IsKeyTriggered(KeyEnter) || input.IsKeyTriggered(KeySpace)
             || input.IsGamepadButtonTriggered(GP_SQUARE)))
        {
            Conductor.CalibratedDisplayOffset       = previewOffset;
            Conductor.Instance.DisplayLatencyOffset = previewOffset;
            Console.WriteLine($"[VisualLatencyCalibrator] Confirmed! " +
                              $"DisplayLatencyOffset = {previewOffset * 1000f:F0} ms saved.");
            done = true;
        }
    }

    public void Reset()
    {
        adjusting      = false;
        done           = false;
        timeSinceStart = 0f;
        rightHeld      = 0f;
        leftHeld       = 0f;
        previewOffset  = Conductor.CalibratedDisplayOffset;
        if (Conductor.Instance != null)
            Conductor.Instance.DisplayLatencyOffset = previewOffset;
        Console.WriteLine("[VisualLatencyCalibrator] Reset — ready to calibrate again.");
    }

    public void OnDestroy()
    {
        if (!done && Conductor.Instance != null)
            Conductor.Instance.DisplayLatencyOffset = originalOffset;

        if (Instance == this) Instance = null;
    }

    public void Skip()
    {
        if (Conductor.Instance != null)
            Conductor.Instance.DisplayLatencyOffset = Conductor.CalibratedDisplayOffset;

        Console.WriteLine("[VisualLatencyCalibrator] Skipped — keeping existing DisplayLatencyOffset.");
        NavigationButtons.ReturnToSettingsAfterCalibration();
    }

    // Read-only access for UI scripts
    public bool  IsAdjusting   => adjusting;
    public bool  IsDone        => done;
    public float GetOffsetMs() => previewOffset * 1000f;

    public string GetOffsetLabel()
    {
        float ms = previewOffset * 1000f;
        if      (ms >  10f) return $"+{ms:F0} ms  (border pre-fired — compensating for display lag)";
        else if (ms < -10f) return $"{ms:F0} ms  (border delayed — display faster than expected)";
        else                return $"~0 ms  (audio and visual in sync)";
    }
}
