/******************************************************************************/
/**
* @file        CutscenePanel.cs
* @project     Pulse Protocol
* @author      Carrie Lam (83%)
* @author      Chloe Lau (13%)
* @author      Leu Jun Yong (2%)
* @author      Goh Pin Kai (2%)
* @brief       Controls staggered slide-in and simultaneous fade-out animations
*              for cutscene panels grouped as [3, 3, 1], with shared timing
*              state across all panel instances and space-to-skip support.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/

using System;
using ScriptAPI;

/// <summary>
/// Cutscene Panel Controller (Groups + Staggered)
/// =================================================
/// Attach this script to EVERY panel entity (Panel1 through Panel8).
///
/// Groups: (1+2+3), (4+5+6+7), (8)
///
/// Entry: Panels within a group slide in one by one (staggered).
/// Exit:  All panels in the group fade out together (alpha fade, not slide up).
///
/// POSITIONING:
///   The panel's X and Y are read from the entity's saved transform.
///   Set the transform position in the editor to control where each panel sits.
///
/// Press SPACE or gamepad A to skip. Loads Level1 when done.
/// </summary>
public class CutscenePanel : Script
{
    // =========================
    // Timing Configuration
    // =========================
    public float slideInDuration = 0.8f;     // Time for one panel to slide into view
    public float staggerDelay = 1.5f;        // Delay before the next panel in a group starts sliding in
    public float holdDuration = 1.8f;        // Time group stays fully visible (after all are in)
    public float slideOutDuration = 0.8f;    // Time for the group to fade out together
    public float overlapDuration = 0.6f;     // Overlap between current group exiting and next group entering

    // =========================
    // Position Configuration (per-panel)
    // =========================
    public float centerY = 0f;
    public float offScreenBelow = -900f;
    public float offScreenAbove = 900f;      // Kept for compatibility, not used for exit animation

    // =========================
    // Group Configuration
    // Groups: [3, 3, 1] — 7 panels total
    // =========================
    private static readonly int[] GroupSizes = { 3, 4, 1 };

    private static int TotalGroups => GroupSizes.Length;

    private static int TotalPanels
    {
        get { int n = 0; foreach (int g in GroupSizes) n += g; return n; }
    }

    // =========================
    // Static Shared State
    // =========================
    private static float sharedElapsedTime = 0f;
    private static bool sharedCutsceneFinished = false;
    private static int sharedInitCount = 0;
    private static bool s_waitingForSkipTransition = false;
    private static float s_skipTransitionTimer = 0f;
    private const float SkipTransitionDuration = 0.5f;
    private static bool s_sceneChangeIssued = false;
    private static string s_sceneSessionTag = "";
    private static bool s_waitingForEntryFade = true;

    public static bool IsCutsceneFinished => sharedCutsceneFinished;

    // Seconds left before the cutscene naturally ends (0 once finished or skipped).
    public static float TimeRemaining =>
        sharedCutsceneFinished ? 0f : Math.Max(0f, ComputeTotalDuration() - sharedElapsedTime);

    // Cached timing values (set once by the first panel)
    private static float sSlideInDuration = 0f;
    private static float sStaggerDelay = 0f;
    private static float sHoldDuration = 0f;
    private static float sSlideOutDuration = 0f;
    private static float sOverlapDuration = 0f;
    private static float sOffScreenBelow = 0f;

    // =========================
    // Per-Instance State
    // =========================
    private int myPanelIndex = -1;
    private int myGroupIndex = -1;
    private int myPosInGroup = -1;
    private int myGroupSize = -1;
    private bool initialized = false;
    private bool isTimerOwner = false;
    private float myTargetX = 0f;

    public override void Update()
    {
        NavigationButtons.TryProcessPendingPostLoadFadeIn();

        if (!initialized)
        {
            Initialize();
            return;
        }

        if (WaitForEntryFadeComplete())
        {
            HoldPanelHidden();
            return;
        }

        if (isTimerOwner && s_waitingForSkipTransition)
        {
            s_skipTransitionTimer -= Time.DeltaTime;
            bool timeDone = s_skipTransitionTimer <= 0f;
            bool screenFadeDone = FadeAPI.IsFadeDone();

            if (timeDone)
                FadeAPI.SetAlpha(1.0f);

            if (timeDone && screenFadeDone)
            {
                s_waitingForSkipTransition = false;
                s_skipTransitionTimer = 0f;
                GoToLevel1AfterCutscene();
            }
            return;
        }

        if (sharedCutsceneFinished)
            return;

        if (isTimerOwner)
        {
            InputComponent input = GetInput();
            if (input.IsKeyTriggered(32) || input.IsGamepadButtonTriggered(0))
            {
                Console.WriteLine("[Cutscene] Skipped!");
                BeginSkipTransition();
                return;
            }

            sharedElapsedTime += Time.DeltaTime;

            float totalDuration = ComputeTotalDuration();
            if (sharedElapsedTime >= totalDuration)
            {
                Console.WriteLine("[Cutscene] Complete! Loading Level1...");
                GoToLevel1AfterCutscene();
                return;
            }
        }

        AnimatePanel();
    }

    // =========================
    // Timing Helpers
    // =========================
    // Total time for one group to complete (last panel slides in → all hold → all fade out)
    private static float GroupTotalDuration(int groupIndex)
    {
        int size = GroupSizes[groupIndex];
        return (size - 1) * sStaggerDelay + sSlideInDuration + sHoldDuration + sSlideOutDuration;
    }

    // Absolute start time of a group in the shared timeline
    private static float GroupStartTime(int groupIndex)
    {
        float t = 0f;
        for (int g = 0; g < groupIndex; g++)
            t += GroupTotalDuration(g) - sOverlapDuration;
        return t;
    }

    // Total runtime of the entire cutscene
    private static float ComputeTotalDuration()
    {
        float t = 0f;
        for (int g = 0; g < TotalGroups; g++)
        {
            t += GroupTotalDuration(g);
            if (g < TotalGroups - 1)
                t -= sOverlapDuration;
        }
        return t;
    }

    // =========================
    // Initialization
    // =========================
    private void Initialize()
    {
        string currentScene = Scene.GetCurrentScene();
        bool sceneChanged = !string.Equals(s_sceneSessionTag, currentScene, StringComparison.OrdinalIgnoreCase);
        bool freshStart = sharedInitCount == 0 || sharedInitCount >= TotalPanels;
        if (sceneChanged || freshStart)
        {
            s_sceneSessionTag = currentScene;
            sharedInitCount = 0;
            sharedElapsedTime = 0f;
            sharedCutsceneFinished = false;
            s_waitingForSkipTransition = false;
            s_skipTransitionTimer = 0f;
            s_sceneChangeIssued = false;
            s_waitingForEntryFade = true;
        }

        myPanelIndex = sharedInitCount;
        sharedInitCount++;

        // Determine which group this panel belongs to and its position within that group
        int accumulated = 0;
        for (int g = 0; g < GroupSizes.Length; g++)
        {
            if (myPanelIndex < accumulated + GroupSizes[g])
            {
                myGroupIndex = g;
                myPosInGroup = myPanelIndex - accumulated;
                myGroupSize = GroupSizes[g];
                break;
            }
            accumulated += GroupSizes[g];
        }

        // Read X and Y directly from the saved transform position
        TransformComponent savedTf = GetTransform();
        myTargetX = savedTf.X;
        centerY = savedTf.Y;

        // First panel sets up shared state
        if (myPanelIndex == 0)
        {
            sharedElapsedTime = 0f;
            sharedCutsceneFinished = false;
            isTimerOwner = true;

            sSlideInDuration = slideInDuration;
            sStaggerDelay = staggerDelay;
            sHoldDuration = holdDuration;
            sSlideOutDuration = slideOutDuration;
            sOverlapDuration = overlapDuration;
            sOffScreenBelow = offScreenBelow;

            float totalDuration = ComputeTotalDuration();
            Console.WriteLine($"[Cutscene] Timer owner: Panel 1");
            Console.WriteLine($"[Cutscene] Groups: {TotalGroups} | Total panels: {TotalPanels}");
            Console.WriteLine($"[Cutscene] Stagger: {sStaggerDelay}s | SlideIn: {sSlideInDuration}s | Hold: {sHoldDuration}s | FadeOut: {sSlideOutDuration}s | Overlap: {sOverlapDuration}s");
            Console.WriteLine($"[Cutscene] Total duration: {totalDuration:F1}s");
        }

        // Start off-screen and invisible
        TransformComponent tf = GetTransform();
        tf.X = myTargetX;
        tf.Y = offScreenBelow;
        tf.IsVisible = false;

        Console.WriteLine($"[CutscenePanel] Panel {myPanelIndex + 1} -> Group {myGroupIndex + 1}, pos {myPosInGroup} X={myTargetX}");

        initialized = true;
    }

    private bool WaitForEntryFadeComplete()
    {
        if (!s_waitingForEntryFade)
            return false;

        string currentScene = Scene.GetCurrentScene();
        if (NavigationButtons.IsPostLoadFadePendingForScene(currentScene))
            return true;

        if (!FadeAPI.IsFadeDone())
            return true;

        s_waitingForEntryFade = false;
        return false;
    }

    private void HoldPanelHidden()
    {
        TransformComponent tf = GetTransform();
        SpriteComponent sprite = GetSprite();
        tf.IsVisible = false;
        tf.X = myTargetX;
        tf.Y = sOffScreenBelow != 0f ? sOffScreenBelow : offScreenBelow;
        SetAlpha(sprite, 0f);
    }

    private void GoToLevel1AfterCutscene()
    {
        if (s_sceneChangeIssued)
            return;

        s_sceneChangeIssued = true;
        sharedCutsceneFinished = true;
        NavigationButtons.ShowLevelSelectOverlay = false;
        s_sceneSessionTag = "";

        const string targetScene = "Level1";
        NavigationButtons.currentScene = targetScene;
        NavigationButtons.LastEnteredLevel = targetScene;

        // Always go through the loading screen from the intro cutscene
        LoadingScreenScript.NextLevel = targetScene;
        Scene.LoadScene("LoadingScreenScene");
    }

    private void BeginSkipTransition()
    {
        if (s_waitingForSkipTransition)
            return;

        sharedCutsceneFinished = true;
        s_waitingForSkipTransition = true;
        s_skipTransitionTimer = SkipTransitionDuration;
        FadeAPI.Stop();
        FadeAPI.SetAlpha(0.0f);
        FadeAPI.FadeOut(SkipTransitionDuration);
    }

    // =========================
    // Animation
    // =========================
    private void AnimatePanel()
    {
        if (myPanelIndex < 0 || myGroupIndex < 0) return;

        TransformComponent tf = GetTransform();
        SpriteComponent sprite = GetSprite();

        // Local time = how far into this group's timeline we are
        float groupStart = GroupStartTime(myGroupIndex);
        float localTime = sharedElapsedTime - groupStart;

        // Timeline within a group:
        // mySlideStart = myPosInGroup * staggerDelay
        // holdStart    = (groupSize - 1) * staggerDelay + slideInDuration
        // holdEnd      = holdStart + holdDuration
        // groupEnd     = holdEnd + slideOutDuration

        float mySlideStart = myPosInGroup * sStaggerDelay;
        float mySlideEnd = mySlideStart + sSlideInDuration;
        float holdStart = (myGroupSize - 1) * sStaggerDelay + sSlideInDuration;
        float holdEnd = holdStart + sHoldDuration;
        float groupEnd = holdEnd + sSlideOutDuration;

        // Before this group starts
        if (localTime < 0f)
        {
            tf.IsVisible = false;
            tf.X = myTargetX;
            tf.Y = sOffScreenBelow;
            SetAlpha(sprite, 0f);
            return;
        }

        // After this group is fully done
        if (localTime >= groupEnd)
        {
            tf.IsVisible = false;
            tf.X = myTargetX;
            tf.Y = centerY;
            SetAlpha(sprite, 0f);
            return;
        }

        // Waiting for this panel's stagger slot
        if (localTime < mySlideStart)
        {
            tf.IsVisible = false;
            tf.X = myTargetX;
            tf.Y = sOffScreenBelow;
            SetAlpha(sprite, 0f);
            return;
        }

        // Panel is active from here
        tf.IsVisible = true;
        tf.X = myTargetX;

        if (localTime < mySlideEnd)
        {
            // === SLIDING IN ===
            float t = (localTime - mySlideStart) / sSlideInDuration;
            float eased = EaseOutCubic(t);

            tf.Y = Lerp(sOffScreenBelow, centerY, eased);
            SetAlpha(sprite, Lerp(0.2f, 1f, eased));
        }
        else if (localTime < holdEnd)
        {
            // === HOLDING (all panels in this group are fully visible) ===
            tf.Y = centerY;
            SetAlpha(sprite, 1f);
        }
        else
        {
            // === FADING OUT (alpha fade in place, not slide upward) ===
            float t = (localTime - holdEnd) / sSlideOutDuration;
            float eased = EaseInCubic(t);

            tf.Y = centerY;
            SetAlpha(sprite, Lerp(1f, 0f, eased));
        }
    }

    // =========================
    // Helpers
    // =========================
    private void SetAlpha(SpriteComponent sprite, float alpha)
    {
        if (sprite.HasSprite())
            sprite.TintA = alpha;
    }

    private float Lerp(float a, float b, float t)
    {
        if (t < 0f) t = 0f;
        if (t > 1f) t = 1f;
        return a + (b - a) * t;
    }

    private float EaseOutCubic(float t)
    {
        float f = 1f - t;
        return 1f - f * f * f;
    }

    private float EaseInCubic(float t)
    {
        return t * t * t;
    }

    public void OnDestroy()
    {
        // Reset static state so the next scene load always triggers a full reinitialize.
        s_sceneSessionTag = "";
        sharedInitCount = 0;
        sharedCutsceneFinished = false;
        s_waitingForSkipTransition = false;
        s_sceneChangeIssued = false;
        s_waitingForEntryFade = true;
    }
}
