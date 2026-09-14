using System;
using ScriptAPI;

public class SwordSlashVFX : Script
{
    public static float SpawnX = 0f;
    public static float SpawnY = 0f;
    // Duration of the attack animation
    public static float SpawnDuration = 0.5f;

    // Set by PlayerAnimatorController before instantiating during the boss fight.
    // "sword" -> swordslash anim, "spear" -> spear_attack_vfx anim. Cleared after use.
    public static string BossVFXOverride = "";

    // Old sword slash VFX (Sword_Slash_VFX_1099x392): 5 rows, 2 cols, 10 frames
    // public const int VFXTotalFrames = 10;
    // public const int VFXRows = 5;
    // public const int VFXCols = 2;

    private int initFrames = 0;
    private float elapsed = 0f;

    public override void Update()
    {
        initFrames++;
        if (initFrames < 2) return;

        if (initFrames == 2)
        {
            var tr = GetTransform();
            tr.X = SpawnX;
            tr.Y = SpawnY;

            // Scale speed so all frames finish in exactly SpawnDuration seconds
            float speed = 6 / SpawnDuration;
            Animation anim = GetAnimation();

            switch (EnemySpawner.enemyQueue.Peek())
            {
                case EnemyIconType.Rat:
                    // Old: anim.SetAnimation("sword_slash_vfx", 5, 2, 10, speed);
                    anim.SetAnimation("swordslash", 2, 3, 6, speed);
                    break;

                case EnemyIconType.BlueGorilla:
                    anim.SetAnimation("spear_attack_vfx", 4, 4, 13, speed);
                    break;

                case EnemyIconType.Boss:
                    if (BossVFXOverride == "spear")
                        anim.SetAnimation("spear_attack_vfx", 4, 4, 13, speed);
                    else
                        anim.SetAnimation("swordslash", 2, 3, 6, speed);
                    BossVFXOverride = "";
                    break;

                default:
                    // Old: anim.SetAnimation("sword_slash_vfx", 5, 2, 10, speed);
                    anim.SetAnimation("swordslash", 2, 3, 6, speed);
                    break;
            }

        }

        elapsed += Time.DeltaTime;
        if (elapsed >= SpawnDuration)
        {
            var tr = GetTransform();
            tr.IsVisible = false;
            var health = GetHealth();
            health.isAlive = false;
        }
    }

}