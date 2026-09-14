using System;
using ScriptAPI;

public class HitVFX : Script
{
    public static float SpawnX = 0f;
    public static float SpawnY = 0f;

    private const float FadeInDuration  = 0.12f;
    private const float HoldDuration    = 0.08f;
    private const float FadeOutDuration = 0.25f;
    private const float TotalDuration   = FadeInDuration + HoldDuration + FadeOutDuration;

    private int   initFrames = 0;
    private float elapsed    = 0f;

    public override void Update()
    {
        initFrames++;
        if (initFrames < 2) return;

        if (initFrames == 2)
        {
            var tr = GetTransform();
            tr.X = SpawnX;
            tr.Y = SpawnY;

            var sp = GetSprite();
            sp.SetTint(1f, 1f, 1f, 0f); // start fully transparent

            float speed = 7f / TotalDuration; // play all 7 frames across the full VFX duration
            GetAnimation().SetAnimation("particle", 3, 3, 7, speed);

            VFXAPI.ShakeCamera(0.05f, 8f); // light shake
            return;
        }

        elapsed += Time.DeltaTime;

        float alpha;
        if (elapsed < FadeInDuration)
            alpha = elapsed / FadeInDuration;
        else if (elapsed < FadeInDuration + HoldDuration)
            alpha = 1f;
        else
            alpha = 1f - (elapsed - FadeInDuration - HoldDuration) / FadeOutDuration;

        alpha = Math.Max(0f, Math.Min(1f, alpha));

        var sprite = GetSprite();
        sprite.SetTint(1f, 1f, 1f, alpha);

        if (elapsed >= TotalDuration)
        {
            var tr = GetTransform();
            tr.IsVisible = false;
            var h = GetHealth();
            h.isAlive = false;
        }
    }
}
