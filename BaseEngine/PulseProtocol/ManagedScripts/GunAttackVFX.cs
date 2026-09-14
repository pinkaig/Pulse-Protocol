using System;
using ScriptAPI;

public class GunAttackVFX : Script
{
    private int   initFrames = 0;
    private float elapsed    = 0f;
    private float duration   = 0f;

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
            float speed = 23f / duration;
            GetAnimation().SetAnimation("gun_attack_vfx", 5, 5, 23, speed);
            return;
        }

        elapsed += Time.DeltaTime;
        if (elapsed >= duration)
        {
            var tr = GetTransform();
            tr.IsVisible = false;
            var h = GetHealth();
            h.isAlive = false;
        }
    }
}
