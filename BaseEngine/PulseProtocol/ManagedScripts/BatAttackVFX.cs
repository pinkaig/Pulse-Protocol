using System;
using ScriptAPI;

public class BatAttackVFX : Script
{
    private int   initFrames = 0;
    private float elapsed    = 0f;
    private float duration   = 0f;

    private const float FadeStartFraction = 0.5f; // begin fading at 50% through

    public override void Update()
    {
        initFrames++;
        if (initFrames < 2) return;

        if (initFrames == 2)
        {
            var tr = GetTransform();
            tr.X = SwordSlashVFX.SpawnX;
            tr.Y = SwordSlashVFX.SpawnY;

            duration = SwordSlashVFX.SpawnDuration;
            float speed = 22f / duration;
            GetAnimation().SetAnimation("bat_attack_vfx", 5, 5, 22, speed);
            return;
        }

        elapsed += Time.DeltaTime;

        // Fade out over the second half
        float fadeStart = duration * FadeStartFraction;
        if (elapsed >= fadeStart)
        {
            float t = (elapsed - fadeStart) / (duration - fadeStart);
            float alpha = Math.Max(0f, 1f - t);
            var sp = GetSprite();
            sp.SetTint(1f, 1f, 1f, alpha);
        }

        if (elapsed >= duration)
        {
            var tr = GetTransform();
            tr.IsVisible = false;
            var h = GetHealth();
            h.isAlive = false;
        }
    }
}
