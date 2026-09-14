using System;
using System.Collections.Generic;
using ScriptAPI;

public class LatencyCalibrator : Script
{
    public static LatencyCalibrator? Instance { get; private set; }

    // -------------------------------------------------------------------------
    // Tunables
    // -------------------------------------------------------------------------
    public string PingSFXName  = "SFX_Border_Beat"; // metronome click SFX
    public int    SampleCount  = 12;                // taps to record
    public float  CalibBPM     = 60f;               // 60 BPM = 1 tick/s, easy to sync with
    public int    WarmupBeats  = 0;                 // taps to discard before recording (0 = record immediately)
    public string NextScene    = "VisualLatencyCalibration";

    // -------------------------------------------------------------------------
    // Internal state
    // -------------------------------------------------------------------------
    private bool  calibrating  = false;
    private bool  done         = false;
    private float beatInterval = 1.0f;   // seconds per calibration beat (= 60 / CalibBPM)
    private float calibOrigin  = 0f;     // raw songTime of beat 0 on the calibration grid
    private float nextPingTime = 0f;     // raw songTime of the next scheduled tick
    private int   warmupLeft   = 0;      // warm-up taps remaining
    private float lastTapTime  = -999f;  // debounce: raw songTime of most recent tap
    private List<float> signedDeltas = new List<float>();
    private float resultOffset = 0f;    // final MusicLatencyOffset written to Conductor
    private float lastSampleMs = 0f;    // most recent signed delta in ms (shown in UI)
    private float stdDevMs     = 0f;
    private bool audioMixApplied = false;

    public override void Update()
    {
        Instance = this;
        if (done) return;

        if (!audioMixApplied)
        {
            audioMixApplied = true;
            AudioComponent audio = GetAudio();
            audio.SetMasterVolume(UserSettingsStore.GetSliderValue("Volume", 1.0f));
            audio.SetMusicVolume(UserSettingsStore.GetSliderValue("Music", 1.0f));
            audio.SetSfxVolume(UserSettingsStore.GetSliderValue("SFX", 0.25f));
        }

        // Allow Conductor to tick even before normal gameplay starts
        GameStart.Ready = true;

        if (Conductor.Instance == null) return;

        // Use raw song time so we measure absolute audio delay,
        float songTime = Conductor.Instance.GetSongTime();

        if (!calibrating)
        {
            var inp = GetInput();
            if (inp.IsKeyTriggered(257) || inp.IsKeyTriggered(32) // Enter / Space
                || inp.IsGamepadButtonTriggered(2))               // Square (GLFW button 2)
            {
                beatInterval = 60f / CalibBPM;
                calibOrigin  = songTime + beatInterval; // first tick 1 beat from now
                nextPingTime = calibOrigin;
                warmupLeft   = WarmupBeats;
                calibrating  = true;
                signedDeltas.Clear();
                Console.WriteLine($"[LatencyCalibrator] Starting metronome at {CalibBPM} BPM." +
                                  $" Tap SPACE in time. First {WarmupBeats} taps are warm-up.");
            }
            return;
        }

        // Fire metronome ticks continuously 
        while (songTime >= nextPingTime)
        {
            GetAudio().PlaySFX(PingSFXName);
            Console.WriteLine($"[LatencyCalibrator] Tick at t={nextPingTime:F3}s");
            nextPingTime += beatInterval;
        }

        if (GetInput().IsKeyTriggered(32) || GetInput().IsGamepadButtonTriggered(2)) // Space / Square
        {
            // Debounce: ignore a tap that arrives within half a beat of the last one.
            // This prevents accidental double-taps counting as two samples.
            if (songTime - lastTapTime < beatInterval * 0.5f)
                return;

            lastTapTime = songTime;

            if (warmupLeft > 0)
            {
                warmupLeft--;
                Console.WriteLine($"[LatencyCalibrator] Warm-up tap ({WarmupBeats - warmupLeft}/{WarmupBeats}) — not recorded");
            }
            else
            {
                // Snap to the nearest beat on the grid.
                float elapsed     = songTime - calibOrigin;
                float nearestIdx  = (float)Math.Round(elapsed / beatInterval);
                float nearestBeat = calibOrigin + nearestIdx * beatInterval;
                float signedDelta = songTime - nearestBeat;

                signedDeltas.Add(signedDelta);
                lastSampleMs = signedDelta * 1000f;

                string dir = signedDelta >= 0f ? "late" : "early";
                Console.WriteLine($"[LatencyCalibrator] Sample {signedDeltas.Count}/{SampleCount}: " +
                                  $"{lastSampleMs:+0.0;-0.0;0}ms ({dir})");
            }
        }

        // ENOOUFH
        if (signedDeltas.Count >= SampleCount)
        {
            float sum = 0f;
            foreach (float d in signedDeltas) sum += d;
            float avgDelta = sum / signedDeltas.Count;

            float variance = 0f;
            foreach (float d in signedDeltas)
            {
                float diff = d * 1000f - avgDelta * 1000f;
                variance += diff * diff;
            }
            stdDevMs = (float)Math.Sqrt(variance / signedDeltas.Count);

            // Shift the existing calibrated offset by the measured average delay.
            // Clamp to 0 so we never use a negative offset.
            resultOffset = Math.Max(0f, Conductor.CalibratedOffset + avgDelta);
            Conductor.CalibratedOffset = resultOffset;
            if (Conductor.Instance != null)
                Conductor.Instance.MusicLatencyOffset = resultOffset;

            Console.WriteLine($"[LatencyCalibrator] Done! " +
                              $"AvgDelta={avgDelta*1000f:+0.0;-0.0;0}ms | " +
                              $"StdDev={stdDevMs:F0}ms | " +
                              $"NewOffset={resultOffset*1000f:F0}ms — applied to Conductor.");
            done = true;
        }
    }

    public void Reset()
    {
        calibrating  = false;
        done         = false;
        warmupLeft   = 0;
        lastTapTime  = -999f;
        nextPingTime = 0f;
        signedDeltas.Clear();
        Console.WriteLine("[LatencyCalibrator] Reset — ready to calibrate again.");
    }

    public void OnDestroy()
    {
        if (Instance == this) Instance = null;
    }

    public void Skip()
    {
        Console.WriteLine("[LatencyCalibrator] Skipped — keeping existing MusicLatencyOffset.");
        Scene.LoadScene(NextScene);
    }

    // Read-only access for UI scripts 
    public bool  IsCalibrating      => calibrating;
    public bool  IsDone             => done;
    public int   GetSamplesDone()   => signedDeltas.Count;
    public int   GetSamplesNeeded() => SampleCount;
    public float GetResultMs()      => resultOffset * 1000f;
    public float GetLastSampleMs()  => lastSampleMs;   // signed: + = late, - = early
    public float GetStdDevMs()      => stdDevMs;

    public string GetConsistencyRating()
    {
        if (stdDevMs < 15f) return "Excellent";
        if (stdDevMs < 30f) return "Good";
        if (stdDevMs < 50f) return "Fair";
        return "Inconsistent";
    }

    public bool ShouldRetry() => stdDevMs >= 50f;
}
