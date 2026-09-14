/******************************************************************************/
/**
 * @file        LevelSelectOverlay.cs
 * @project     Pulse Protocol
 * @author      Leu Jun Yong (primary) - 100%
 * @brief       Controls level-select overlay visibility, selection, and input flow.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/

using ScriptAPI;
using System;

public class LevelSelectOverlay : Script
{
    private const int GP_CIRCLE     = InputConstants.GP_CIRCLE;
    private const int GP_DPAD_RIGHT = InputConstants.GP_DPAD_RIGHT;
    private const int GP_DPAD_LEFT  = InputConstants.GP_DPAD_LEFT;

    public override void Update()
    {
        TransformComponent t = GetTransform();
        bool visible = NavigationButtons.ShowLevelSelectOverlay &&
                       !NavigationButtons.ShowSettingsOverlay &&
                       !NavigationButtons.ShowQuitOverlay &&
                       !NavigationButtons.ShowHowToPlayOverlay;

        t.IsVisible = visible;
        if (!visible)
            return;

        InputComponent input = GetInput();
        if (input.IsGamepadButtonTriggered(GP_CIRCLE))
            NavigationButtons.ShowLevelSelectOverlay = false;
    }
}

public class LevelSelectCard : Script
{
    private static readonly string[] s_sceneOrder = new[] { "Level1", "Level2", "Level3" };
    private static int s_selectedIndex = 0;
    private static bool s_initializedSelection = false;
    private static bool s_clickedThisFrame = false;
    private static int s_clickFrame = -1;
    private static bool s_centerSettled = false;
    private static int s_settleFrame = -1;

    // Gamepad navigation state (shared, managed by myIndex==0 instance)
    private static bool s_gpNavThisFrame = false;
    private static int s_gpNavFrame = -1;
    private static bool s_stickLeftWasActive = false;
    private static bool s_stickRightWasActive = false;

    private const int GP_DPAD_RIGHT = InputConstants.GP_DPAD_RIGHT;
    private const int GP_DPAD_LEFT  = InputConstants.GP_DPAD_LEFT;
    private const float StickDeadzone = 0.5f;

    private bool initialized = false;
    private int myIndex = 0;
    private string levelScene = "Level1";

    public override void Update()
    {
        if (s_clickFrame != Time.FrameCount)
        {
            s_clickFrame = Time.FrameCount;
            s_clickedThisFrame = false;
        }
        if (s_settleFrame != Time.FrameCount)
        {
            s_settleFrame = Time.FrameCount;
            s_centerSettled = false;
        }

        TransformComponent t = GetTransform();
        bool visible = NavigationButtons.ShowLevelSelectOverlay &&
                       !NavigationButtons.ShowSettingsOverlay &&
                       !NavigationButtons.ShowQuitOverlay &&
                       !NavigationButtons.ShowHowToPlayOverlay;

        t.IsVisible = visible;
        if (!visible)
        {
            // Reinitialize selection whenever overlay is opened again.
            s_initializedSelection = false;
            return;
        }

        if (!initialized)
        {
            initialized = true;

            string tex = (GetSprite().Texture ?? "").ToLower();
            if (tex.Contains("level2"))
                levelScene = "Level2";
            else if (tex.Contains("level3") || tex.Contains("level 3"))
                levelScene = "Level3";
            else
                levelScene = "Level1";

            myIndex = GetSceneIndex(levelScene);
        }

        if (!s_initializedSelection)
        {
            s_initializedSelection = true;
            s_selectedIndex = GetSceneIndex(NavigationButtons.SelectedLevelScene);
            if (!IsSceneUnlocked(s_sceneOrder[s_selectedIndex]))
                s_selectedIndex = 0;
            NavigationButtons.SelectedLevelScene = s_sceneOrder[s_selectedIndex];
        }

        InputComponent input = GetInput();

        int rawOffset = myIndex - s_selectedIndex;
        bool withinRange = Math.Abs(rawOffset) <= 1;

        // Target position:
        //   offset  0  → centre  (selected)
        //   offset ±1  → ±920    (adjacent, visible)
        //   offset ±2+ → ±1440   (off-screen, hidden)
        float targetX, targetY, targetScaleX, targetScaleY;
        if      (rawOffset ==  0) { targetX =     0.0f; targetY = 180.0f; targetScaleX = 420.0f; targetScaleY = 420.0f; }
        else if (rawOffset == -1) { targetX =  -920.0f; targetY = 140.0f; targetScaleX = 360.0f; targetScaleY = 360.0f; }
        else if (rawOffset ==  1) { targetX =   920.0f; targetY = 140.0f; targetScaleX = 360.0f; targetScaleY = 360.0f; }
        else if (rawOffset  < -1) { targetX = -1440.0f; targetY = 140.0f; targetScaleX = 360.0f; targetScaleY = 360.0f; }
        else                      { targetX =  1440.0f; targetY = 140.0f; targetScaleX = 360.0f; targetScaleY = 360.0f; }

        // Gamepad navigation — only one instance handles it per frame
        if (s_gpNavFrame != Time.FrameCount)
        {
            s_gpNavFrame = Time.FrameCount;
            s_gpNavThisFrame = false;
        }

        if (!s_gpNavThisFrame && myIndex == 0)
        {
            float stickX = input.GetGamepadAxis(0);
            bool stickLeft  = stickX < -StickDeadzone;
            bool stickRight = stickX >  StickDeadzone;

            bool navLeft  = input.IsGamepadButtonTriggered(GP_DPAD_LEFT)  || (stickLeft  && !s_stickLeftWasActive);
            bool navRight = input.IsGamepadButtonTriggered(GP_DPAD_RIGHT) || (stickRight && !s_stickRightWasActive);

            s_stickLeftWasActive  = stickLeft;
            s_stickRightWasActive = stickRight;

            if (navLeft || navRight)
            {
                s_gpNavThisFrame = true;
                int next = s_selectedIndex + (navRight ? 1 : -1);
                next = Math.Max(0, Math.Min(s_sceneOrder.Length - 1, next));
                if (next != s_selectedIndex)
                {
                    s_selectedIndex = next;
                    NavigationButtons.SelectedLevelScene = s_sceneOrder[s_selectedIndex];
                    AudioComponent audio = GetAudio();
                    audio.PlaySFX("SFX_Button_Hover");
                }
            }
        }

        // Always lerp
        t.IsVisible = withinRange;

        float lerp = Math.Min(1.0f, Time.DeltaTime * 10.0f);
        t.X += (targetX - t.X) * lerp;
        t.Y += (targetY - t.Y) * lerp;
        t.ScaleX += (targetScaleX - t.ScaleX) * lerp;
        t.ScaleY += (targetScaleY - t.ScaleY) * lerp;

        // Skip click detection and settle check for hidden cards.
        if (!withinRange)
            return;

        float halfW = t.ScaleX * 0.5f;
        float halfH = t.ScaleY * 0.5f;
        bool isHoveredNow = input.InGameViewport &&
                         input.WorldMouseX >= t.X - halfW && input.WorldMouseX <= t.X + halfW &&
                         input.WorldMouseY >= t.Y - halfH && input.WorldMouseY <= t.Y + halfH;

        if (!s_clickedThisFrame && isHoveredNow && input.IsMouseButtonTriggered(0))
        {
            s_clickedThisFrame = true;
            // Always allow clicking to scroll the carousel.
            // Locked levels just can't be started (checked in LevelSelectStartDisc).
            s_selectedIndex = myIndex;
            NavigationButtons.SelectedLevelScene = s_sceneOrder[myIndex];
        }

        SpriteComponent sprite = GetSprite();
        if (IsSceneUnlocked(levelScene))
            sprite.SetTint(1.0f, 1.0f, 1.0f, 1.0f);
        else
            sprite.SetTint(0.42f, 0.42f, 0.42f, 1.0f);

        if (rawOffset == 0)
        {
            bool atCenter =
                Math.Abs(t.X - targetX) < 3.0f &&
                Math.Abs(t.Y - targetY) < 3.0f &&
                Math.Abs(t.ScaleX - targetScaleX) < 3.0f &&
                Math.Abs(t.ScaleY - targetScaleY) < 3.0f;

            if (atCenter)
                s_centerSettled = true;
        }
    }

    public static float SelectedX => 0.0f;
    public static float SelectedY => 180.0f;
    public static float SelectedScaleX => 420.0f;
    public static float SelectedScaleY => 420.0f;
    public static bool IsCenterSettled => s_centerSettled;

    private static int GetSceneIndex(string scene)
    {
        for (int i = 0; i < s_sceneOrder.Length; ++i)
        {
            if (string.Equals(s_sceneOrder[i], scene, StringComparison.OrdinalIgnoreCase))
                return i;
        }
        return 0;
    }

    public static bool IsSceneUnlocked(string sceneName)
    {
        int targetLevel = ExtractLevelNumber(sceneName);
        if (targetLevel <= 1)
            return true; // Level1 always unlocked

        int lastClearedLevel = ExtractLevelNumber(NavigationButtons.LastClearedLevel);
        return lastClearedLevel >= (targetLevel - 1);
    }

    private static int ExtractLevelNumber(string sceneName)
    {
        if (string.IsNullOrEmpty(sceneName))
            return 0;

        int i = sceneName.Length - 1;
        while (i >= 0 && char.IsDigit(sceneName[i]))
            i--;

        if (i == sceneName.Length - 1)
            return 0;

        string numberPart = sceneName.Substring(i + 1);
        if (int.TryParse(numberPart, out int parsed))
            return parsed;

        return 0;
    }
}

public class LevelSelectSelector : Script
{
    private const float FadeInDuration = 0.35f;
    private float _alpha = 0.0f;
    private bool _wasVisible = false;

    public override void Update()
    {
        TransformComponent t = GetTransform();
        bool visible = NavigationButtons.ShowLevelSelectOverlay &&
                       !NavigationButtons.ShowSettingsOverlay &&
                       !NavigationButtons.ShowQuitOverlay &&
                       !NavigationButtons.ShowHowToPlayOverlay &&
                       LevelSelectCard.IsCenterSettled;

        if (!visible)
        {
            if (_wasVisible)
            {
                _alpha = 0.0f;
                _wasVisible = false;
                SpriteComponent s = GetSprite();
                s.TintA = 0.0f;
            }
            t.IsVisible = false;
            return;
        }

        // First frame visible: snap position to target, start from alpha 0
        if (!_wasVisible)
        {
            _alpha = 0.0f;
            _wasVisible = true;
            t.X = LevelSelectCard.SelectedX;
            t.Y = LevelSelectCard.SelectedY;
            t.ScaleX = LevelSelectCard.SelectedScaleX * 1.20f;
            t.ScaleY = LevelSelectCard.SelectedScaleY * 1.20f;
        }

        t.IsVisible = true;

        _alpha = Math.Min(1.0f, _alpha + Time.DeltaTime / FadeInDuration);
        SpriteComponent sprite = GetSprite();
        sprite.TintA = _alpha;

        float lerp = Math.Min(1.0f, Time.DeltaTime * 12.0f);
        t.X += (LevelSelectCard.SelectedX - t.X) * lerp;
        t.Y += (LevelSelectCard.SelectedY - t.Y) * lerp;
        t.ScaleX += (LevelSelectCard.SelectedScaleX * 1.20f - t.ScaleX) * lerp;
        t.ScaleY += (LevelSelectCard.SelectedScaleY * 1.20f - t.ScaleY) * lerp;
    }
}

public class LevelSelectStartDisc : Script
{
    private static bool s_waitingForSceneTransition = false;
    private static string s_pendingScene = "";
    private static float s_sceneTransitionTimer = 0.0f;
    private static int s_transitionTickFrame = -1;
    private const float SceneTransitionDuration = 0.5f;

    private bool initialized = false;
    private bool wasStartHovered = false;
    private float baseScaleX;
    private float baseScaleY;
    private const float SpinSpeed = 45.0f;
    private const float StartHitboxScale = 0.22f; // center-focused area around the START text
    private const float StartHoverScaleBoost = 1.03f;

    public override void Update()
    {
        string realScene = Scene.GetCurrentScene();
        if (!string.Equals(realScene, "MainMenu", StringComparison.OrdinalIgnoreCase))
        {
            // Safety reset: this transition state must never leak outside MainMenu.
            s_waitingForSceneTransition = false;
            s_pendingScene = "";
            s_sceneTransitionTimer = 0.0f;
            s_transitionTickFrame = -1;
            return;
        }

        if (s_waitingForSceneTransition)
        {
            if (s_transitionTickFrame != Time.FrameCount)
            {
                s_transitionTickFrame = Time.FrameCount;

                float dt = Time.DeltaTime;
                if (dt <= 0.0f)
                    dt = 0.016f;
                s_sceneTransitionTimer -= dt;
                bool timeDone = s_sceneTransitionTimer <= 0.0f;
                bool screenFadeDone = FadeAPI.IsFadeDone();
                if (timeDone)
                    FadeAPI.SetAlpha(1.0f);
                if (timeDone && screenFadeDone)
                {
                    s_waitingForSceneTransition = false;

                    string sceneToLoad = s_pendingScene;
                    s_pendingScene = "";
                    s_sceneTransitionTimer = 0.0f;
                    s_transitionTickFrame = -1;

                    if (string.IsNullOrEmpty(sceneToLoad) || !Scene.SceneExists(sceneToLoad))
                        sceneToLoad = "Level1";

                    NavigationButtons.ShowLevelSelectOverlay = false;
                    NavigationButtons.currentScene = sceneToLoad;
                    NavigationButtons.LastEnteredLevel = sceneToLoad;
                    if (NavigationButtons.ShouldUseLoadingScreenForScene(sceneToLoad))
                    {
                        LoadingScreenScript.NextLevel = sceneToLoad;
                        Scene.LoadScene("LoadingScreenScene");
                    }
                    else
                    {
                        NavigationButtons.QueuePostLoadFadeIn(sceneToLoad, SceneTransitionDuration);
                        Scene.LoadScene(sceneToLoad);
                    }
                }
            }
            return;
        }

        TransformComponent t = GetTransform();
        bool visible = NavigationButtons.ShowLevelSelectOverlay &&
                       !NavigationButtons.ShowSettingsOverlay &&
                       !NavigationButtons.ShowQuitOverlay &&
                       !NavigationButtons.ShowHowToPlayOverlay;

        t.IsVisible = visible;
        if (!visible)
            return;

        if (!initialized)
        {
            initialized = true;
            baseScaleX = t.ScaleX;
            baseScaleY = t.ScaleY;
        }

        InputComponent input = GetInput();
        float mx = input.WorldMouseX;
        float my = input.WorldMouseY;

        // Only the center START area triggers highlight/click-to-start.
        float startNx = (mx - t.X) / (baseScaleX * StartHitboxScale);
        float startNy = (my - t.Y) / (baseScaleY * StartHitboxScale);
        bool isStartHovered = input.InGameViewport && (startNx * startNx + startNy * startNy <= 1.0f);

        if (isStartHovered && !wasStartHovered)
        {
            AudioComponent audio = GetAudio();
            audio.PlaySFX("SFX_Button_Hover");
        }
        wasStartHovered = isStartHovered;

        // Rotation pause now uses the same center hitbox as hover SFX/scale.
        if (!isStartHovered)
            t.RotX += SpinSpeed * Time.DeltaTime;

        float targetScaleX = isStartHovered ? baseScaleX * StartHoverScaleBoost : baseScaleX;
        float targetScaleY = isStartHovered ? baseScaleY * StartHoverScaleBoost : baseScaleY;
        float scaleLerp = Math.Min(1.0f, Time.DeltaTime * 12.0f);
        t.ScaleX += (targetScaleX - t.ScaleX) * scaleLerp;
        t.ScaleY += (targetScaleY - t.ScaleY) * scaleLerp;

        bool startPressed = (isStartHovered && input.IsMouseButtonTriggered(0))
                         || (LevelSelectCard.IsCenterSettled && input.IsGamepadButtonTriggered(0));

        if (startPressed)
        {
            AudioComponent audio = GetAudio();
            audio.PlaySFX("SFX_Button_Select");

            string sceneToLoad = NavigationButtons.SelectedLevelScene;
            if (!LevelSelectCard.IsSceneUnlocked(sceneToLoad))
                return;

            if (string.IsNullOrEmpty(sceneToLoad) || !Scene.SceneExists(sceneToLoad))
                sceneToLoad = "Level1";

            audio.FadeOutMusic(SceneTransitionDuration);
            NavigationButtons.PendingSceneFadeIn = false;
            FadeAPI.Stop();
            FadeAPI.SetAlpha(0.0f);
            FadeAPI.FadeOut(SceneTransitionDuration);
            s_pendingScene = sceneToLoad;
            s_waitingForSceneTransition = true;
            s_sceneTransitionTimer = SceneTransitionDuration;
            s_transitionTickFrame = -1;
        }
    }
}

public class LevelSelectBackButton : Script
{
    private bool initialized = false;
    private string defaultTexture = "";
    private string hoverTexture = "";
    private bool isHovered = false;

    public override void Update()
    {
        TransformComponent t = GetTransform();
        bool visible = NavigationButtons.ShowLevelSelectOverlay &&
                       !NavigationButtons.ShowSettingsOverlay &&
                       !NavigationButtons.ShowQuitOverlay &&
                       !NavigationButtons.ShowHowToPlayOverlay;
        t.IsVisible = visible;

        if (!visible)
            return;

        SpriteComponent sprite = GetSprite();
        if (!initialized)
        {
            initialized = true;
            defaultTexture = sprite.Texture ?? "Assets/Textures/UI/back.png";

            int dot = defaultTexture.LastIndexOf('.');
            if (dot >= 0)
                hoverTexture = defaultTexture.Substring(0, dot) + "_hover" + defaultTexture.Substring(dot);
            else
                hoverTexture = defaultTexture + "_hover";
        }

        InputComponent input = GetInput();
        float halfW = t.ScaleX * 0.5f;
        float halfH = t.ScaleY * 0.5f;
        bool hoveredNow = input.InGameViewport &&
                          input.WorldMouseX >= t.X - halfW && input.WorldMouseX <= t.X + halfW &&
                          input.WorldMouseY >= t.Y - halfH && input.WorldMouseY <= t.Y + halfH;
        bool clickedNow = hoveredNow && input.IsMouseButtonTriggered(0);

        if (hoveredNow && !isHovered)
        {
            sprite.Texture = hoverTexture;
            isHovered = true;
            if (!clickedNow)
                PlayHoverSound();
        }
        else if (!hoveredNow && isHovered)
        {
            sprite.Texture = defaultTexture;
            isHovered = false;
        }

        if (clickedNow)
        {
            PlayClickSound();
            NavigationButtons.BeginMainMenuOverlayTransitionToBase();
        }
    }

    private void PlayHoverSound()
    {
        AudioComponent audio = GetAudio();
        audio.PlaySFX("SFX_Button_Hover");
    }

    private void PlayClickSound()
    {
        AudioComponent audio = GetAudio();
        audio.PlaySFX("SFX_Button_Select");
    }
}
