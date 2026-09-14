using System;
using ScriptAPI;

/// <summary>
/// Attach to the skip-hint text entity in the Cutscene scene.
/// Fades in a "Press SPACE / × to skip" prompt at the bottom-right of the screen.
/// Switches wording automatically based on whether a gamepad is connected.
/// </summary>
public class EndingCutsceneSkipHint : Script
{
    public float fadeInDelay    = 1.0f;
    public float fadeInDuration = 1.2f;

    private float elapsed = 0f;

    public override void Update()
    {
        if (EndingCutscenePanel.IsCutsceneFinished)
        {
            GetTextComponent().SetVisible(false);
            return;
        }

        elapsed += Time.DeltaTime;

        TextComponentAPI text  = GetTextComponent();
        InputComponent   input = GetInput();

        text.SetText(input.IsGamepadConnected ? "Press CROSS to skip" : "Press SPACE to skip");

        if (elapsed < fadeInDelay)
        {
            text.SetAlpha(0f);
            return;
        }

        float t = (elapsed - fadeInDelay) / fadeInDuration;
        if (t > 1f) t = 1f;

        text.SetAlpha(t * t * (3f - 2f * t));
    }
}

public class CutsceneSkipHint : Script
{
    public float fadeInDelay    = 1.0f;   // seconds before fade starts
    public float fadeInDuration = 1.2f;   // seconds to reach full opacity

    private float elapsed = 0f;

    public override void Update()
    {
        if (CutscenePanel.IsCutsceneFinished)
        {
            GetTextComponent().SetVisible(false);
            return;
        }

        elapsed += Time.DeltaTime;

        TextComponentAPI text  = GetTextComponent();
        InputComponent   input = GetInput();

        // Switch prompt based on active input device
        text.SetText(input.IsGamepadConnected ? "Press CROSS to skip" : "Press SPACE to skip");

        if (elapsed < fadeInDelay)
        {
            text.SetAlpha(0f);
            return;
        }

        float t = (elapsed - fadeInDelay) / fadeInDuration;
        if (t > 1f) t = 1f;

        // Smoothstep easing
        text.SetAlpha(t * t * (3f - 2f * t));
    }
}
