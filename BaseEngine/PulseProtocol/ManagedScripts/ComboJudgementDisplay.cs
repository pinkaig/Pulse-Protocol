using System;
using ScriptAPI;

public class ComboJudgementDisplay : Script
{
    // Configuration
    public string PerfectSprite = "perfect_ui";
    public string GreatSprite = "great_ui";
    public string MissSprite = "miss_ui";

    public float DisplayDuration = 0.25f;

    // State
    private float displayTimer = 0f;
    private bool isShowing = false;
    private bool hasRegistered = false;

    public override void Update()
    {
        if (hasRegistered && !ComboDisplayManager.IsJudgementRegistered(this))
        {
            hasRegistered = false;
        }

        if (!hasRegistered)
        {
            ComboDisplayManager.RegisterJudgement(this);
            Hide();
            hasRegistered = true;
            Console.WriteLine("[JudgementDisplay] Initialized and hidden");
        }

        if (isShowing)
        {
            displayTimer -= Time.DeltaTime;

            if (displayTimer <= 0)
            {
                Hide();
            }
        }
    }

    public void ShowJudgement(string judgement)
    {
        var sprite = GetSprite();
        if (!sprite.HasSprite())
        {
            Console.WriteLine("[JudgementDisplay] ERROR: No sprite component!");
            return;
        }

        string texturePath = GetSpriteForJudgement(judgement);
        sprite.Texture = texturePath;

        var transform = GetTransform();
        transform.IsVisible = true;

        isShowing = true;
        displayTimer = DisplayDuration;
    }

    public void Hide()
    {
        var transform = GetTransform();
        transform.IsVisible = false;
        isShowing = false;
    }

    private string GetSpriteForJudgement(string judgement)
    {
        switch (judgement.ToUpper())
        {
            case "PERFECT": return PerfectSprite;
            case "GOOD":
            case "GREAT": return GreatSprite;
            case "MISS":
            case "LATE": return MissSprite;
            default: return MissSprite;
        }
    }

    public void OnDestroy()
    {
        ComboDisplayManager.UnregisterJudgement();
    }
}