using System;
using ScriptAPI;

// Fill layer of the boss health bar.
// - Regular enemies : same texture-swap logic as EnemyHealthBarController.
// - Boss            : shows fill chips (frames 1-9) on top of the border entity.
//                     Hidden at 0 HP so only the border (frame 0) shows.
public class BossHealthBarController : Script
{
    // ── Regular-enemy textures (mirrors EnemyHealthBarController) ──────────
    private readonly string[] healthFrames2 =
    {
        "../../PulseProtocol/Assets/Textures/UI/enemy_hp2_00.png",
        "../../PulseProtocol/Assets/Textures/UI/enemy_hp2_01.png",
        "../../PulseProtocol/Assets/Textures/UI/enemy_hp2_02.png"
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

    // ── Regular-enemy sizing ───────────────────────────────────────────────
    public float GlobalOffsetX = 0f;
    public float GlobalOffsetY = 0f;
    public float ScaleX        = 176f;
    public float ScaleY        = 33f;

    // ── Boss fill-chip config ──────────────────────────────────────────────
    // Frame 0 = border (handled by BossHPBorderController).
    // Frames 1-9 = fill chips overlaid on the border.
    private const int   BossSheetRows    = 3;
    private const int   BossSheetColumns = 4;
    private const int   BossTotalFrames  = 10;
    private const int   BossFillStart    = 1;   // first fill frame
    private const int   BossFillEnd      = 9;   // last fill frame (full HP)
    public        float BossScaleX       = 659f;
    public        float BossScaleY       = 68f;

    // ── Internal state ─────────────────────────────────────────────────────
    private bool   initialized     = false;
    private bool   bossSheetLoaded = false;
    private int    lastFrame       = -1;
    private int    lastBlocks      = -1;

    public override void Update()
    {
        if (!initialized)
        {
            initialized = true;
            GetAnimation().SetAnimationSpeed(0f);
        }

        var barTf     = GetTransform();
        var barSprite = GetSprite();

        if (EnemyRhythmController.Instance == null || EnemyRhythmController.Instance.IsDying)
        {
            barTf.IsVisible = false;
            lastFrame       = -1;
            lastBlocks      = -1;
            bossSheetLoaded = false;
            return;
        }

        bool isBoss = EnemyRhythmController.Instance is BossEnemyController;

        // Position follows the enemy (same formula for both boss and regular).
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

        if (isBoss)
            UpdateBossFill(barTf);
        else
            UpdateRegularBar(barTf, barSprite);
    }

    // ── Boss fill chips (frames 1-9, hidden at 0 HP) ──────────────────────
    private void UpdateBossFill(TransformComponent barTf)
    {
        barTf.ScaleX = BossScaleX;
        barTf.ScaleY = BossScaleY;

        if (!bossSheetLoaded)
        {
            GetAnimation().SetAnimation("boss_healthbar", BossSheetRows, BossSheetColumns, BossTotalFrames, 0f);
            bossSheetLoaded = true;
            lastBlocks      = -1;
        }

        float healthPercent = EnemyRhythmController.Instance!.GetHealthPercent();

        // At 0 HP the fill layer hides; the border (BossHPBorderController) remains.
        if (healthPercent <= 0f)
        {
            barTf.IsVisible = false;
            lastFrame = 0;
            return;
        }

        barTf.IsVisible = true;

        // Map health % → fill frame (1 = one chip, 9 = full).
        int targetFrame = (int)Math.Round(healthPercent * BossFillEnd);
        targetFrame = Math.Max(BossFillStart, Math.Min(BossFillEnd, targetFrame));

        if (targetFrame != lastFrame)
        {
            GetAnimation().SetCurrentFrame(targetFrame);
            lastFrame = targetFrame;
            Console.WriteLine($"[BossHPBar Fill] frame {targetFrame} ({healthPercent:P0} HP)");
        }
        else
        {
            // Reset elapsedTime every frame so the zero-speed tick never drifts.
            GetAnimation().SetCurrentFrame(targetFrame);
        }
    }

    // ── Regular enemy: texture-swap (same as EnemyHealthBarController) ────
    private void UpdateRegularBar(TransformComponent barTf, SpriteComponent barSprite)
    {
        barTf.IsVisible = true;
        barTf.ScaleX    = ScaleX;
        barTf.ScaleY    = ScaleY;

        bossSheetLoaded = false;

        int blocks = EnemyRhythmController.Instance!.GetHealthBarBlockCount();
        if (blocks < 2) blocks = 2;
        if (blocks > 4) blocks = 4;

        float healthPercent = EnemyRhythmController.Instance.GetHealthPercent();
        int missingBlocks   = (int)Math.Round((1.0f - healthPercent) * blocks);
        if (missingBlocks < 0)      missingBlocks = 0;
        if (missingBlocks > blocks) missingBlocks = blocks;

        string[] activeFrames = healthFrames4;
        if (blocks == 2) activeFrames = healthFrames2;
        else if (blocks == 3) activeFrames = healthFrames3;

        if (missingBlocks != lastFrame || blocks != lastBlocks)
        {
            // SetAnimation with rows=1, cols=1, totalFrames=1 forces single-image mode.
            // This is required after the boss sheet (rows=3, cols=4) was active, otherwise
            // the engine renders the regular HP texture as a cropped 3x4 grid cell.
            GetAnimation().SetAnimation(activeFrames[missingBlocks], 1, 1, 1, 0f);
            lastFrame  = missingBlocks;
            lastBlocks = blocks;
        }
    }
}
