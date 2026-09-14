/******************************************************************************/
/**
 * @file        EnemyHealthBarController.cs
 * @project     Pulse Protocol
 * @author      Leu Jun Yong (primary) - 100%
 * @brief       Updates enemy health bar sprites based on current health state.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/

using System;
using ScriptAPI;

public class EnemyHealthBarController : Script
{
    // Generated from enemy health bar atlas (enemy health bar_176x33.png).
    // Each set is "full -> empty" for a fixed block count.
    private readonly string[] healthFrames2 =
    {
        "../../PulseProtocol/Assets/Textures/UI/enemy_hp2_00.png", // full
        "../../PulseProtocol/Assets/Textures/UI/enemy_hp2_01.png",
        "../../PulseProtocol/Assets/Textures/UI/enemy_hp2_02.png"  // empty
    };

    private readonly string[] healthFrames3 =
    {
        "../../PulseProtocol/Assets/Textures/UI/enemy_hp3_00.png",
        "../../PulseProtocol/Assets/Textures/UI/enemy_hp3_01.png",
        "../../PulseProtocol/Assets/Textures/UI/enemy_hp3_02.png",
        "../../PulseProtocol/Assets/Textures/UI/enemy_hp3_03.png"
    };

    private readonly string[] healthFrames4 =
    {
        "../../PulseProtocol/Assets/Textures/UI/enemy_hp4_00.png",
        "../../PulseProtocol/Assets/Textures/UI/enemy_hp4_01.png",
        "../../PulseProtocol/Assets/Textures/UI/enemy_hp4_02.png",
        "../../PulseProtocol/Assets/Textures/UI/enemy_hp4_03.png",
        "../../PulseProtocol/Assets/Textures/UI/enemy_hp4_04.png"
    };

    public float GlobalOffsetX = 0f;
    public float GlobalOffsetY = 0f;
    public float ScaleX = 176f;
    public float ScaleY = 33f;

    private int lastFrame = -1;
    private int lastBlocks = -1;
    private bool initialized = false;

    public override void Update()
    {
        if (!initialized)
        {
            initialized = true;
            var anim = GetAnimation();
            anim.SetAnimationSpeed(0f);
        }

        var barTf = GetTransform();
        var barSprite = GetSprite();

        if (EnemyRhythmController.Instance == null || EnemyRhythmController.Instance.IsDying)
        {
            barTf.IsVisible = false;
            lastFrame = -1;
            lastBlocks = -1;
            return;
        }

        barTf.IsVisible = true;

        var enemyTf = EnemyRhythmController.Instance.GetTransform();
        float offsetX =
            GlobalOffsetX +
            EnemyRhythmController.Instance.GetHealthBarOffsetX() +
            (enemyTf.ScaleX * EnemyRhythmController.Instance.GetHealthBarScaleOffsetXFactor());

        float offsetY =
            GlobalOffsetY +
            EnemyRhythmController.Instance.GetHealthBarOffsetY() -
            (enemyTf.ScaleY * EnemyRhythmController.Instance.GetHealthBarScaleOffsetYFactor());

        barTf.X = enemyTf.X + offsetX;
        barTf.Y = enemyTf.Y + offsetY;
        barTf.ScaleX = ScaleX;
        barTf.ScaleY = ScaleY;

        int blocks = EnemyRhythmController.Instance.GetHealthBarBlockCount();
        if (blocks < 2) blocks = 2;
        if (blocks > 4) blocks = 4;

        float healthPercent = EnemyRhythmController.Instance.GetHealthPercent();
        int missingBlocks = (int)Math.Round((1.0f - healthPercent) * blocks);
        if (missingBlocks < 0) missingBlocks = 0;
        if (missingBlocks > blocks) missingBlocks = blocks;

        string[] activeFrames = healthFrames4;
        if (blocks == 2) activeFrames = healthFrames2;
        else if (blocks == 3) activeFrames = healthFrames3;

        if ((missingBlocks != lastFrame || blocks != lastBlocks) && barSprite.HasSprite())
        {
            barSprite.Texture = activeFrames[missingBlocks];
            lastFrame = missingBlocks;
            lastBlocks = blocks;
        }
    }
}
