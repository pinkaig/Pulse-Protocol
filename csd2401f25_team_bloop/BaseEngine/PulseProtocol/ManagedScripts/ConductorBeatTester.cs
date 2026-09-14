using System;
using ScriptAPI;

public class ConductorBeatTester : Script
{
    private int lastSeenBeat = -999;
    private float timeSinceLastBeat = 0.0f;

    public override void Update()
    {
        float dt = Time.DeltaTime;
        timeSinceLastBeat += dt;

        Conductor? c = Conductor.Instance;
        if (c == null)
        {
            if (Time.FrameCount % 60 == 0)
                Console.WriteLine("[BeatTester] Waiting for Conductor.Instance...");
            return;
        }

        int beat = c.GetCurrentBeat();

        if (beat != lastSeenBeat)
        {
            if (lastSeenBeat >= 0)
            {
                Console.WriteLine(
                    $"[BeatTester] Beat={beat} | Interval={timeSinceLastBeat:F3}s | Expected={c.SecondsPerBeat:F3}s | GameTime={Time.GameTime:F3}"
                );
            }
            else
            {
                Console.WriteLine(
                    $"[BeatTester] Beat={beat} | Expected={c.SecondsPerBeat:F3}s | GameTime={Time.GameTime:F3}"
                );
            }

            lastSeenBeat = beat;
            timeSinceLastBeat = 0.0f;
        }
    }
}

