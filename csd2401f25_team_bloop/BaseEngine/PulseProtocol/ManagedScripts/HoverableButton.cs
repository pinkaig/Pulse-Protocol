/******************************************************************************/
/**
* @file        HoverableButton.cs
* @project     Pulse Protocol
* @author      Chloe Lau Rey En (primary) - 60%
               Leu Jun Yong (secondary) - 30%
               Goh Pin Kai (secondary) - 10%
* @brief       Script for hovering of buttons, and transition of scenes and showing overlay for all 4 buttons
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/

using ScriptAPI;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;

public class NavigationButtons : Script
{
    // === STATIC SCENE HISTORY (shared across all buttons) ===
    private static Stack<string> sceneHistory = new Stack<string>();
    public static string currentScene = "";
    public static string LastGameplayScene = "";
    public static string LastEnteredLevel = ""; // set when entering any level
    public static string LastClearedLevel = ""; // set only when a level is beaten

    // Track if we've set the initial scene THIS play session
    private static int lastFrameCount = -1;

    // === OVERLAY STATE ===
    public static bool ShowQuitOverlay = false;
    public static bool ShowNewGameResetOverlay = false;
    public static bool ShowPauseOverlay = false;
    public static bool ShowHowToPlayOverlay = false;
    public static int HtpPage = 1; // 1, 2, or 3
    private static bool htpOpenedFromPause = false; // true when HTP was opened from the in-game pause overlay
    private static bool openedSettingsFromPause  = false;
    private static bool stageClearPaused = false;
    private static bool s_appliedGamePause = false;

    // === SETTINGS OVERLAY (inside MainMenu scene) ===
    public static bool ShowSettingsOverlay = false;
    public static bool ShowLevelSelectOverlay = false;
    public static string SelectedLevelScene = "Level1";
    public static bool PendingSceneFadeIn = false;

    private string pauseTexture = "";
    private string pauseHoverTexture = "";
    private string defaultTexture = "";
    private string hoverTexture = "";
    private string exitTexture = "";
    private string exitHoverTexture = "";
    private bool isCurrentlyHovered = false;
    private bool buttonInitialized = false;
    private bool lastPauseState = false;
    private bool isPauseButton = false;
    private float hoverScaleBoost = 1.0f;
    private const float ButtonHitboxScaleX = 1.0f;
    private const float ButtonHitboxScaleY = 1.0f;
    private const float ButtonHitboxPadX = 120.0f;
    private const float ButtonHitboxPadY = 30.0f;

    private static bool wasEscPressed = false;
    private static bool wasCircleHandled = false;

    // Gamepad button constants — see InputConstants.cs
    private const int GP_CROSS      = InputConstants.GP_CROSS;
    private const int GP_CIRCLE     = InputConstants.GP_CIRCLE;
    private const int GP_START      = InputConstants.GP_START;
    private const int GP_DPAD_UP    = InputConstants.GP_DPAD_UP;
    private const int GP_DPAD_RIGHT = InputConstants.GP_DPAD_RIGHT;
    private const int GP_DPAD_DOWN  = InputConstants.GP_DPAD_DOWN;
    private const int GP_DPAD_LEFT  = InputConstants.GP_DPAD_LEFT;

    // D-pad navigation state (shared across all button instances)
    private static readonly List<NavigationButtons> s_navCandidates = new List<NavigationButtons>();
    private static readonly List<NavigationButtons> s_prevCandidates = new List<NavigationButtons>();
    private static NavigationButtons? s_focusedButton = null;
    private static int s_navClearFrame   = -1;
    private static int s_navHandledFrame = -1;
    private static bool s_resumeCountdownActive = false;

    private static bool waitingForSceneTransition = false;
    private static string pendingScene = "";
    private static bool pendingClearOverlaysOnSceneSwitch = false;
    private static float sceneTransitionTimer = 0.0f;
    private static int sceneTransitionTickFrame = -1;
    private static bool waitingForPostLoadFadeIn = false;
    private static string pendingPostLoadFadeScene = "";
    private static float pendingPostLoadFadeDuration = 0.0f;
    private static bool forcePostLoadFadeForNextTransition = false;
    private static bool s_initialLevel1LoadingUsed = false;
    private const string RestartSceneSentinel = "__RESTART_CURRENT_SCENE__";
    private static bool pendingStageClearResetOnSceneSwitch = false;
    private const float SceneTransitionDuration = 0.5f;
    private static string s_prevSceneForMainMenuFade = "";
    private static int s_lastMainMenuFadeFrame = -1;
    private static bool waitingForMenuOverlayTransition = false;
    private static float menuOverlayTransitionTimer = 0.0f;
    private static int menuOverlayTransitionTickFrame = -1;
    private static int pendingMenuOverlayTarget = 0; // 1 = LevelSelect, 2 = Settings, 3 = MainMenuBase
    private const float MenuOverlayTransitionDuration = 0.2f;
    private static bool openSettingsOnMainMenuEntry = false;

    // Used by background scripts to avoid one-frame visual gaps when switching
    // from pause overlay to settings overlay in gameplay scenes.
    public static bool IsSettingsOpenedFromPause => openedSettingsFromPause;

    public override void Update()
    {
        // Reset history when a new Play session starts (frame count resets to 0)
        int currentFrame = Time.FrameCount;
        if (currentFrame < lastFrameCount || lastFrameCount < 0)
        {
            sceneHistory.Clear();
            currentScene = "";
            LastGameplayScene = "";
            var (savedCleared, savedEntered) = LevelProgressStore.LoadProgress();
            LastClearedLevel = savedCleared;
            if (!string.IsNullOrWhiteSpace(savedEntered))
                LastEnteredLevel = savedEntered;

            ShowQuitOverlay = false;
            ShowPauseOverlay = false;
            ShowHowToPlayOverlay = false;
            HtpPage = 1;
            htpOpenedFromPause = false;
            ShowSettingsOverlay = false;
            ShowLevelSelectOverlay = false;
            SelectedLevelScene = "Level1";
            PendingSceneFadeIn = false;
            s_prevSceneForMainMenuFade = "";
            s_lastMainMenuFadeFrame = -1;

            wasEscPressed = false;
            s_appliedGamePause = false;

            waitingForSceneTransition = false;
            pendingScene = "";
            pendingClearOverlaysOnSceneSwitch = false;
            sceneTransitionTimer = 0.0f;
            sceneTransitionTickFrame = -1;
            waitingForPostLoadFadeIn = false;
            pendingPostLoadFadeScene = "";
            pendingPostLoadFadeDuration = 0.0f;
            s_initialLevel1LoadingUsed = false;
            pendingStageClearResetOnSceneSwitch = false;
            waitingForMenuOverlayTransition = false;
            menuOverlayTransitionTimer = 0.0f;
            menuOverlayTransitionTickFrame = -1;
            pendingMenuOverlayTarget = 0;
            openSettingsOnMainMenuEntry = false;

            s_navCandidates.Clear();
            s_prevCandidates.Clear();
            s_focusedButton    = null;
            s_navClearFrame    = -1;
            s_navHandledFrame  = -1;
        }

        if (currentFrame != lastFrameCount)
        {
            wasEscPressed = false;
            wasCircleHandled = false;
        }

        lastFrameCount = currentFrame;

        InputComponent input = GetInput();
        TransformComponent t = GetTransform();

        string realScene = Scene.GetCurrentScene();
        TryProcessPendingPostLoadFadeIn();

        // Stage-clear must hard-lock gameplay pause regardless of which button updates first.
        if (StageClearElement.IsCleared)
        {
            stageClearPaused = true;
            if (!Application.IsPaused())
                Application.SetPaused(true);
        }

        // Detect scene entry and keep compatibility with pending flag flow.
        if (!string.IsNullOrEmpty(realScene) && realScene != s_prevSceneForMainMenuFade)
        {
            if (realScene == "MainMenu")
            {
                // Scene-entry safety reset: prevent stale overlays from flashing
                // when MainMenu has just loaded and is about to fade in.
                ShowQuitOverlay = false;
                ShowNewGameResetOverlay = false;
                ShowPauseOverlay = false;
                ShowHowToPlayOverlay = false;
                htpOpenedFromPause = false;
                ShowSettingsOverlay = openSettingsOnMainMenuEntry;
                ShowLevelSelectOverlay = false;
                openSettingsOnMainMenuEntry = false;
            }

            s_prevSceneForMainMenuFade = realScene;
        }

        if (PendingSceneFadeIn &&
            realScene == "MainMenu" &&
            !waitingForSceneTransition &&
            !waitingForMenuOverlayTransition &&
            s_lastMainMenuFadeFrame != Time.FrameCount)
        {
            FadeAPI.Stop();
            FadeAPI.SetAlpha(1.0f);
            FadeAPI.FadeIn(SceneTransitionDuration);
            s_lastMainMenuFadeFrame = Time.FrameCount;
            PendingSceneFadeIn = false;
        }

        if (waitingForSceneTransition)
        {
            if (sceneTransitionTickFrame != Time.FrameCount)
            {
                sceneTransitionTickFrame = Time.FrameCount;

                float dt = Time.DeltaTime;
                if (dt <= 0.0f)
                    dt = 0.016f; // keep transition progressing even while gameplay is paused
                sceneTransitionTimer -= dt;
                bool timeDone = sceneTransitionTimer <= 0.0f;
                bool screenFadeDone = FadeAPI.IsFadeDone();
                if (screenFadeDone || timeDone)
                    FadeAPI.SetAlpha(1.0f);
                if (timeDone && screenFadeDone)
                {
                    // Keep the screen fully dark right before scene switch to avoid one-frame flashes.
                    waitingForSceneTransition = false;

                    string targetScene = pendingScene;
                    pendingScene = "";
                    bool clearOverlays = pendingClearOverlaysOnSceneSwitch;
                    pendingClearOverlaysOnSceneSwitch = false;
                    sceneTransitionTimer = 0.0f;
                    sceneTransitionTickFrame = -1;

                    if (clearOverlays)
                    {
                        ShowPauseOverlay = false;
                        ShowQuitOverlay = false;
                        ShowNewGameResetOverlay = false;
                        ShowHowToPlayOverlay = false;
                        htpOpenedFromPause = false;
                        ShowSettingsOverlay = false;
                        ShowLevelSelectOverlay = false;
                    }

                    if (pendingStageClearResetOnSceneSwitch)
                    {
                        StageClearElement.ResetStageClear();
                        pendingStageClearResetOnSceneSwitch = false;
                    }

                    if (!string.IsNullOrEmpty(targetScene))
                    {
                        if (string.Equals(targetScene, RestartSceneSentinel, StringComparison.Ordinal))
                        {
                            Application.SetPaused(false);
                            FadeAPI.SetAlpha(1.0f);
                            Scene.RestartScene();
                            return;
                        }

                        bool usePostLoadFade = forcePostLoadFadeForNextTransition || ShouldUsePostLoadFadeTransition(targetScene);
                        forcePostLoadFadeForNextTransition = false;
                        if (usePostLoadFade)
                        {
                            waitingForPostLoadFadeIn = true;
                            pendingPostLoadFadeScene = targetScene;
                            pendingPostLoadFadeDuration = SceneTransitionDuration;
                            PendingSceneFadeIn = false;
                        }
                        else
                        {
                            waitingForPostLoadFadeIn = false;
                            pendingPostLoadFadeScene = "";
                            pendingPostLoadFadeDuration = 0.0f;
                        }

                        // Resume only at the handoff moment so gameplay does not continue during fade.
                        Application.SetPaused(false);
                        FadeAPI.SetAlpha(1.0f); // stay black while next scene loads
                        NavigateTo(targetScene);
                    }
                }
            }

            return;
        }

        if (waitingForMenuOverlayTransition)
        {
            if (menuOverlayTransitionTickFrame != Time.FrameCount)
            {
                menuOverlayTransitionTickFrame = Time.FrameCount;

                menuOverlayTransitionTimer -= Time.DeltaTime;
                bool timeDone = menuOverlayTransitionTimer <= 0.0f;
                bool screenFadeDone = FadeAPI.IsFadeDone();
                if (timeDone)
                    FadeAPI.SetAlpha(1.0f);

                if (timeDone && screenFadeDone)
                {
                    ApplyMenuOverlayTransitionTarget();
                    FadeAPI.FadeIn(MenuOverlayTransitionDuration);
                    waitingForMenuOverlayTransition = false;
                    menuOverlayTransitionTimer = 0.0f;
                    menuOverlayTransitionTickFrame = -1;
                    pendingMenuOverlayTarget = 0;
                }
            }
            return;
        }

        // Keep tracked scene in sync with the actual loaded scene (fixes Cutscene -> Level1 issue)
        if (!string.IsNullOrEmpty(realScene) && currentScene != realScene)
        {
            currentScene = realScene;
        }

        // ESC or gamepad Options button → pause toggle (only in gameplay scenes)
        // Both are guarded by wasEscPressed so only the first HoverableButton instance
        // to run this frame processes the toggle (prevents even-count cancellation).
        bool pauseTriggered = (input.IsKeyTriggered(256) || input.IsGamepadButtonTriggered(GP_START))
                           && !wasEscPressed;
        if (pauseTriggered)
        {
            wasEscPressed = true;

            if (!openedSettingsFromPause &&
                !ShowHowToPlayOverlay &&
                !StageClearElement.IsCleared &&
                !realScene.Equals("GameLose") &&
                !string.IsNullOrEmpty(realScene) &&
                !realScene.Equals("MainMenu") &&
                !realScene.Equals("Settings") &&
                !realScene.Equals("Cutscene") &&
                !realScene.Equals("HowToPlay"))
            {
                ShowPauseOverlay = !ShowPauseOverlay;
            }
        }

        // Gamepad Circle → go back / close settings / resume from pause.
        if (!wasCircleHandled && input.IsGamepadButtonTriggered(GP_CIRCLE))
        {
            if (ShowSettingsOverlay || realScene == "Settings")
            {
                // If the screen mode dropdown is open, Circle belongs to it (closes dropdown only)
                if (DropdownState.ScreenDropdownOpen)
                {
                    wasCircleHandled = true;
                    return;
                }
                wasCircleHandled = true;
                AudioSliderKnobBase.ClearFocus();
                GoBack();
                return;
            }
            else if (ShowPauseOverlay)
            {
                wasCircleHandled = true;
                ShowPauseOverlay = false;
                s_resumeCountdownActive = true;
                PauseResumeCountdown? countdown = PauseResumeCountdown.Instance;
                if (countdown != null)
                    countdown.StartCountdown();
                else
                {
                    s_resumeCountdownActive = false;
                    Application.SetPaused(false);
                }
                return;
            }
        }

        // --- Always initialize first (even if currently invisible),
        // so buttons can be hidden then shown again safely.
        if (!buttonInitialized)
        {
            SpriteComponent sprite = GetSprite();
            defaultTexture = sprite.Texture ?? "";
            pauseTexture = defaultTexture;

            if (string.IsNullOrEmpty(currentScene))
            {
                // Prefer engine scene name; fall back to MainMenu if empty
                string sc = Scene.GetCurrentScene();
                currentScene = string.IsNullOrEmpty(sc) ? "MainMenu" : sc;
            }

            int dotIndex = defaultTexture.LastIndexOf('.');
            if (dotIndex >= 0)
                hoverTexture = defaultTexture.Substring(0, dotIndex) + "_hover" + defaultTexture.Substring(dotIndex);
            else
                hoverTexture = defaultTexture + "_hover";

            pauseHoverTexture = hoverTexture;
            isPauseButton = defaultTexture.ToLower().Contains("pause");

            buttonInitialized = true;

            if (defaultTexture.ToLower().Contains("pause"))
            {
                exitTexture = defaultTexture.Replace("Pause", "Exit");
                exitHoverTexture = pauseHoverTexture.Replace("Pause", "Exit");
            }
        }

        // =========================================================
        // SETTINGS OVERLAY VISIBILITY GATING (MainMenu buttons)
        // =========================================================
        // This is what actually hides "other stuff" in MainMenu.
        // It runs BEFORE the early return on invisibility.
        if (realScene == "MainMenu")
        {
            SpriteComponent sp = GetSprite();
            string texNow = (sp.Texture ?? "").ToLower();

            bool isBackBtn = texNow.Contains("back");

            bool isMainMenuBtn =
                texNow.Contains("new game") || texNow.Contains("new_game") ||
                texNow.Contains("continue") ||
                // settings main menu button (avoid pause settings)
                (texNow.Contains("settings") && !texNow.Contains("pausesettings")) ||
                // quit main menu button (avoid quit yes/no and pausequit)
                (texNow.Contains("quit") && !texNow.Contains("quit yes") && !texNow.Contains("quit no") && !texNow.Contains("pausequit")) ||
                texNow.Contains("how to play") || texNow.Contains("how_to_play");

            bool shouldBeVisible = t.IsVisible;

            if (ShowSettingsOverlay)
            {
                if (isBackBtn) shouldBeVisible = true;
                else if (isMainMenuBtn) shouldBeVisible = false;
            }
            else if (ShowLevelSelectOverlay)
            {
                if (isBackBtn) shouldBeVisible = false;
                else if (isMainMenuBtn) shouldBeVisible = false;
            }
            else
            {
                if (isBackBtn) shouldBeVisible = false;
                else if (isMainMenuBtn) shouldBeVisible = true;
            }

            if (t.IsVisible != shouldBeVisible)
            {
                // If we are hiding while hovered, reset sprite/scale cleanly
                if (!shouldBeVisible && isCurrentlyHovered)
                {
                    SpriteComponent sprite = GetSprite();
                    sprite.Texture = defaultTexture;

                    t.ScaleX = t.ScaleX / hoverScaleBoost;
                    t.ScaleY = t.ScaleY / hoverScaleBoost;

                    isCurrentlyHovered = false;
                }

                t.IsVisible = shouldBeVisible;
            }
        }

        // If invisible after gating, do nothing further
        if (!t.IsVisible)
            return;

        // MainMenu Continue lock: grey out + fully non-interactable until progress exists.
        bool isMainMenuContinueButton =
            realScene == "MainMenu" &&
            defaultTexture.ToLower().Contains("continue");
        bool continueUnlocked = HasProgressCache();
        if (isMainMenuContinueButton && !continueUnlocked)
        {
            SpriteComponent continueSprite = GetSprite();
            continueSprite.SetTint(0.45f, 0.45f, 0.45f, 1.0f);

            if (isCurrentlyHovered)
            {
                continueSprite.Texture = defaultTexture;
                t.ScaleX = t.ScaleX / hoverScaleBoost;
                t.ScaleY = t.ScaleY / hoverScaleBoost;
                isCurrentlyHovered = false;
            }

            if (s_focusedButton == this)
                s_focusedButton = null;

            return;
        }
        else if (isMainMenuContinueButton)
        {
            GetSprite().SetTint(1.0f, 1.0f, 1.0f, 1.0f);
        }

        // =========================================================
        // Pause button swaps to Exit while pause overlay is shown
        // =========================================================
        if (isPauseButton)
        {
            bool isPauseMenuOpen = ShowPauseOverlay;
            bool isPauseSettingsOpen = ShowSettingsOverlay && openedSettingsFromPause;
            bool shouldGameBePaused = isPauseMenuOpen || isPauseSettingsOpen || stageClearPaused
                || s_resumeCountdownActive;

            if (isPauseMenuOpen != lastPauseState)
            {
                SpriteComponent sprite = GetSprite();

                if (isPauseMenuOpen)
                {
                    sprite.Texture = exitTexture;
                    defaultTexture = exitTexture;
                    hoverTexture = exitHoverTexture;
                }
                else
                {
                    sprite.Texture = pauseTexture;
                    defaultTexture = pauseTexture;
                    hoverTexture = pauseHoverTexture;
                }

                lastPauseState = isPauseMenuOpen;
            }

            // Apply actual pause state once
            if (shouldGameBePaused != s_appliedGamePause)
            {
                Application.SetPaused(shouldGameBePaused);

                AudioComponent audio = GetAudio();

                if (shouldGameBePaused)
                {
                    // Keep win BGM playing during stage clear; pause BGM only for normal pause flows.
                    if (!stageClearPaused)
                        audio.PauseBGM();
                }
                else
                {
                    audio.FadeInMusic(0.5f);
                }

                s_appliedGamePause = shouldGameBePaused;
            }
        }

        string texLower = defaultTexture.ToLower();
        bool isQuitOverlayButton = texLower.Contains("quit yes") || texLower.Contains("quit no");
        bool isPauseOverlayButton = texLower.Contains("pause") || texLower.Contains("exit") || texLower.Contains("how to play");
        
        bool isHowToPlayOverlayButton = texLower.Contains("temp_close") || texLower.Contains("close_htp") ||
            texLower.Contains("exit") || texLower.Contains("nextstage");

        bool isOverlayButton = isQuitOverlayButton || isPauseOverlayButton;

        if ((ShowQuitOverlay || ShowNewGameResetOverlay || ShowPauseOverlay) && !isOverlayButton && !isHowToPlayOverlayButton)
        {
            if (isCurrentlyHovered)
            {
                SpriteComponent sprite = GetSprite();
                TransformComponent transform = GetTransform();
                sprite.Texture = defaultTexture;
                transform.ScaleX = t.ScaleX / hoverScaleBoost;
                transform.ScaleY = t.ScaleY / hoverScaleBoost;
                isCurrentlyHovered = false;
            }
            return;
        }

        if ((ShowQuitOverlay || ShowNewGameResetOverlay) && isPauseOverlayButton && !isQuitOverlayButton) return;
        if (ShowPauseOverlay && isQuitOverlayButton && !isPauseOverlayButton) return;
        if (ShowHowToPlayOverlay && (!isHowToPlayOverlayButton || isPauseButton)) return;

        if (stageClearPaused && isPauseButton)
        {
            if (isCurrentlyHovered)
            {
                SpriteComponent sprite = GetSprite();
                TransformComponent transform = GetTransform();
                sprite.Texture = defaultTexture;
                transform.ScaleX = t.ScaleX / hoverScaleBoost;
                transform.ScaleY = t.ScaleY / hoverScaleBoost;
                isCurrentlyHovered = false;
            }
            return;
        }

        float currentScaleX = t.ScaleX;
        float currentScaleY = t.ScaleY;
        float baseScaleX = isCurrentlyHovered ? currentScaleX / hoverScaleBoost : currentScaleX;
        float baseScaleY = isCurrentlyHovered ? currentScaleY / hoverScaleBoost : currentScaleY;

        float halfW = (baseScaleX * ButtonHitboxScaleX * 0.5f) + ButtonHitboxPadX;
        float halfH = (baseScaleY * ButtonHitboxScaleY * 0.5f) + ButtonHitboxPadY;
        float mx = input.WorldMouseX;
        float my = input.WorldMouseY;

        // --- Register as D-pad nav candidate (visible, passed all overlay filters) ---
        if (s_navClearFrame != currentFrame)
        {
            s_prevCandidates.Clear();
            s_prevCandidates.AddRange(s_navCandidates);
            s_navCandidates.Clear();
            s_navClearFrame = currentFrame;
        }
        // Skip buttons that are hidden via scale=0 (e.g. dropdown options use this trick
        // instead of IsVisible=false, so we must check scale to avoid ghost candidates).
        if (t.ScaleX > 0.5f && t.ScaleY > 0.5f)
            s_navCandidates.Add(this);

        // --- Handle D-pad navigation once per frame, using previous frame's candidates ---
        if (s_navHandledFrame != currentFrame)
        {
            s_navHandledFrame = currentFrame;

            // D-pad navigation only active in menus / overlays — not during gameplay,
            // to prevent D-pad game inputs from accidentally focusing UI buttons.
            bool inMenuOrOverlay = realScene == "MainMenu" || realScene == "Settings" ||
                                   realScene == "HowToPlay" ||
                                   realScene == "LatencyCalibration" ||
                                   realScene == "VisualLatencyCalibration" ||
                                   realScene == "GameLose" ||
                                   ShowPauseOverlay || ShowQuitOverlay ||
                                   ShowSettingsOverlay || ShowHowToPlayOverlay ||
                                   stageClearPaused;

            // Clear any lingering focus when returning to gameplay
            if (!inMenuOrOverlay && s_focusedButton != null)
                s_focusedButton = null;

            // Scenes/overlays with side-by-side horizontal buttons use Left/Right to navigate.
            // All other menus use Up/Down. Left/Right is reserved for slider value adjustment.
            bool _noSliderOrDropdown = !AudioSliderKnobBase.IsAnySliderFocused && !DropdownState.IsDropdownFocused;
            bool _useHorizontalNav = ShowQuitOverlay
                                     || ShowNewGameResetOverlay
                                     || ShowPauseOverlay
                                     || stageClearPaused
                                     || realScene == "LatencyCalibration"
                                     || realScene == "VisualLatencyCalibration"
                                     || realScene == "GameLose";
            bool navFwd, navBack;
            if (_useHorizontalNav)
            {
                navFwd  = inMenuOrOverlay && _noSliderOrDropdown && input.IsGamepadButtonTriggered(GP_DPAD_RIGHT);
                navBack = inMenuOrOverlay && _noSliderOrDropdown && input.IsGamepadButtonTriggered(GP_DPAD_LEFT);
            }
            else
            {
                navFwd  = inMenuOrOverlay && _noSliderOrDropdown && input.IsGamepadButtonTriggered(GP_DPAD_DOWN);
                navBack = inMenuOrOverlay && _noSliderOrDropdown && input.IsGamepadButtonTriggered(GP_DPAD_UP);
            }
            if (navFwd || navBack)
            {
                int count = s_prevCandidates.Count;
                if (count > 0)
                {
                    int idx = s_focusedButton != null ? s_prevCandidates.IndexOf(s_focusedButton) : -1;
                    if (idx < 0) idx = navFwd ? -1 : 0;
                    int nextIdx = navFwd ? (idx + 1) : (idx - 1 + count) % count;
                    bool inSettings = ShowSettingsOverlay || realScene == "Settings";
                    if (navFwd && nextIdx >= count && inSettings)
                    {
                        // Past the last button in settings → hand focus to the screen mode dropdown
                        s_focusedButton = null;
                        DropdownState.IsDropdownFocused = true;
                    }
                    else
                    {
                        s_focusedButton = s_prevCandidates[nextIdx % count];
                    }
                }
            }

            // When a slider or the dropdown has gamepad focus, clear button focus so the
            // back arrow doesn't stay highlighted and Cross can't fire on a stale button.
            if ((AudioSliderKnobBase.IsAnySliderFocused || DropdownState.IsDropdownFocused) && s_focusedButton != null)
                s_focusedButton = null;
        }

        // Hover: mouse bounds check OR gamepad D-pad focus
        bool isMouseHovered = input.InGameViewport && mx >= t.X - halfW && mx <= t.X + halfW && my >= t.Y - halfH && my <= t.Y + halfH;
        bool isGamepadFocused = (this == s_focusedButton);

        // Mouse movement releases gamepad focus so the two modes don't fight
        if (isMouseHovered && !isGamepadFocused && s_focusedButton != null)
            s_focusedButton = null;

        bool isHovered = isMouseHovered || isGamepadFocused;

        if (isHovered && !isCurrentlyHovered)
        {
            SpriteComponent sprite = GetSprite();
            TransformComponent transform = GetTransform();
            sprite.Texture = hoverTexture;
            transform.ScaleX = currentScaleX * hoverScaleBoost;
            transform.ScaleY = currentScaleY * hoverScaleBoost;
            isCurrentlyHovered = true;

            PlayHoverSound();
        }
        else if (!isHovered && isCurrentlyHovered)
        {
            SpriteComponent sprite = GetSprite();
            TransformComponent transform = GetTransform();
            sprite.Texture = defaultTexture;
            transform.ScaleX = currentScaleX / hoverScaleBoost;
            transform.ScaleY = currentScaleY / hoverScaleBoost;
            isCurrentlyHovered = false;
        }

        // Click: left mouse button OR gamepad Cross button while hovering.
        // Cross is blocked when the screen mode dropdown has gamepad focus (DropdownToggle handles it).
        if (isHovered && (input.IsMouseButtonTriggered(0) || (!DropdownState.IsDropdownFocused && input.IsGamepadButtonTriggered(GP_CROSS))))
        {
            PlayClickSound();
            OnClick();
        }
    }

    private void NavigateTo(string newScene)
    {
        // Leaving MainMenu? don't carry Settings overlay across scenes
        if (newScene != "MainMenu")
        {
            ShowSettingsOverlay = false;
            ShowLevelSelectOverlay = false;
        }

        if (!string.IsNullOrEmpty(currentScene))
        {
            sceneHistory.Push(currentScene);
        }  

        if (newScene.ToLower().Contains("level"))
        {
            LastEnteredLevel = newScene;
            LevelProgressStore.SaveLastEnteredLevel(newScene);
        }

        currentScene = newScene;
        Scene.LoadScene(newScene);
    }

    private void GoBack()
    {
        // If settings overlay is open, back should just close it
        if (ShowSettingsOverlay)
        {
            HideSettings();
            return;
        }

        if (sceneHistory.Count > 0)
        {
            currentScene = sceneHistory.Pop();
            Scene.LoadScene(currentScene);
        }
        else
        {
            currentScene = "MainMenu";
            Scene.LoadScene("MainMenu");
        }
    }

    private void GoToMainMenu()
    {
        sceneHistory.Clear();
        currentScene = "MainMenu";
        Application.SetPaused(false);
        BeginSceneTransition("MainMenu", true, true);
    }

    private void ResetImmediateNoFadeState()
    {
        waitingForSceneTransition = false;
        pendingScene = "";
        pendingClearOverlaysOnSceneSwitch = false;
        sceneTransitionTimer = 0.0f;
        sceneTransitionTickFrame = -1;
        waitingForPostLoadFadeIn = false;
        pendingPostLoadFadeScene = "";
        pendingPostLoadFadeDuration = 0.0f;
        forcePostLoadFadeForNextTransition = false;
        waitingForMenuOverlayTransition = false;
        menuOverlayTransitionTimer = 0.0f;
        menuOverlayTransitionTickFrame = -1;
        pendingMenuOverlayTarget = 0;
        pendingStageClearResetOnSceneSwitch = false;
        PendingSceneFadeIn = false;
        LoadingScreenScript.PendingLevel1Transition = false;
        LoadingScreenScript.PendingLevel1LoadingOverlay = false;
        LoadingScreenScript.PendingLevel2LoadingOverlay = false;
        LoadingScreenScript.BlockLevelStartCountdown = false;

        ShowQuitOverlay = false;
        ShowNewGameResetOverlay = false;
        ShowPauseOverlay = false;
        ShowHowToPlayOverlay = false;
        htpOpenedFromPause = false;
        ShowSettingsOverlay = false;
        ShowLevelSelectOverlay = false;
        openedSettingsFromPause = false;
        Application.SetPaused(false);
        FadeAPI.Stop();
        FadeAPI.SetAlpha(0.0f);
    }

    private void LoadSceneImmediateNoFade(string targetScene, bool clearHistory)
    {
        if (string.IsNullOrEmpty(targetScene))
            return;

        ResetImmediateNoFadeState();

        if (clearHistory)
            sceneHistory.Clear();

        if (targetScene.StartsWith("Level", StringComparison.OrdinalIgnoreCase))
        {
            LastEnteredLevel = targetScene;
            LevelProgressStore.SaveLastEnteredLevel(targetScene);
        }

        currentScene = targetScene;
        Scene.LoadScene(targetScene);
    }

    private void RestartSceneImmediateNoFade()
    {
        ResetImmediateNoFadeState();
        Scene.RestartScene();
    }

    private void BeginSceneTransition(string targetScene, bool fadeInOnMainMenu, bool clearOverlaysAfterFade)
    {
        if (waitingForSceneTransition || string.IsNullOrEmpty(targetScene))
            return;

        // Clear stale post-load requests from earlier transitions.
        waitingForPostLoadFadeIn = false;
        pendingPostLoadFadeScene = "";
        pendingPostLoadFadeDuration = 0.0f;

        if (fadeInOnMainMenu && targetScene == "MainMenu")
            PendingSceneFadeIn = true;
        else
            PendingSceneFadeIn = false;

        GetAudio().FadeOutMusic(SceneTransitionDuration);
        // Keep gameplay frozen for the entire fade transition.
        Application.SetPaused(true);
        FadeAPI.Stop();
        FadeAPI.SetAlpha(0.0f);
        FadeAPI.FadeOut(SceneTransitionDuration);

        waitingForSceneTransition = true;
        pendingScene = targetScene;
        pendingClearOverlaysOnSceneSwitch = clearOverlaysAfterFade;
        sceneTransitionTimer = SceneTransitionDuration;
        sceneTransitionTickFrame = -1;
    }

    private static bool ShouldUsePostLoadFadeTransition(string targetScene)
    {
        if (string.IsNullOrEmpty(targetScene))
            return false;

        if (string.Equals(targetScene, "LoadingScreenScene", StringComparison.OrdinalIgnoreCase))
            return false; // level flow owns its own loading-screen transition

        if (string.Equals(targetScene, "LatencyCalibration", StringComparison.OrdinalIgnoreCase) ||
            string.Equals(targetScene, "VisualLatencyCalibration", StringComparison.OrdinalIgnoreCase))
            return false; // keep calibration pages on their existing transition behavior

        return !targetScene.StartsWith("Level", StringComparison.OrdinalIgnoreCase);
    }

    public static bool ShouldUseLoadingScreenForScene(string targetScene)
    {
        if (string.IsNullOrEmpty(targetScene))
            return false;

        if (!string.Equals(targetScene, "Level1", StringComparison.OrdinalIgnoreCase))
            return false;

        if (s_initialLevel1LoadingUsed)
            return false;

        s_initialLevel1LoadingUsed = true;
        return true;
    }

    public static bool IsPostLoadFadePendingForScene(string sceneName)
    {
        if (!waitingForPostLoadFadeIn || string.IsNullOrEmpty(sceneName))
            return false;

        return string.Equals(pendingPostLoadFadeScene, sceneName, StringComparison.OrdinalIgnoreCase);
    }

    public static void QueuePostLoadFadeIn(string sceneName, float duration)
    {
        if (string.IsNullOrEmpty(sceneName))
            return;

        waitingForPostLoadFadeIn = true;
        pendingPostLoadFadeScene = sceneName;
        pendingPostLoadFadeDuration = duration > 0.0f ? duration : SceneTransitionDuration;
        PendingSceneFadeIn = false;
    }

    public static void TryProcessPendingPostLoadFadeIn()
    {
        if (waitingForSceneTransition || waitingForMenuOverlayTransition)
            return;

        if (!waitingForPostLoadFadeIn)
            return;

        string currentScene = Scene.GetCurrentScene();
        if (string.IsNullOrEmpty(currentScene) ||
            !string.Equals(currentScene, pendingPostLoadFadeScene, StringComparison.OrdinalIgnoreCase))
            return;

        if (!Scene.ConsumeSceneLoadedPulse())
            return;

        FadeAPI.Stop();
        FadeAPI.SetAlpha(1.0f);
        float fadeDuration = pendingPostLoadFadeDuration > 0.0f ? pendingPostLoadFadeDuration : SceneTransitionDuration;
        FadeAPI.FadeIn(fadeDuration);

        waitingForPostLoadFadeIn = false;
        pendingPostLoadFadeScene = "";
        pendingPostLoadFadeDuration = 0.0f;
    }

    private void PlayHoverSound()
    {
        string texLower = defaultTexture.ToLower();
        bool isQuitOverlayButton = texLower.Contains("quit yes") || texLower.Contains("quit no");
        bool isPauseOverlayButton = texLower.Contains("pause") || texLower.Contains("exit") ||
                                    texLower.Contains("pausequit") || texLower.Contains("pauserestart") ||
                                    texLower.Contains("pausesettings");

        if ((ShowQuitOverlay || ShowNewGameResetOverlay) && !isQuitOverlayButton)
            return;

        if (ShowPauseOverlay && !isPauseOverlayButton)
            return;

        AudioComponent audio = GetAudio();
        audio.PlaySFX("SFX_Button_Hover");
    }

    private void PlayClickSound()
    {
        AudioComponent audio = GetAudio();
        audio.PlaySFX("SFX_Button_Select");
    }

    public static void GoToGameLose()
    {
        ShowSettingsOverlay = false;

        if (!string.IsNullOrEmpty(currentScene))
            LastGameplayScene = currentScene;
        else
            LastGameplayScene = "MainMenu";

        currentScene = "GameLose";
        Scene.LoadScene("GameLose");
    }

    // === The 2 functions you asked for (now used properly) ===
    private static void ShowSettings()
    {
        // Close other overlays just in case
        ShowQuitOverlay = false;
        ShowNewGameResetOverlay = false;
        ShowPauseOverlay = false;
        ShowHowToPlayOverlay = false;
        htpOpenedFromPause = false;
        ShowLevelSelectOverlay = false;

        ShowSettingsOverlay = true;
    }

    private static void HideSettings()
    {
        ShowSettingsOverlay = false;
        ShowLevelSelectOverlay = false;
        AudioSliderKnobBase.ClearFocus();

        if (openedSettingsFromPause)
        {
            ShowPauseOverlay = true;      // return to pause menu
            Application.SetPaused(true);  // keep game paused
            openedSettingsFromPause = false;
        }
    }

    public static void SetStageClearPaused(bool value)
    {
        stageClearPaused = value;
        if (value)
        {
            // Flush stale gameplay nav candidates (e.g. PauseBTN) so the first
            // D-pad press on the stage-clear screen focuses an actual button.
            s_focusedButton = null;
            s_prevCandidates.Clear();
            s_navCandidates.Clear();
            if (!Application.IsPaused())
                Application.SetPaused(true);
        }
    }

    public static void ReportLevelCleared(string sceneName)
    {
        if (string.IsNullOrEmpty(sceneName))
            return;

        if (!sceneName.ToLower().Contains("level"))
            return;

        int newLevel = ExtractTrailingNumber(sceneName);
        int prevLevel = ExtractTrailingNumber(LastClearedLevel);

        // Keep the highest unlocked level progression.
        if (newLevel > prevLevel)
        {
            LastClearedLevel = sceneName;
            LevelProgressStore.SaveLastClearedLevel(sceneName);
        }
    }

    private static int ExtractTrailingNumber(string sceneName)
    {
        if (string.IsNullOrEmpty(sceneName))
            return 0;

        int i = sceneName.Length - 1;
        while (i >= 0 && char.IsDigit(sceneName[i]))
            i--;

        if (i == sceneName.Length - 1)
            return 0;

        string numberPart = sceneName.Substring(i + 1);
        if (int.TryParse(numberPart, out int levelNum))
            return levelNum;

        return 0;
    }

    private bool TryGetNextLevelSceneName(string sceneName, out string nextScene)
    {
        nextScene = "";

        if (string.IsNullOrEmpty(sceneName))
            return false;

        int i = sceneName.Length - 1;
        while (i >= 0 && char.IsDigit(sceneName[i]))
            i--;

        // no trailing number
        if (i == sceneName.Length - 1)
            return false;

        string prefix = sceneName.Substring(0, i + 1);
        string numberPart = sceneName.Substring(i + 1);

        int levelNumber;
        if (!int.TryParse(numberPart, out levelNumber))
            return false;

        nextScene = prefix + (levelNumber + 1).ToString();
        return true;
    }

    private void ContinueToNextLevel()
    {
    // Sync with actual loaded scene first (important for Cutscene -> Level1 flow)
    string realScene = Scene.GetCurrentScene();
    if (!string.IsNullOrEmpty(realScene))
        currentScene = realScene;

    if (string.IsNullOrEmpty(currentScene))
    {
        Application.SetPaused(false);
        GoToMainMenu();
        return;
    }

    string nextScene;
    if (!TryGetNextLevelSceneName(currentScene, out nextScene))
    {
        Application.SetPaused(false);
        GoToMainMenu();
        return;
    }

    // Check once only (avoid duplicate checks / duplicate debug prints)
    bool nextSceneExists = Scene.SceneExists(nextScene);

    Console.WriteLine("ContinueToNextLevel -> currentScene: " + currentScene);
    Console.WriteLine("ContinueToNextLevel -> realScene: " + realScene);
    Console.WriteLine("ContinueToNextLevel -> nextScene: " + nextScene);
    Console.WriteLine("ContinueToNextLevel -> exists?: " + nextSceneExists);

    if (!nextSceneExists)
    {
        Console.WriteLine("Next scene does not exist: " + nextScene + " -> going MainMenu");

        // Make sure game is not stuck paused when falling back
        ShowQuitOverlay = false;
        ShowNewGameResetOverlay = false;
        ShowPauseOverlay = false;
        ShowHowToPlayOverlay = false;
        htpOpenedFromPause = false;
        ShowSettingsOverlay = false;
        openedSettingsFromPause = false;
        Application.SetPaused(false);

        GoToMainMenu();
        return;
    }

    // Clear UI/overlay states before changing scene
    ShowQuitOverlay = false;
    ShowNewGameResetOverlay = false;
    ShowPauseOverlay = false;
    ShowHowToPlayOverlay = false;
    htpOpenedFromPause = false;
    ShowSettingsOverlay = false;
    openedSettingsFromPause = false;

    // Make sure stage clear / gameplay freeze is removed before next scene
    Application.SetPaused(false);

    currentScene = nextScene;

    if (ShouldUseLoadingScreenForScene(currentScene))
    {
        LoadingScreenScript.NextLevel = currentScene;
        BeginSceneTransition("LoadingScreenScene", false, true);
        return;
    }

    BeginSceneTransitionWithPostLoadFade(currentScene, true);
}


    private void OnClick()
    {
        string tex = defaultTexture.ToLower();
        Console.WriteLine($"Button clicked: {tex}");

        // HTP overlay: handle Exit (close overlay) and NextStage (advance page) before generic handlers
        if (ShowHowToPlayOverlay)
        {
            if (tex.Contains("exit") || tex.Contains("temp_close") || tex.Contains("close_htp"))
            {
                ShowHowToPlayOverlay = false;
                HtpPage = 1;
                htpOpenedFromPause = false;
                return;
            }
            if (tex.Contains("nextstage"))
            {
                if (HtpPage < 3) HtpPage++;
                return;
            }
        }

        if (tex.Contains("exit"))
        {
            ShowPauseOverlay = false;
            s_resumeCountdownActive = true;

            PauseResumeCountdown? countdown = PauseResumeCountdown.Instance;
            if (countdown != null)
            {
                countdown.StartCountdown();
            }
            else
            {
                s_resumeCountdownActive = false;
                Application.SetPaused(false);
            }

            return;
        }

        if (tex.Contains("pausequit"))
        {
            GoToMainMenu();
            return;
        }

        if (tex.Contains("pauserestart"))
        {
            if (StageClearElement.IsCleared)
            {
                StageClearElement.ResetStageClear();
                RestartSceneImmediateNoFade();
                return;
            }

            if (Scene.GetCurrentScene() == "LatencyCalibration")
            {
                LatencyCalibrator.Instance?.Reset();
                return;
            }
            if (Scene.GetCurrentScene() == "VisualLatencyCalibration")
            {
                VisualLatencyCalibrator.Instance?.Reset();
                return;
            }

            BeginRestartTransition(true);

            return;
        }

        if (tex.Contains("pausesettings"))
        {
            openedSettingsFromPause = true;
            ShowPauseOverlay = false;
            ShowSettingsOverlay = true;
            return;
        }

        if (tex.Contains("restartyes"))
        {
            string realScene = Scene.GetCurrentScene();
            if (string.Equals(realScene, "GameLose", StringComparison.OrdinalIgnoreCase))
            {
                string restartTarget = !string.IsNullOrEmpty(LastGameplayScene) ? LastGameplayScene : "MainMenu";
                LoadSceneImmediateNoFade(restartTarget, string.Equals(restartTarget, "MainMenu", StringComparison.OrdinalIgnoreCase));
                return;
            }

            if (!string.IsNullOrEmpty(LastGameplayScene))
            {
                currentScene = LastGameplayScene;
                BeginSceneTransition(LastGameplayScene, false, true);
            }
            else
            {
                currentScene = "MainMenu";
                BeginSceneTransition("MainMenu", true, true);
            }
            return;
        }

        if (tex.Contains("restartno"))
        {
            if (string.Equals(Scene.GetCurrentScene(), "GameLose", StringComparison.OrdinalIgnoreCase))
            {
                LoadSceneImmediateNoFade("MainMenu", true);
                return;
            }

            GoToMainMenu();
            return;
        }

        if (tex.Contains("win_restart_btn"))
        {
            StageClearElement.ResetStageClear();
            RestartSceneImmediateNoFade();
            return;
        }

        // Stage clear replay button (restart current gameplay level).
        // Must be checked before "start" / New Game matching.
        if (tex.Contains("restart") && !tex.Contains("pause") && !tex.Contains("yes") && !tex.Contains("no"))
        {
            if (StageClearElement.IsCleared)
            {
                StageClearElement.ResetStageClear();
                RestartSceneImmediateNoFade();
                return;
            }

            pendingStageClearResetOnSceneSwitch = true;
            BeginRestartTransition(true);
            return;
        }

        if (tex.Contains("quit yes"))
        {
            if (ShowNewGameResetOverlay)
            {
                ShowNewGameResetOverlay = false;
                ResetProgressForNewGame();
                BeginNewGameFlow();
                return;
            }
            Application.Quit();
            return;
        }

        if (tex.Contains("quit no"))
        {
            if (ShowNewGameResetOverlay)
            {
                ShowNewGameResetOverlay = false;
                return;
            }
            ShowQuitOverlay = false;
            return;
        }

        if (tex.Contains("new game") || tex.Contains("new_game") || tex.Contains("start"))
        {
            if (Scene.GetCurrentScene() == "MainMenu" && HasProgressCache())
            {
                ShowQuitOverlay = false;
                ShowNewGameResetOverlay = true;
                ShowPauseOverlay = false;
                ShowHowToPlayOverlay = false;
                ShowSettingsOverlay = false;
                ShowLevelSelectOverlay = false;
                return;
            }

            BeginNewGameFlow();
            return;
        }

        if (tex.Contains("continue"))
        {
            if (Scene.GetCurrentScene() == "MainMenu")
            {
                if (!HasProgressCache())
                    return;

                BeginMainMenuOverlayTransitionToLevelSelect();
                return;
            }

            string sceneToLoad = "";

            if (!string.IsNullOrEmpty(LastClearedLevel))
            {
                // Player has cleared at least one level — go to the next one
                string nextScene;
                if (TryGetNextLevelSceneName(LastClearedLevel, out nextScene) && Scene.SceneExists(nextScene))
                    sceneToLoad = nextScene;
                else
                    sceneToLoad = LastClearedLevel; // no next level, replay last cleared
            }
            else if (!string.IsNullOrEmpty(LastEnteredLevel))
            {
                // Player paused and quit — return to the level they were in
                sceneToLoad = LastEnteredLevel;
            }
            else
            {
                // Nothing played yet — same as New Game
                sceneToLoad = "Cutscene";
            }

            // Stage clear Continue should switch immediately with no fade.
            if (StageClearElement.IsCleared)
            {
                StageClearElement.ResetStageClear();
                if (string.IsNullOrEmpty(sceneToLoad))
                    sceneToLoad = "MainMenu";
                bool clearHistory = string.Equals(sceneToLoad, "MainMenu", StringComparison.OrdinalIgnoreCase);
                LoadSceneImmediateNoFade(sceneToLoad, clearHistory);
                return;
            }

            NavigateTo(sceneToLoad);
            return;
        }

        if (tex.Contains("settings") || tex.Contains("options"))
        {
            if (Scene.GetCurrentScene() == "MainMenu")
            {
                BeginMainMenuOverlayTransitionToSettings();
                return;
            }
            ShowSettings();
            return;
        }

        if (tex.Contains("back") || tex.Contains("return"))
        {
            if (Scene.GetCurrentScene() == "MainMenu" && ShowSettingsOverlay)
            {
                BeginMainMenuOverlayTransitionToBase();
                return;
            }
            HideSettings();
            return;
        }

        if (tex.Contains("quit"))
        {
            ShowNewGameResetOverlay = false;
            ShowQuitOverlay = true;
            return;
        }

        if (tex.Contains("pause"))
        {
            if (StageClearElement.IsCleared || Scene.GetCurrentScene() == "GameLose")
                return;
            ShowPauseOverlay = true;
            return;
        }

        if (tex.Contains("home"))
        {
            if (StageClearElement.IsCleared)
            {
                StageClearElement.ResetStageClear();
                LoadSceneImmediateNoFade("MainMenu", true);
                return;
            }

            ShowQuitOverlay = false;
            ShowNewGameResetOverlay = false;
            ShowPauseOverlay = false;
            ShowHowToPlayOverlay = false;
            htpOpenedFromPause = false;
            ShowSettingsOverlay = false;
            ShowLevelSelectOverlay = false;
            StageClearElement.ResetStageClear();
            GoToMainMenu();
            return;
        }

        if (tex.Contains("nextstage"))
        {
            if (Scene.GetCurrentScene() == "LatencyCalibration")
            {
                LatencyCalibrator.Instance?.Skip();
                return;
            }
            if (Scene.GetCurrentScene() == "VisualLatencyCalibration")
            {
                VisualLatencyCalibrator.Instance?.Skip();
                return;
            }

            if (StageClearElement.IsCleared)
            {
                string stageScene = Scene.GetCurrentScene();
                if (!string.IsNullOrEmpty(stageScene))
                    currentScene = stageScene;

                string nextScene;
                if (!TryGetNextLevelSceneName(currentScene, out nextScene) || !Scene.SceneExists(nextScene))
                {
                    nextScene = Scene.SceneExists("EndingCutscene") && string.Equals(currentScene, "Level3", StringComparison.OrdinalIgnoreCase)
                        ? "EndingCutscene"
                        : "MainMenu";
                }

                StageClearElement.ResetStageClear();
                ReportLevelCleared(currentScene);
                NavigationButtons.QueuePostLoadFadeIn(nextScene, 0.5f);
                LoadSceneImmediateNoFade(nextScene, string.Equals(nextScene, "MainMenu", StringComparison.OrdinalIgnoreCase));
                return;
            }

            StageClearElement.ResetStageClear();
            ReportLevelCleared(currentScene);
            ContinueToNextLevel();
            return;
        }

        if (tex.Contains("how to play"))
        {
            htpOpenedFromPause = ShowPauseOverlay;
            ShowHowToPlayOverlay = true;
            return;
        }

        if (tex.Contains("temp_close"))
        {
            ShowHowToPlayOverlay = false;
            return;
        }

        if (tex.Contains("windowed"))
        {
            return;
        }

        if (tex.Contains("full screen"))
        {
            return;
        }
    }

    public static void NotifyResumeCountdownFinished()
    {
        s_resumeCountdownActive = false;
        ShowPauseOverlay = false;
        ShowQuitOverlay = false;
        ShowNewGameResetOverlay = false;

        if (openedSettingsFromPause)
        {
            ShowSettingsOverlay = false;
            openedSettingsFromPause = false;
        }
    }

    public static void BeginMainMenuOverlayTransitionToLevelSelect()
    {
        BeginMenuOverlayTransition(1);
    }

    public static void BeginMainMenuOverlayTransitionToSettings()
    {
        BeginMenuOverlayTransition(2);
    }

    public static void BeginMainMenuOverlayTransitionToBase()
    {
        BeginMenuOverlayTransition(3);
    }

    private static void BeginMenuOverlayTransition(int target)
    {
        if (waitingForSceneTransition || waitingForMenuOverlayTransition)
            return;

        pendingMenuOverlayTarget = target;
        waitingForMenuOverlayTransition = true;
        menuOverlayTransitionTimer = MenuOverlayTransitionDuration;
        menuOverlayTransitionTickFrame = -1;
        PendingSceneFadeIn = false;
        FadeAPI.Stop();
        FadeAPI.SetAlpha(0.0f);
        FadeAPI.FadeOut(MenuOverlayTransitionDuration);
    }

    private static void ApplyMenuOverlayTransitionTarget()
    {
        if (pendingMenuOverlayTarget == 1)
        {
            ShowQuitOverlay = false;
            ShowNewGameResetOverlay = false;
            ShowPauseOverlay = false;
            ShowHowToPlayOverlay = false;
            ShowSettingsOverlay = false;
            ShowLevelSelectOverlay = true;
            SelectedLevelScene = "Level1";
            return;
        }

        if (pendingMenuOverlayTarget == 2)
        {
            ShowQuitOverlay = false;
            ShowNewGameResetOverlay = false;
            ShowPauseOverlay = false;
            ShowHowToPlayOverlay = false;
            ShowLevelSelectOverlay = false;
            ShowSettingsOverlay = true;
            return;
        }

        if (pendingMenuOverlayTarget == 3)
        {
            ShowQuitOverlay = false;
            ShowNewGameResetOverlay = false;
            ShowPauseOverlay = false;
            ShowHowToPlayOverlay = false;
            ShowLevelSelectOverlay = false;
            ShowSettingsOverlay = false;
        }
    }

    private static bool HasProgressCache()
    {
        if (!string.IsNullOrWhiteSpace(LastClearedLevel))
            return true;
        if (!string.IsNullOrWhiteSpace(LastEnteredLevel))
            return true;

        var (cleared, entered) = LevelProgressStore.LoadProgress();
        return !string.IsNullOrWhiteSpace(cleared) || !string.IsNullOrWhiteSpace(entered);
    }

    private void BeginNewGameFlow()
    {
        ShowLevelSelectOverlay = false;
        ShowNewGameResetOverlay = false;
        BeginSceneTransition("Cutscene", false, true);
    }

    private void BeginSceneTransitionWithPostLoadFade(string targetScene, bool clearOverlaysAfterFade)
    {
        if (string.IsNullOrEmpty(targetScene))
            return;

        forcePostLoadFadeForNextTransition = true;
        BeginSceneTransition(targetScene, false, clearOverlaysAfterFade);
    }

    private void BeginRestartTransition(bool clearOverlaysAfterFade)
    {
        string restartSceneName = Scene.GetCurrentScene();
        if (string.IsNullOrEmpty(restartSceneName))
            restartSceneName = currentScene;

        BeginSceneTransition(RestartSceneSentinel, false, clearOverlaysAfterFade);
        if (waitingForSceneTransition && !string.IsNullOrEmpty(restartSceneName))
            QueuePostLoadFadeIn(restartSceneName, SceneTransitionDuration);
    }

    private static void ResetProgressForNewGame()
    {
        LastClearedLevel = "";
        LastEnteredLevel = "";
        LevelProgressStore.ClearProgress();
    }

    public static void ReturnToSettingsAfterCalibration()
    {
        openSettingsOnMainMenuEntry = true;

        ShowQuitOverlay = false;
        ShowNewGameResetOverlay = false;
        ShowPauseOverlay = false;
        ShowHowToPlayOverlay = false;
        ShowLevelSelectOverlay = false;
        ShowSettingsOverlay = true;
        openedSettingsFromPause = false;

        currentScene = "MainMenu";
        Scene.LoadScene("MainMenu");
    }
}

public class QuitOverlay : Script
{
    private const string QuitPromptTexture = "Assets/Textures/Backgrounds/quit screen.png";
    private const string NewGamePromptTexture = "Assets/Textures/Backgrounds/Calibrator BG.png";
    private const float QuitPromptAlpha = 1.0f;
    private const float NewGamePromptAlpha = 0.95f;

    public override void Update()
    {
        TransformComponent t = GetTransform();
        bool visible = NavigationButtons.ShowQuitOverlay || NavigationButtons.ShowNewGameResetOverlay;
        t.IsVisible = visible;
        if (!visible)
            return;

        // This script also exists on Yes/No; only skin the full-screen popup background.
        bool isPopupBackground = t.ScaleX > 1000.0f && t.ScaleY > 600.0f;
        if (!isPopupBackground)
            return;

        SpriteComponent sprite = GetSprite();
        if (NavigationButtons.ShowNewGameResetOverlay)
        {
            sprite.Texture = NewGamePromptTexture;
            sprite.SetTint(1.0f, 1.0f, 1.0f, NewGamePromptAlpha);
        }
        else
        {
            sprite.Texture = QuitPromptTexture;
            sprite.SetTint(1.0f, 1.0f, 1.0f, QuitPromptAlpha);
        }
    }
}

public class NewGameResetPromptText : Script
{
    public override void Update()
    {
        bool visible = NavigationButtons.ShowNewGameResetOverlay;
        TransformComponent t = GetTransform();
        t.IsVisible = visible;

        TextComponentAPI text = GetTextComponent();
        text.SetVisible(visible);
        if (!visible)
            return;

        text.SetText("Start a new game?\nYour saved progress will be erased.");
    }
}

public class PauseOverlay : Script
{
    public override void Update()
    {
        TransformComponent t = GetTransform();
        t.IsVisible = NavigationButtons.ShowPauseOverlay;
    }
}

public class HowToPlayOverlay : Script
{
    public override void Update()
    {
        TransformComponent t = GetTransform();
        t.IsVisible = NavigationButtons.ShowHowToPlayOverlay;
    }
}

public class HowToPlayPage1 : Script
{
    public override void Update()
    {
        TransformComponent t = GetTransform();
        t.IsVisible = NavigationButtons.ShowHowToPlayOverlay && NavigationButtons.HtpPage == 1;
    }
}

public class HowToPlayPage2 : Script
{
    public override void Update()
    {
        TransformComponent t = GetTransform();
        t.IsVisible = NavigationButtons.ShowHowToPlayOverlay && NavigationButtons.HtpPage == 2;
    }
}

public class HowToPlayPage3 : Script
{
    public override void Update()
    {
        TransformComponent t = GetTransform();
        t.IsVisible = NavigationButtons.ShowHowToPlayOverlay && NavigationButtons.HtpPage == 3;
    }
}

// Visible when HTP overlay is open AND there is a next page (pages 1 and 2)
public class HtpNextButton : Script
{
    public override void Update()
    {
        TransformComponent t = GetTransform();
        t.IsVisible = NavigationButtons.ShowHowToPlayOverlay && NavigationButtons.HtpPage < 3;
    }
}

// Attach this script name to your Settings root entity (BG_PlaceholderImg)
// so the actual settings panel appears/disappears with ShowSettingsOverlay.
public class SettingsOverlay : Script
{
    public override void Update()
    {
        TransformComponent t = GetTransform();
        t.IsVisible = NavigationButtons.ShowSettingsOverlay;
    }
}

internal static class LevelProgressStore
{
    private const string SaveFileName = "level_progress.json";

    private static readonly string[] CandidatePaths = new[]
    {
        Path.Combine("PulseProtocol", "JSON", SaveFileName),
        Path.Combine("..", "..", "PulseProtocol", "JSON", SaveFileName),
        Path.Combine("..", "..", "..", "PulseProtocol", "JSON", SaveFileName),
        SaveFileName
    };

    private sealed class ProgressData
    {
        public string LastClearedLevel { get; set; } = "";
        public string LastEnteredLevel { get; set; } = "";
    }

    // Loads both fields at once; returns (cleared, entered).
    public static (string cleared, string entered) LoadProgress()
    {
        foreach (string path in CandidatePaths)
        {
            try
            {
                if (!File.Exists(path))
                    continue;

                string json = File.ReadAllText(path);
                ProgressData? data = JsonSerializer.Deserialize<ProgressData>(json);
                if (data != null)
                    return (data.LastClearedLevel ?? "", data.LastEnteredLevel ?? "");
            }
            catch { }
        }
        return ("", "");
    }

    public static string LoadLastClearedLevel() => LoadProgress().cleared;
    public static string LoadLastEnteredLevel() => LoadProgress().entered;

    private static void SaveProgress(string cleared, string entered)
    {
        string json = JsonSerializer.Serialize(
            new ProgressData { LastClearedLevel = cleared, LastEnteredLevel = entered },
            new JsonSerializerOptions { WriteIndented = true });

        foreach (string path in CandidatePaths)
        {
            try
            {
                string? dir = Path.GetDirectoryName(path);
                if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
                    continue;

                File.WriteAllText(path, json);
                return;
            }
            catch { }
        }
    }

    public static void SaveLastClearedLevel(string sceneName)
    {
        if (string.IsNullOrWhiteSpace(sceneName))
            return;
        var (_, entered) = LoadProgress();
        SaveProgress(sceneName, entered);
    }

    public static void SaveLastEnteredLevel(string sceneName)
    {
        if (string.IsNullOrWhiteSpace(sceneName))
            return;
        var (cleared, _) = LoadProgress();
        SaveProgress(cleared, sceneName);
    }

    public static void ClearProgress()
    {
        foreach (string path in CandidatePaths)
        {
            try
            {
                if (File.Exists(path))
                    File.Delete(path);
            }
            catch
            {
                // Try next candidate path.
            }
        }
    }


}

public class MainMenuAutoFadeIn : Script
{
    private bool ran = false;

    public override void Update()
    {
        if (ran)
            return;

        if (!string.Equals(Scene.GetCurrentScene(), "MainMenu", StringComparison.OrdinalIgnoreCase))
            return;

        ran = true;
        // MainMenu no longer auto-fades on entry.
        // Fade-in should be controlled explicitly by NavigationButtons.PendingSceneFadeIn.
        FadeAPI.Stop();
        FadeAPI.SetAlpha(0.0f);
    }
}
