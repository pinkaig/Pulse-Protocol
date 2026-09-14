using System;
using ScriptAPI;

// =============================================================================
// LatencySliderApplier
//
// Attach ONE of these anywhere in the Settings scene.
// It reads the "Latency" slider knob value (0→1) and maps it to
// Conductor.MusicLatencyOffset so players can compensate for their
// machine's audio hardware latency.
//
// Slider range: MinLatencyMs → MaxLatencyMs  (default 0 ms → 300 ms)
// Default value: DefaultLatencyMs            (default 100 ms = existing offset)
//
// HOW TO ADD THE SLIDER TO YOUR SETTINGS SCENE:
//   1. Create a knob entity  → attach LatencyKnob  script
//   2. Create a fill entity  → attach LatencyFill  script
//   3. Create any entity     → attach LatencySliderApplier script
//      (place this entity BEFORE the knob entity in the scene so it
//       pre-seeds the default value before the knob initialises)
// =============================================================================
public class LatencySliderApplier : Script
{
    // Range in milliseconds exposed to the editor
    public float MinLatencyMs = 0f;
    public float MaxLatencyMs = 500f;

    // What to use when no saved value exists yet
    public float DefaultLatencyMs = 80f;

    private bool seeded = false;

    public override void Update()
    {
        // Seed the default once before the knob reads States["Latency"]
        if (!seeded)
        {
            if (!AudioSliderKnobBase.States.ContainsKey("Latency"))
            {
                float defaultV01 = (DefaultLatencyMs - MinLatencyMs) / (MaxLatencyMs - MinLatencyMs);
                defaultV01 = Clamp01(defaultV01);
                var st = new AudioSliderKnobBase.SliderState();
                st.Value01 = defaultV01;
                AudioSliderKnobBase.States["Latency"] = st;
                Console.WriteLine($"[LatencySlider] Seeded default {DefaultLatencyMs:F0} ms (v01={defaultV01:F3})");
            }
            seeded = true;
        }

        if (!AudioSliderKnobBase.States.ContainsKey("Latency")) return;

        float v01 = AudioSliderKnobBase.States["Latency"].Value01;
        float offsetMs = MinLatencyMs + v01 * (MaxLatencyMs - MinLatencyMs);
        float offsetSec = offsetMs / 1000f;

        // Update static so new Conductor instances (level loads) also pick up the saved value
        Conductor.CalibratedOffset = offsetSec;
        if (Conductor.Instance != null)
            Conductor.Instance.MusicLatencyOffset = offsetSec;
    }

    // Returns the current latency in ms (for display text, etc.)
    public float GetCurrentLatencyMs()
    {
        if (!AudioSliderKnobBase.States.ContainsKey("Latency")) return DefaultLatencyMs;
        float v01 = AudioSliderKnobBase.States["Latency"].Value01;
        return MinLatencyMs + v01 * (MaxLatencyMs - MinLatencyMs);
    }

    private float Clamp01(float v)
    {
        if (v < 0f) return 0f;
        if (v > 1f) return 1f;
        return v;
    }
}
