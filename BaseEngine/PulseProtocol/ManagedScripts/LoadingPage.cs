/******************************************************************************/
/**
 * @file        LoadingPage.cs
 * @project     Pulse Protocol
 * @author      Leu Jun Yong (primary) - 100%
 * @brief       Handles loading screen timing, fade flow, and next-scene handoff.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/

using System;
using ScriptAPI;
// Call loading screen scene and the png will have this script where it just loads the png then loads level 1 after like 3 frames
// Then it will freeze on this scene instead of wtv scene it was at first.

public class LoadingScreenScript : Script
{
    private bool started = false;
    private bool sceneRequested = false;
    private int phase = 0; // 0 = fade in, 1 = hold, 2 = fade out+load request, 3 = done
    private float phaseElapsed = 0.0f;
    private const float LoadingFadeInDuration = 0.5f;
    private const float LoadingFadeOutDuration = 0.5f;
    private const float MinLoadingScreenUptime = 1.0f;
    public static bool PendingLevel1Transition = false;
    public static bool PendingLevel1LoadingOverlay = false;
    public static bool PendingLevel2LoadingOverlay = false;
    public static bool BlockLevelStartCountdown = false;

    // for the next level to be loaded, it is global so before u load the scene, just make delcare this variable.
    public static string NextLevel = ""; 

    public override void Update()
    {
        SpriteComponent sprite = GetSprite();

        if (!started)
        {
            started = true;
            sceneRequested = false;
            phase = 0;
            phaseElapsed = 0.0f;
            sprite.SetTint(1.0f, 1.0f, 1.0f, 1.0f);
            FadeAPI.Stop();
            FadeAPI.SetAlpha(1.0f);
            FadeAPI.FadeIn(LoadingFadeInDuration);
            return;
        }

        if (phase == 3)
            return;

        phaseElapsed += Time.DeltaTime;

        if (phase == 0)
        {
            if (FadeAPI.IsFadeDone())
            {
                phase = 1;
                phaseElapsed = 0.0f;
            }
            return;
        }

        if (phase == 1)
        {
            sprite.SetTint(1.0f, 1.0f, 1.0f, 1.0f);
            if (phaseElapsed >= MinLoadingScreenUptime)
            {
                phase = 2;
                phaseElapsed = 0.0f;
            }
            return;
        }

        if (phase == 2)
        {
            bool needsLevelStyleTransition =
                string.Equals(NextLevel, "Level1", StringComparison.OrdinalIgnoreCase) ||
                string.Equals(NextLevel, "Level2", StringComparison.OrdinalIgnoreCase);

            // For Level1, do not fade here. Let Level1 handle fade-out/fade-in
            // after the engine confirms loading has completed.
            if (!needsLevelStyleTransition && phaseElapsed <= Time.DeltaTime && !sceneRequested)
            {
                FadeAPI.Stop();
                FadeAPI.SetAlpha(0.0f);
                FadeAPI.FadeOut(LoadingFadeOutDuration);
            }

            bool canLoadNow = needsLevelStyleTransition || FadeAPI.IsFadeDone();
            if (!sceneRequested && canLoadNow)
            {
                PendingLevel1Transition = needsLevelStyleTransition;
                PendingLevel1LoadingOverlay = string.Equals(NextLevel, "Level1", StringComparison.OrdinalIgnoreCase);
                PendingLevel2LoadingOverlay = string.Equals(NextLevel, "Level2", StringComparison.OrdinalIgnoreCase);
                BlockLevelStartCountdown = needsLevelStyleTransition;
                sceneRequested = true;
                phase = 3;
                Scene.LoadScene(NextLevel);
            }
        }
    }
}

public class Level1LoadingOverlayTransition : Script
{
    private const float Level1FadeOutDuration = 0.5f;
    private const float Level1FadeInDuration = 0.5f;
    private const float PostLoadSettleDuration = 0.12f;
    private bool started = false;
    private bool finished = false;
    private int phase = 0; // 0 = wait load complete, 1 = post-load settle, 2 = fade out, 3 = fade in
    private float phaseTimer = 0.0f;

    public override void Update()
    {
        if (finished)
            return;

        TransformComponent t = GetTransform();
        SpriteComponent sprite = GetSprite();

        if (!started)
        {
            started = true;
            bool pending = LoadingScreenScript.PendingLevel1LoadingOverlay;
            LoadingScreenScript.PendingLevel1Transition = false;
            LoadingScreenScript.PendingLevel1LoadingOverlay = false;

            // Keep loading BG overlay visible immediately on Level1 start.
            t.IsVisible = pending;
            sprite.SetTint(1.0f, 1.0f, 1.0f, pending ? 1.0f : 0.0f);

            if (!pending)
            {
                LoadingScreenScript.BlockLevelStartCountdown = false;
                finished = true;
                return;
            }
        }

        if (phase == 0)
        {
            if (!Scene.ConsumeSceneLoadedPulse())
                return;

            phaseTimer = 0.0f;
            phase = 1;
            return;
        }

        if (phase == 1)
        {
            phaseTimer += Time.DeltaTime;
            if (phaseTimer < PostLoadSettleDuration)
                return;

            FadeAPI.Stop();
            FadeAPI.SetAlpha(0.0f);
            FadeAPI.FadeOut(Level1FadeOutDuration);
            phase = 2;
            return;
        }

        if (phase == 2)
        {
            if (!FadeAPI.IsFadeDone())
                return;

            // While fully black, hide the loading BG overlay before revealing Level1.
            FadeAPI.SetAlpha(1.0f);
            t.IsVisible = false;
            sprite.SetTint(1.0f, 1.0f, 1.0f, 0.0f);
            FadeAPI.FadeIn(Level1FadeInDuration);
            phase = 3;
            return;
        }

        if (phase == 3 && !FadeAPI.IsFadeDone())
            return;

        FadeAPI.Stop();
        FadeAPI.SetAlpha(0.0f);
        LoadingScreenScript.BlockLevelStartCountdown = false;
        finished = true;
    }
}

public class Level2LoadingOverlayTransition : Script
{
    private const float Level2FadeOutDuration = 0.5f;
    private const float Level2FadeInDuration = 0.5f;
    private const float PostLoadSettleDuration = 0.12f;
    private bool started = false;
    private bool finished = false;
    private int phase = 0; // 0 = wait load complete, 1 = post-load settle, 2 = fade out, 3 = fade in
    private float phaseTimer = 0.0f;

    public override void Update()
    {
        if (finished)
            return;

        TransformComponent t = GetTransform();
        SpriteComponent sprite = GetSprite();

        if (!started)
        {
            started = true;
            bool pending = LoadingScreenScript.PendingLevel2LoadingOverlay;
            LoadingScreenScript.PendingLevel2LoadingOverlay = false;

            t.IsVisible = pending;
            sprite.SetTint(1.0f, 1.0f, 1.0f, pending ? 1.0f : 0.0f);

            if (!pending)
            {
                LoadingScreenScript.BlockLevelStartCountdown = false;
                finished = true;
                return;
            }
        }

        if (phase == 0)
        {
            if (!Scene.ConsumeSceneLoadedPulse())
                return;

            phaseTimer = 0.0f;
            phase = 1;
            return;
        }

        if (phase == 1)
        {
            phaseTimer += Time.DeltaTime;
            if (phaseTimer < PostLoadSettleDuration)
                return;

            FadeAPI.Stop();
            FadeAPI.SetAlpha(0.0f);
            FadeAPI.FadeOut(Level2FadeOutDuration);
            phase = 2;
            return;
        }

        if (phase == 2)
        {
            if (!FadeAPI.IsFadeDone())
                return;

            FadeAPI.SetAlpha(1.0f);
            t.IsVisible = false;
            sprite.SetTint(1.0f, 1.0f, 1.0f, 0.0f);
            FadeAPI.FadeIn(Level2FadeInDuration);
            phase = 3;
            return;
        }

        if (phase == 3 && !FadeAPI.IsFadeDone())
            return;

        FadeAPI.Stop();
        FadeAPI.SetAlpha(0.0f);
        LoadingScreenScript.BlockLevelStartCountdown = false;
        finished = true;
    }
}

