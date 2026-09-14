using System;
using ScriptAPI;

public class HealthBGController : Script
{
    public static HealthBGController? Instance { get; private set; }

    // Back Glow Sprites
    public string blueGlowSprite = "back_glow_blue";
    public string redGlowSprite = "back_glow_red";

    private float flashTimer = 0f;
    private float flashDuration = 0.5f;
    private bool isFlashing = false;

    private bool initialized = false;

    public override void Update()
    {
        if (!initialized)
        {
            if (Instance != null && Instance != this)
            {
                Console.WriteLine("[HealthBGController] Replacing stale singleton instance");
            }
            Instance = this;

            // Force dimensions
            var transform = GetTransform();
            transform.ScaleX = 300.0f;
            transform.ScaleY = 104.475f;

            var sprite = GetSprite();
            if (sprite.HasSprite())
            {
                sprite.Texture = blueGlowSprite;
                Console.WriteLine("[HealthBGController] Set default blue glow sprite");
            }

            initialized = true;
            Console.WriteLine("[HealthBGController] Initialized");
            return;
        }

        if (isFlashing)
        {
            flashTimer -= Time.DeltaTime;
            if (flashTimer <= 0f)
            {
                isFlashing = false;

                // Force dimensions on swap back
                var transform = GetTransform();
                transform.ScaleX = 300.0f;
                transform.ScaleY = 104.475f;

                var sprite = GetSprite();
                if (sprite.HasSprite())
                {
                    sprite.Texture = blueGlowSprite;
                    Console.WriteLine("[HealthBGController] Flash ended, back to blue");
                }
            }
        }
    }

    public void TriggerDamageFlash()
    {
        // Force dimensions on flash
        var transform = GetTransform();
        transform.ScaleX = 300.0f;
        transform.ScaleY = 104.475f;

        var sprite = GetSprite();
        if (sprite.HasSprite())
        {
            sprite.Texture = redGlowSprite;
            flashTimer = flashDuration;
            isFlashing = true;
            Console.WriteLine("[HealthBGController] Damage flash triggered");
        }
    }
}