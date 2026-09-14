using System;
using ScriptAPI;

public class HealthBarController : Script
{
    public static HealthBarController? Instance { get; private set; }

    private string[] healthFrames = new string[]
    {
        "health_tile001",
        "health_tile002",
        "health_tile003",
        "health_tile004",
        "health_tile005",
        "health_tile006",
        "health_tile007",
        "health_tile008",
        "health_tile009",
        "health_tile010",
        "health_tile011"
    };

    // Match the inspector values
    private float forceScaleX = 400.0f;
    private float forceScaleY = 184.253f;

    private int totalFrames = 11;
    private bool initialized = false;
    private int lastFrame = -1;

    public override void Update()
    {
        if (!initialized)
        {
            if (Instance != null && Instance != this)
            {
                Console.WriteLine("[HealthBGController] Replacing stale singleton instance");
            }
            Instance = this;

            var anim = GetAnimation();
            anim.SetAnimationSpeed(0f);

            // Show full health
            var sprite = GetSprite();
            if (sprite.HasSprite())
            {
                sprite.Texture = healthFrames[0];
            }

            // Force transform size
            var transform = GetTransform();
            transform.ScaleX = forceScaleX;
            transform.ScaleY = forceScaleY;

            lastFrame = 0;
            initialized = true;
            Console.WriteLine("[HealthBarController] Initialized");
            return;
        }

        var t = GetTransform();
        t.ScaleX = forceScaleX;
        t.ScaleY = forceScaleY;

        if (PlayerHealth.Instance != null)
        {
            float healthPercent = PlayerHealth.Instance.GetHealthPercent();

            int frame = (int)Math.Round((1.0f - healthPercent) * (totalFrames - 1));
            if (frame < 0) frame = 0;
            if (frame >= totalFrames) frame = totalFrames - 1;

            if (frame != lastFrame)
            {
                var sprite = GetSprite();
                if (sprite.HasSprite())
                {
                    sprite.Texture = healthFrames[frame];
                }

                t.ScaleX = forceScaleX;
                t.ScaleY = forceScaleY;

                lastFrame = frame;
                Console.WriteLine($"[HealthBarController] HP: {healthPercent * 100:F0}% -> Frame {frame}");
            }
        }
    }
}