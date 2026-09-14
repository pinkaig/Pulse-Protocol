using System;
using ScriptAPI;

// Border layer of the boss health bar.
// Always shows frame 0 of boss_healthbar (the empty container/border).
// Visible only when the active enemy is the boss; hides for regular enemies.
// Sits on a lower sorting layer than BossHealthBarController (the fill layer).
public class BossHPBorderController : Script
{
    private const int BossSheetRows    = 3;
    private const int BossSheetColumns = 4;
    private const int BossTotalFrames  = 10;

    public float GlobalOffsetX = 0f;
    public float GlobalOffsetY = 0f;
    public float BossScaleX    = 659f;
    public float BossScaleY    = 68f;

    private bool initialized = false;

    public override void Update()
    {
        if (!initialized)
        {
            GetAnimation().SetAnimation("boss_healthbar", BossSheetRows, BossSheetColumns, BossTotalFrames, 0f);
            GetAnimation().SetCurrentFrame(0);
            initialized = true;
        }

        var barTf = GetTransform();

        if (EnemyRhythmController.Instance == null || EnemyRhythmController.Instance.IsDying)
        {
            barTf.IsVisible = false;
            return;
        }

        // Only show for the boss.
        if (!(EnemyRhythmController.Instance is BossEnemyController))
        {
            barTf.IsVisible = false;
            return;
        }

        barTf.IsVisible = true;
        barTf.ScaleX    = BossScaleX;
        barTf.ScaleY    = BossScaleY;

        // Mirror the same offset formula used by BossHealthBarController.
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

        // Lock to frame 0 every frame so the zero-speed tick can't drift.
        GetAnimation().SetCurrentFrame(0);
    }
}
