using System;
using ScriptAPI;

public class ScoreManager : Script
{
    public static ScoreManager? Instance { get; private set; }

    // -------------------------------------------------------------------------
    // Tunables
    // -------------------------------------------------------------------------
    public int BaseScore          = 100;
    public int PerfectMultiplier  = 5;     // PERFECT = BaseScore × 5  = 500 base
    public int GreatMultiplier    = 3;     // GREAT   = BaseScore × 3  = 300 base
    public int ComboTierSize      = 10;    // how many combos per tier step

    // -------------------------------------------------------------------------
    // Internal state
    // ------------------------------------------------------------------------- 
    private int totalScore = 0;
    private int combo      = 0;
    private int maxCombo   = 0;

    private int   lastScoreAdded    = 0;
    private float deltaDisplayTimer = 0f;
    private const float DeltaDisplayDuration = 1.5f;

    // Public read-only 
    public int TotalScore => totalScore;
    public int Combo      => combo;
    public int MaxCombo   => maxCombo;

    public override void Update()
    {
        if (Instance != this)
        {
            Instance = this;
            ResetScore();
        }

        if (!GameStart.Ready) return;
        if (Application.IsPaused()) return;

        if (deltaDisplayTimer > 0f)
            deltaDisplayTimer -= Time.DeltaTime;

        // Refresh the on-screen text every frame
        var text = GetTextComponent();
        int tier = combo / Math.Max(1, ComboTierSize) + 1;
        string deltaStr = (deltaDisplayTimer > 0f && lastScoreAdded > 0) ? $" (+{lastScoreAdded:N0})" : "";
        if (combo >= ComboTierSize)
            text.SetText($"Score  {totalScore:N0}{deltaStr}\nCombo  {combo}×   (×{tier})");
        else
            text.SetText($"Score  {totalScore:N0}{deltaStr}\nCombo  {combo}×");
    }

    // Called by ComboSystem on every judgement 
    public void AddScore(string judgement)
    {
        int tier = combo / Math.Max(1, ComboTierSize) + 1;
        int points = 0;

        switch (judgement)
        {
            case "PERFECT":
                points = BaseScore * PerfectMultiplier * tier;
                totalScore += points;
                combo++;
                break;

            case "GREAT":
                points = BaseScore * GreatMultiplier * tier;
                totalScore += points;
                combo++;
                break;

            case "MISS":
                combo = 0;   // break combo; no score added
                break;
        }

        if (combo > maxCombo)
            maxCombo = combo;

        lastScoreAdded = points;
        if (points > 0)
            deltaDisplayTimer = DeltaDisplayDuration;

        Console.WriteLine($"[ScoreManager] {judgement} | +{points} | " +
                          $"Total={totalScore} | Combo={combo} (tier ×{tier}) | MaxCombo={maxCombo}");
    }

    public void RestoreCombo(int value)
    {
        if (value > combo)
        {
            combo = value;
            if (combo > maxCombo) maxCombo = combo;
            Console.WriteLine($"[ScoreManager] Combo restored to {combo} after bar-alignment wait");
        }
    }

    public void ResetScore()
    {
        totalScore       = 0;
        combo            = 0;
        maxCombo         = 0;
        lastScoreAdded   = 0;
        deltaDisplayTimer = 0f;
    }

    public void OnDestroy()
    {
        if (Instance == this) Instance = null;
    }
}
