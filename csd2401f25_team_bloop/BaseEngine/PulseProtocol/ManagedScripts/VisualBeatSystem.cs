using System;
using ScriptAPI;

public class VisualBeatSystem : Script
{
    // =========================
    // Singleton
    // =========================
    public static VisualBeatSystem? Instance { get; private set; }

    // =========================
    // Pulse Settings
    // =========================
    public float scaleAmp = 0.03f;      // +3% swell
    public int pulsesPerBeat = 1;       // 1=every beat, 2=half, 4=quarter
    public float width = 0.12f;         // smaller = sharper/snappier pulse

    // =========================
    // Opacity Settings
    // =========================
    public float minOpacity = 0.20f;    // 20% (when "open"/expanded)
    public float maxOpacity = 1.00f;    // 100% (when "closed"/back to base)

    // =========================
    // Judgement Flash
    // =========================
    public float flashDuration = 0.3f;  // seconds the colour flash lasts

    private float _flashTimer = 0f;
    private float _flashR = 1f, _flashG = 1f, _flashB = 1f;

    // =========================
    // Cached base scale
    // =========================
    private bool _cachedBaseScale = false;
    private float _baseScaleX = 1f;
    private float _baseScaleY = 1f;

    public override void Update()
    {
        Instance = this;

        TransformComponent t = GetTransform();
        SpriteComponent s = GetSprite();

        // During stage clear, keep border fully hidden and stop beat animation.
        if (StageClearElement.IsCleared)
        {
            t.IsVisible = false;
            return;
        }

        if (!t.IsVisible)
            t.IsVisible = true;

        if (Conductor.Instance == null) return;

        float spb = Conductor.Instance!.SecondsPerBeat;
        if (spb <= 0.0001f) return;

        // Cache initial scale once
        if (!_cachedBaseScale)
        {
            _baseScaleX = t.ScaleX;
            _baseScaleY = t.ScaleY;
            _cachedBaseScale = true;
        }

        // =========================
        // Beat phase (0..1)
        // =========================
        // Pre-advance by one frame so the rendered spike appears on screen
        // on the same frame the beat is heard, compensating for display pipeline lag.
        float songTime = Conductor.Instance!.GetVisualAdjustedSongTime();

        int ppb = Math.Max(1, pulsesPerBeat);
        float subBeat = spb / ppb;

        float phase = songTime / subBeat;
        phase = phase - (float)Math.Floor(phase); // 0..1
        if (phase > 0.5f) phase -= 1.0f;          // -0.5..0.5, symmetric around the beat

        // =========================
        // Pulse curve (peaks exactly on the beat, fades symmetrically before and after)
        // =========================
        float w = Math.Max(0.0001f, width);
        float pulse = (float)Math.Exp(-(phase * phase) / (2f * w * w)); // ~1 at beat, -> 0 before/after

        // =========================
        // Scale: open on beat, close back
        // =========================
        float scale = 1f + pulse * scaleAmp;
        t.ScaleX = _baseScaleX * scale;
        t.ScaleY = _baseScaleY * scale;

        // =========================
        // Opacity: 100% on beat, dims as it decays
        // =========================
        float opacity = maxOpacity - (1f - pulse) * (maxOpacity - minOpacity);
        s.TintA = Clamp01(opacity);

        // =========================
        // Judgement colour flash
        // PERFECT = green, GREAT = blue, MISS = red
        // Fades back to white over flashDuration seconds
        // =========================
        if (_flashTimer > 0f)
        {
            _flashTimer -= Time.DeltaTime;
            float ft = Clamp01(_flashTimer / flashDuration); // 1 at flash start, 0 at end
            s.TintR = 1f - ft * (1f - _flashR);
            s.TintG = 1f - ft * (1f - _flashG);
            s.TintB = 1f - ft * (1f - _flashB);
        }
        else
        {
            s.TintR = 1f;
            s.TintG = 1f;
            s.TintB = 1f;
        }
    }

    // Called by ComboSystem whenever a judgement fires
    public void FlashJudgement(string judgement)
    {
        _flashTimer = flashDuration;
        switch (judgement)
        {
            case "PERFECT":
                _flashR = 0f; _flashG = 1f; _flashB = 0f;   // green
                break;
            case "GREAT":
                _flashR = 0f; _flashG = 0.5f; _flashB = 1f; // blue
                break;
            case "MISS":
                _flashR = 1f; _flashG = 0f; _flashB = 0f;   // red
                break;
            default:
                _flashR = 1f; _flashG = 1f; _flashB = 1f;
                break;
        }
    }

    private float Clamp01(float v)
    {
        if (v < 0f) return 0f;
        if (v > 1f) return 1f;
        return v;
    }
}
