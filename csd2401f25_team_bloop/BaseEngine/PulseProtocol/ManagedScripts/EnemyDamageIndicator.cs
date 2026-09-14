/******************************************************************************/
/**
 * @file        EnemyDamageIndicator.cs
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En
 * @brief       Damage indicator script that spawns a floating text showing
 *              damage dealt, fading out and moving upward before destroying itself.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/
using System;
using ScriptAPI;

public class EnemyDamageIndicator : Script
{
    public static float SpawnX = 0f;
    public static float SpawnY = 0f;
    public static string DamageText = "";

    private float duration = 1.0f;
    private float elapsed = 0.0f;
    private float floatSpeed = 80.0f;
    private int initFrames = 0;

    public override void Update()
    {
        initFrames++;
        if (initFrames < 2) return;

        if (initFrames == 2)
        {
            var tr = GetTransform();
            tr.X = SpawnX - 150f;
            tr.Y = SpawnY + 150f;

            var text = GetTextComponent();
            text.SetText(DamageText);
            text.SetAlignment(1); // center align
            text.SetFontSize(60.0f);
            // Console.WriteLine($"[DamageIndicator] Init done, text={DamageText} alignment set to center");
            return;
        }

        float dt = Time.DeltaTime;
        elapsed += dt;
        float t = elapsed / duration;

        var tr2 = GetTransform();
        tr2.Y += floatSpeed * dt;

        var text2 = GetTextComponent();
        text2.SetAlpha(1.0f - t);

        if (elapsed >= duration)
        {
            var health = GetHealth();
            health.isAlive = false;
        }
    }
}