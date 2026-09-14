/******************************************************************************/
/**
* @file        ComboKeySlot.cs
* @project     Pulse Protocol
* @author      Carrie Lam (65%)
*              Chloe Lau (30%)
*              Goh Pin Kai (5%)
* @brief       Represents a single key slot in the combo UI, handling sprite detection,
*              registration, and animations for enemy reveals and player input feedback
*              (correct, wrong, countdown, pop effects). Extended with dim-always-visible
*              behaviour (0.2 alpha during enemy turn), fade-in on player turn start,
*              fade-out back to dim on enemy turn reset, and per-slot beat scale pulse.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/

using System;
using ScriptAPI;

public class ComboKeySlot : Script
{
    // =========================
    // Configuration
    // =========================
    public int SlotIndex = 0;

    // Enemy row sprites
    public string WSprite = "../../PulseProtocol/Assets/Textures/UI/W.png";
    public string ASprite = "../../PulseProtocol/Assets/Textures/UI/A.png";
    public string SSprite = "../../PulseProtocol/Assets/Textures/UI/S.png";
    public string DSprite = "../../PulseProtocol/Assets/Textures/UI/D.png";

    // Empty gap slot sprites (shown on slots not used by the current enemy's pattern)
    public string PlayerEmptySprite = "playeremptykey";
    public string EnemyEmptySprite  = "enemyemptykey";

    // Player fill sprites
    public string BlankSprite = "questionmark_blankkey";
    public string GreenWSprite = "W_KEY_GREEN";
    public string GreenASprite = "A_KEY_GREEN";
    public string GreenSSprite = "S_KEY_GREEN";
    public string GreenDSprite = "D_KEY_GREEN";

    public string RedWSprite = "W_KEY_RED";
    public string RedASprite = "A_KEY_RED";
    public string RedSSprite = "S_KEY_RED";
    public string RedDSprite = "D_KEY_RED";

    // Controller fill sprites
    public string GreenTriSprite = "Controller_Green_TRI";
    public string GreenSqSprite = "Controller_Green_SQ";
    public string GreenXSprite_Controller = "Controller_Green_X";
    public string GreenOSprite = "Controller_Green_O";

    public string RedTriSprite = "Controller_Red_TRI";
    public string RedSqSprite = "Controller_Red_SQ";
    public string RedXSprite_Controller = "Controller_Red_X";
    public string RedOSprite = "Controller_Red_O";

    public float DisplayDuration = 0.5f;
    public float ScaleAnimDuration = 0.15f;
    public float ResultDisplayDuration = 0.3f;
    public float CorrectHintDuration = 0.3f;

    // =========================
    // Internal State
    // =========================
    private float displayTimer = 0f;
    private bool isShowing = false;
    private bool hasRegistered = false;
    private bool IsEnemySlot = false;

    private bool isScaling = false;
    private float scaleTimer = 0f;
    private float baseScaleX = 1f;
    private float baseScaleY = 1f;
    private bool scaleCached = false;

    // Enemy pop animation
    private bool isPopping = false;
    private float popTimer = 0f;
    public float PopDuration = 0.15f;
    public float PopScale = 1.9f; // this is for enhancing the "pop" animation

    // PLAYER pop animation (when correct input)
    private bool isPoppingCorrect = false;
    private float correctPopTimer = 0f;
    public float CorrectPopDuration = 0.1f;
    public float CorrectPopScale = 1.2f;

    private bool isShowingResult = false;
    private float resultTimer = 0f;

    // Correct hint after wrong press
    private bool isShowingCorrectHint = false;
    private float correctHintTimer = 0f;
    private char correctKey = ' ';

    // Beat-synced scale pulse (triggered per-slot during player turn)
    public  float BeatScaleAmp        = 0.3f;
    private float _scalePulseTimer    = 0f;
    private float _scalePulseTotalDur = 0.5f;

    // Fade-in from dim (0.2) to full (1.0) when player turn starts
    private bool  _inFadeIn       = false;
    private float _fadeInTimer    = 0f;
    private float _fadeInDuration = 0.5f;

    // Fade-out from full (1.0) back to dim (0.2) when enemy turn resets
    private bool  _isFadingToDim    = false;
    private float _fadeToDimTimer   = 0f;
    private float _fadeToDimDuration = 0.5f;

    // Appear fade-in (0 → dim) when the level/session first starts
    public float AppearFadeInDuration = 1.0f;
    private bool  _inAppearFadeIn      = false;
    private float _appearFadeInTimer   = 0f;
    private float _currentAlpha        = 0f; // tracked so ShowBlankFadeToDim knows current state

    // Fade from current alpha → 0 then hide (unused slots when enemy turn starts)
    private bool  _isFadingToHidden       = false;
    private float _fadeToHiddenTimer      = 0f;
    private float _fadeToHiddenDuration   = 0.5f;
    private float _fadeToHiddenStartAlpha = 0f;

    private static bool _preferControllerUI = false;
    private static bool _inputModeInitialized = false;
    private static bool _wasControllerConnected = false;
    private bool _prevPreferControllerUI = false;

    private enum SpriteType { None, EnemyKey, GreenPlayer, RedPlayer }
    private SpriteType _displayedSpriteType = SpriteType.None;
    private char _displayedKey = '\0';

    // =========================
    // Update
    // =========================
    public override void Update()
    {
        if (hasRegistered && !ComboDisplayManager.IsSlotRegistered(SlotIndex, this))
            hasRegistered = false;

        if (!hasRegistered)
        {
            // Cache scale FIRST
            if (!scaleCached)
            {
                var t = GetTransform();
                baseScaleX = t.ScaleX;
                baseScaleY = t.ScaleY;
                scaleCached = true;
            }

            var sprite = GetSprite();
            if (!sprite.HasSprite()) return;

            string texturePath = sprite.Texture;
            if (string.IsNullOrEmpty(texturePath)) return;

            string fileName = System.IO.Path.GetFileName(texturePath.ToLower());

            // Check player row identifiers first
            if (fileName == "player input green w.png" || fileName == "w_key_green" || fileName == "controller_green_tri") { IsEnemySlot = false; SlotIndex = 0; }
            else if (fileName == "player input green a.png" || fileName == "a_key_green" || fileName == "controller_green_sq") { IsEnemySlot = false; SlotIndex = 1; }
            else if (fileName == "player input green s.png" || fileName == "s_key_green" || fileName == "controller_green_x") { IsEnemySlot = false; SlotIndex = 2; }
            else if (fileName == "player input green d.png" || fileName == "d_key_green" || fileName == "controller_green_o") { IsEnemySlot = false; SlotIndex = 3; }

            // Then enemy row
            else if (fileName == "ekeyw.png" || fileName == "w_key_black") { IsEnemySlot = true; SlotIndex = 0; }
            else if (fileName == "ekeya.png" || fileName == "a_key_black") { IsEnemySlot = true; SlotIndex = 1; }
            else if (fileName == "ekeys.png" || fileName == "s_key_black") { IsEnemySlot = true; SlotIndex = 2; }
            else if (fileName == "ekeyd.png" || fileName == "d_key_black") { IsEnemySlot = true; SlotIndex = 3; }

            // Then single letter player row
            else if (fileName == "w.png") { IsEnemySlot = false; SlotIndex = 0; }
            else if (fileName == "a.png") { IsEnemySlot = false; SlotIndex = 1; }
            else if (fileName == "s.png") { IsEnemySlot = false; SlotIndex = 2; }
            else if (fileName == "d.png") { IsEnemySlot = false; SlotIndex = 3; }
            else
            {
                Console.WriteLine($"[ComboKeySlot] ⚠️ Could not detect slot from '{fileName}'");
                IsEnemySlot = false;
                SlotIndex = 0;
            }

            ComboDisplayManager.RegisterKeySlot(SlotIndex, this);
            HideInstant();
            hasRegistered = true;
            Console.WriteLine($"[ComboKeySlot] ✓ Slot {SlotIndex} registered as {(IsEnemySlot ? "Enemy" : "Player")} slot");

            // Player slots: immediately show blank question mark and fade in from invisible
            if (!IsEnemySlot && sprite.HasSprite())
            {
                sprite.Texture = BlankSprite;
                sprite.SetTint(1f, 1f, 1f, 1f);
                GetFade().SetAlpha(0f);
                _currentAlpha = 0f;
                var t = GetTransform();
                t.IsVisible = true;
                t.ScaleX = baseScaleX;
                t.ScaleY = baseScaleY;
                _inAppearFadeIn = true;
                _appearFadeInTimer = 0f;
            }
        }

        // =========================
        // Hot-swap: refresh sprite when controller/keyboard mode changes
        // =========================
        {
            bool currentMode = UsingControllerUI();
            if (currentMode != _prevPreferControllerUI)
            {
                _prevPreferControllerUI = currentMode;
                if (_displayedSpriteType != SpriteType.None && _displayedKey != '\0')
                {
                    var s = GetSprite();
                    if (s.HasSprite())
                    {
                        switch (_displayedSpriteType)
                        {
                            case SpriteType.EnemyKey:    s.Texture = GetSpriteForKey(_displayedKey);    break;
                            case SpriteType.GreenPlayer: s.Texture = GetGreenSpriteForKey(_displayedKey); break;
                            case SpriteType.RedPlayer:   s.Texture = GetRedSpriteForKey(_displayedKey);  break;
                        }
                    }
                }
            }
        }

        // =========================
        // Scale Animation
        // =========================
        if (isScaling && !IsEnemySlot)
        {
            scaleTimer += Time.DeltaTime;
            float t01 = Math.Min(scaleTimer / ScaleAnimDuration, 1f);
            float eased = 1f - (1f - t01) * (1f - t01);

            var transform = GetTransform();
            transform.ScaleX = baseScaleX * eased;
            transform.ScaleY = baseScaleY * eased;

            if (t01 >= 1f)
            {
                isScaling = false;
                // Console.WriteLine($"[ComboKeySlot] Slot {SlotIndex} scale animation complete");
            }
        }

        // =========================
        // Pop Animation (enemy row)
        // =========================
        if (isPopping)
        {
            popTimer += Time.DeltaTime;
            float t01 = Math.Min(popTimer / PopDuration, 1f);

            // Ease from PopScale back to 1
            float currentScale = PopScale + (1f - PopScale) * t01;

            var transform = GetTransform();
            transform.ScaleX = baseScaleX * currentScale;
            transform.ScaleY = baseScaleY * currentScale;

            if (t01 >= 1f)
                isPopping = false;
        }

        // =========================
        // Correct Pop Animation
        // =========================
        if (isPoppingCorrect)
        {
            correctPopTimer += Time.DeltaTime;
            float t01 = Math.Min(correctPopTimer / CorrectPopDuration, 1f);
            float currentScale = CorrectPopScale + (1f - CorrectPopScale) * t01;

            var transform = GetTransform();
            transform.ScaleX = baseScaleX * currentScale;
            transform.ScaleY = baseScaleY * currentScale;

            if (t01 >= 1f)
                isPoppingCorrect = false;
        }

        // =========================
        // Result Display Timer
        // =========================
        if (isShowingResult)
        {
            resultTimer -= Time.DeltaTime;
            if (resultTimer <= 0f)
            {
                isShowingResult = false;
                if (isShowingCorrectHint)
                    ShowCorrectHint(); // swap to green correct key
            }
        }

        // =========================
        // Correct Hint Timer
        // =========================
        if (isShowingCorrectHint && !isShowingResult)
        {
            correctHintTimer -= Time.DeltaTime;
            if (correctHintTimer <= 0f)
            {
                isShowingCorrectHint = false;
                HideInstant();
            }
        }


        // =========================
        // Fade-in (dim → full on player turn start)
        // =========================
        if (_inFadeIn)
        {
            _fadeInTimer += Time.DeltaTime;
            float t01 = Math.Min(_fadeInTimer / _fadeInDuration, 1f);
            float eased = 1f - (1f - t01) * (1f - t01);
            _currentAlpha = 0.2f + 0.8f * eased;
            GetFade().SetAlpha(_currentAlpha);
            if (t01 >= 1f) _inFadeIn = false;
        }

        // =========================
        // Fade-out (full → dim on enemy turn reset)
        // =========================
        if (_isFadingToDim)
        {
            _fadeToDimTimer += Time.DeltaTime;
            float t01 = Math.Min(_fadeToDimTimer / _fadeToDimDuration, 1f);
            // ease-out: fast drop then settles slowly at dim
            float eased = 1f - (1f - t01) * (1f - t01);
            _currentAlpha = 1.0f - 0.8f * eased;
            GetFade().SetAlpha(_currentAlpha);
            if (t01 >= 1f) _isFadingToDim = false;
        }

        // =========================
        // Appear fade-in (0 → dim at level/session start)
        // =========================
        if (_inAppearFadeIn)
        {
            _appearFadeInTimer += Time.DeltaTime;
            float t01 = Math.Min(_appearFadeInTimer / AppearFadeInDuration, 1f);
            float eased = 1f - (1f - t01) * (1f - t01);
            _currentAlpha = 0.2f * eased;
            GetFade().SetAlpha(_currentAlpha);
            if (t01 >= 1f) _inAppearFadeIn = false;
        }

        // =========================
        // Fade to hidden (unused slots during enemy turn)
        // =========================
        if (_isFadingToHidden)
        {
            _fadeToHiddenTimer += Time.DeltaTime;
            float t01 = Math.Min(_fadeToHiddenTimer / _fadeToHiddenDuration, 1f);
            float eased = t01 * t01; // ease-in: slow start, quicker end
            _currentAlpha = _fadeToHiddenStartAlpha * (1f - eased);
            GetFade().SetAlpha(_currentAlpha);
            if (t01 >= 1f)
            {
                _isFadingToHidden = false;
                var ht = GetTransform();
                ht.IsVisible = false;
                _currentAlpha = 0f;
            }
        }

        // =========================
        // Beat scale pulse (per-slot, player turn)
        // =========================
        if (_scalePulseTimer > 0f && !isPopping && !isPoppingCorrect && !isScaling)
        {
            _scalePulseTimer -= Time.DeltaTime;
            if (_scalePulseTimer < 0f) _scalePulseTimer = 0f;
            float ft    = _scalePulseTimer / _scalePulseTotalDur; // always 0..1, no NaN
            float eased = (float)System.Math.Sqrt(ft); // ease-out: holds big, eases back
            var bt = GetTransform();
            bt.ScaleX = baseScaleX * (1f + BeatScaleAmp * eased);
            bt.ScaleY = baseScaleY * (1f + BeatScaleAmp * eased);
        }

        // =========================
        // Display Timer
        // =========================
        if (isShowing)
        {
            displayTimer -= Time.DeltaTime;
            if (displayTimer <= 0)
                Hide();
        }
    }

    // =========================
    // Player Fill Animation
    // =========================
    public void StartCountdown(char expectedKey)
    {
        if (IsEnemySlot) return;

            // DEBUG
        if (Conductor.Instance != null)
        {
            float songTime = Conductor.Instance.GetSongTime();
            float beatDuration = Conductor.Instance.GetBeatDuration();
            float timeUntilBeat = beatDuration - (songTime % beatDuration);
            Console.WriteLine($"[ComboKeySlot] Slot {SlotIndex} StartCountdown | timeUntilNextBeat={timeUntilBeat:F3}");
        }

        correctKey = expectedKey;

        var sprite = GetSprite();
        sprite.Texture = BlankSprite;
        sprite.SetTint(1f, 1f, 1f, 1f); // white tint so blank key is visible

        var transform = GetTransform();
        transform.IsVisible = true;
        transform.ScaleX = 0f;
        transform.ScaleY = 0f;

        isScaling = true;
        scaleTimer = 0f;
        isShowingResult = false;
        isShowingCorrectHint = false;
        _inAppearFadeIn = false;
        _displayedKey = '\0';
        _displayedSpriteType = SpriteType.None;
    }

    public void ShowCorrect(char key)
    {
        if (IsEnemySlot) return;

        isScaling = false;
        isShowingResult = false;
        isShowingCorrectHint = false;
        _inFadeIn = false;
        _isFadingToDim = false;
        _inAppearFadeIn = false;
        _currentAlpha = 1.0f;
        GetFade().SetAlpha(1.0f);

        var sprite = GetSprite();
        sprite.Texture = GetGreenSpriteForKey(key);
        sprite.SetTint(1f, 1f, 1f, 1f);
        _displayedKey = key;
        _displayedSpriteType = SpriteType.GreenPlayer;

        var transform = GetTransform();
        transform.IsVisible = true;
        transform.ScaleX = baseScaleX;
        transform.ScaleY = baseScaleY;

        isPoppingCorrect = true;
        correctPopTimer = 0f;
    }

    public void ShowWrong(char pressedKey, char expectedKey)
    {
        if (IsEnemySlot) return;

        isScaling = false;
        isShowingResult = false;
        isShowingCorrectHint = false;
        _inFadeIn = false;
        _isFadingToDim = false;
        _inAppearFadeIn = false;
        _currentAlpha = 1.0f;
        GetFade().SetAlpha(1.0f);

        var sprite = GetSprite();
        sprite.Texture = GetRedSpriteForKey(pressedKey);
        sprite.SetTint(1f, 1f, 1f, 1f);
        _displayedKey = pressedKey;
        _displayedSpriteType = SpriteType.RedPlayer;

        var transform = GetTransform();
        transform.IsVisible = true;
        transform.ScaleX = baseScaleX;
        transform.ScaleY = baseScaleY;
    }

    private void ShowCorrectHint()
    {
        var sprite = GetSprite();
        sprite.Texture = GetGreenSpriteForKey(correctKey);
        sprite.SetTint(1f, 1f, 1f, 1f);
        _displayedKey = correctKey;
        _displayedSpriteType = SpriteType.GreenPlayer;

        var transform = GetTransform();
        transform.IsVisible = true;
        transform.ScaleX = baseScaleX;
        transform.ScaleY = baseScaleY;
    }

    // =========================
    // Display Control
    // =========================
    public void ShowKey(char key)
    {
        var sprite = GetSprite();
        if (!sprite.HasSprite())
        {
            Console.WriteLine("[ComboKeySlot] ERROR: No sprite component!");
            return;
        }

        sprite.Texture = GetSpriteForKey(key);
        _displayedKey = key;
        _displayedSpriteType = SpriteType.EnemyKey;

        var fade = GetFade();
        fade.SetAlpha(1.0f);

        var transform = GetTransform();
        transform.IsVisible = true;

        isShowing = true;
        displayTimer = DisplayDuration;

        Console.WriteLine($"[ComboKeySlot] Slot {SlotIndex} ({(IsEnemySlot ? "Enemy" : "Player")}) showing {key}");
    }

    public void ShowKeyPersistent(char key)
    {
        var sprite = GetSprite();
        if (!sprite.HasSprite())
        {
            Console.WriteLine("[ComboKeySlot] ERROR: No sprite component!");
            return;
        }

        sprite.Texture = GetSpriteForKey(key);
        _displayedKey = key;
        _displayedSpriteType = SpriteType.EnemyKey;

        var fade = GetFade();
        fade.SetAlpha(1.0f);

        var transform = GetTransform();
        transform.IsVisible = true;

        isShowing = false;
        displayTimer = 0f;

        Console.WriteLine($"[ComboKeySlot] Slot {SlotIndex} ({(IsEnemySlot ? "Enemy" : "Player")}) showing {key} (persistent)");
    }

    public void FadeOutAndHide(float duration)
    {
        var fade = GetFade();
        fade.FadeOut(duration);

        isShowing = true;
        displayTimer = duration;

        Console.WriteLine($"[ComboKeySlot] Slot {SlotIndex} ({(IsEnemySlot ? "Enemy" : "Player")}) fading out over {duration}s");
    }

    // Show the empty-gap PNG for this slot (used when the enemy pattern doesn't use this slot position)
    public void ShowEmpty()
    {
        var sprite = GetSprite();
        if (!sprite.HasSprite()) return;

        sprite.Texture = IsEnemySlot ? EnemyEmptySprite : PlayerEmptySprite;
        sprite.SetTint(1f, 1f, 1f, 1f);

        var transform = GetTransform();
        transform.IsVisible = true;
        transform.ScaleX = baseScaleX > 0f ? baseScaleX : 1f;
        transform.ScaleY = baseScaleY > 0f ? baseScaleY : 1f;

        float alpha = IsEnemySlot ? 1f : 0.2f;
        GetFade().SetAlpha(alpha);
        _currentAlpha = alpha;

        isShowing         = false;
        isScaling         = false;
        isPopping         = false;
        isPoppingCorrect  = false;
        isShowingResult   = false;
        isShowingCorrectHint = false;
        _inFadeIn         = false;
        _isFadingToDim    = false;
        _isFadingToHidden = false;
        _inAppearFadeIn   = false;
    }

    public void HideInstant()
    {
        var sprite = GetSprite();
        if (sprite.HasSprite())
            sprite.SetTint(1f, 1f, 1f, 1f); // reset tint

        var fade = GetFade();
        fade.SetAlpha(1.0f);

        var transform = GetTransform();
        transform.IsVisible = false;
        transform.ScaleX = baseScaleX > 0f ? baseScaleX : 1f;
        transform.ScaleY = baseScaleY > 0f ? baseScaleY : 1f;

        isShowing = false;
        isScaling = false;
        isShowingResult = false;
        isShowingCorrectHint = false;
        isPoppingCorrect = false;
        _inFadeIn = false;
        _isFadingToDim = false;
        _inAppearFadeIn = false;
        _isFadingToHidden = false;
        _currentAlpha = 0f;
        _displayedKey = '\0';
        _displayedSpriteType = SpriteType.None;
    }

    // Smoothly fade from current alpha → 0, then hide. Used for unused player slots.
    public void FadeToHidden(float duration)
    {
        if (IsEnemySlot) return;

        var transform = GetTransform();
        if (!transform.IsVisible || _currentAlpha <= 0f)
        {
            HideInstant();
            return;
        }

        _fadeToHiddenStartAlpha = _currentAlpha;
        _fadeToHiddenDuration   = Math.Max(duration, 0.05f);
        _fadeToHiddenTimer      = 0f;
        _isFadingToHidden       = true;

        _inFadeIn       = false;
        _isFadingToDim  = false;
        _inAppearFadeIn = false;
    }

    public void Hide()
    {
        var transform = GetTransform();
        transform.IsVisible = false;
        isShowing = false;
        _displayedKey = '\0';
        _displayedSpriteType = SpriteType.None;
    }

    // =========================
    // Helper Funcs
    // =========================
    public bool GetIsEnemySlot() => IsEnemySlot;

    private bool UsingControllerUI()
    {
        InputComponent input = GetInput();

        bool controllerConnected = input.IsGamepadConnected;

        bool controllerUsed =
            input.IsGamepadButtonPressed(3) ||  // Triangle -> W
            input.IsGamepadButtonPressed(1) ||  // Square   -> A
            input.IsGamepadButtonPressed(0) ||  // Cross    -> S
            input.IsGamepadButtonPressed(2);    // Circle   -> D

        bool keyboardUsed =
            input.IsKeyPressed(87) ||  // W
            input.IsKeyPressed(65) ||  // A
            input.IsKeyPressed(83) ||  // S
            input.IsKeyPressed(68);    // D

        if (!_inputModeInitialized)
        {
            _preferControllerUI = controllerConnected || controllerUsed;
            _inputModeInitialized = true;
        }

        // Switch to controller UI when controller is reconnected mid-game
        if (controllerConnected && !_wasControllerConnected)
            _preferControllerUI = true;
        _wasControllerConnected = controllerConnected;

        // Force fallback to keyboard if controller is gone
        if (!controllerConnected)
            _preferControllerUI = false;

        if (controllerUsed)
            _preferControllerUI = true;

        if (keyboardUsed)
            _preferControllerUI = false;

        return _preferControllerUI;
    }
    private string GetSpriteForKey(char key)
    {
        if (UsingControllerUI())
        {
            switch (key)
            {
                case 'W': return GreenTriSprite;
                case 'A': return GreenOSprite;
                case 'S': return GreenXSprite_Controller;
                case 'D': return GreenSqSprite;
                default:
                    Console.WriteLine($"[ComboKeySlot] Unknown key: {key}");
                    return GreenTriSprite;
            }
        }

        switch (key)
        {
            case 'W': return WSprite;
            case 'A': return ASprite;
            case 'S': return SSprite;
            case 'D': return DSprite;
            default:
                Console.WriteLine($"[ComboKeySlot] Unknown key: {key}");
                return WSprite;
        }
    }

    private string GetGreenSpriteForKey(char key)
    {
        if (UsingControllerUI())
        {
            switch (key)
            {
                case 'W': return GreenTriSprite;
                case 'A': return GreenOSprite;
                case 'S': return GreenXSprite_Controller;
                case 'D': return GreenSqSprite;
                default: return GreenTriSprite;
            }
        }

        switch (key)
        {
            case 'W': return GreenWSprite;
            case 'A': return GreenASprite;
            case 'S': return GreenSSprite;
            case 'D': return GreenDSprite;
            default: return GreenWSprite;
        }
    }

    private string GetRedSpriteForKey(char key)
    {
        if (UsingControllerUI())
        {
            switch (key)
            {
                case 'W': return RedTriSprite;
                case 'A': return RedOSprite;
                case 'S': return RedXSprite_Controller;
                case 'D': return RedSqSprite;
                default: return RedTriSprite;
            }
        }

        switch (key)
        {
            case 'W': return RedWSprite;
            case 'A': return RedASprite;
            case 'S': return RedSSprite;
            case 'D': return RedDSprite;
            default: return RedWSprite;
        }
    }

    // FOR PLAYER SLOTS: snap from dim to full + pop, called at player turn start
    public void ShowBlankPop(char expectedKey)
    {
        if (IsEnemySlot) return;

        correctKey = expectedKey;

        var sprite = GetSprite();
        if (!sprite.HasSprite()) return;

        sprite.Texture = BlankSprite;
        sprite.SetTint(1f, 1f, 1f, 1f);

        var fade = GetFade();
        fade.SetAlpha(0.2f); // start dim, fade-in to full over one beat
        _currentAlpha = 0.2f;

        var transform = GetTransform();
        transform.IsVisible = true;
        transform.ScaleX = baseScaleX * PopScale;
        transform.ScaleY = baseScaleY * PopScale;

        isPopping = true;
        popTimer = 0f;
        isScaling = false;
        isShowingResult = false;
        isShowingCorrectHint = false;
        isPoppingCorrect = false;
        _isFadingToDim = false;
        _inAppearFadeIn = false;
        _isFadingToHidden = false;
        _displayedKey = '\0';
        _displayedSpriteType = SpriteType.None;

        _fadeInDuration = Conductor.Instance != null ? Conductor.Instance.SecondsPerBeat : 0.5f;
        _fadeInTimer = 0f;
        _inFadeIn = true;
    }

    // FOR PLAYER SLOTS: show blank/dim during enemy turn (not yet pressable)
    public void ShowBlankDim(char expectedKey)
    {
        if (IsEnemySlot) return;

        correctKey = expectedKey;

        var sprite = GetSprite();
        if (!sprite.HasSprite()) return;

        sprite.Texture = BlankSprite;
        sprite.SetTint(1f, 1f, 1f, 1f);

        var fade = GetFade();
        fade.SetAlpha(0.2f); // dim — not pressable yet
        _currentAlpha = 0.2f;

        var transform = GetTransform();
        transform.IsVisible = true;
        transform.ScaleX = baseScaleX;
        transform.ScaleY = baseScaleY;

        isPopping = false;
        isScaling = false;
        isShowingResult = false;
        isShowingCorrectHint = false;
        isPoppingCorrect = false;
        _inFadeIn = false;
        _isFadingToDim = false;
        _inAppearFadeIn = false;
        _isFadingToHidden = false;
        _displayedKey = '\0';
        _displayedSpriteType = SpriteType.None;
    }

    // FOR PLAYER SLOTS: smoothly fade from full opacity back to dim when enemy turn resets
    public void ShowBlankFadeToDim(char expectedKey, float duration)
    {
        if (IsEnemySlot) return;

        correctKey = expectedKey;

        var sprite = GetSprite();
        if (!sprite.HasSprite()) return;

        sprite.Texture = BlankSprite;
        sprite.SetTint(1f, 1f, 1f, 1f);

        var transform = GetTransform();
        bool wasVisible = transform.IsVisible;
        transform.IsVisible = true;
        transform.ScaleX = baseScaleX;
        transform.ScaleY = baseScaleY;

        isPopping = false;
        isScaling = false;
        isShowingResult = false;
        isShowingCorrectHint = false;
        isPoppingCorrect = false;
        _inFadeIn = false;
        _inAppearFadeIn = false;
        _isFadingToHidden = false;
        _displayedKey = '\0';
        _displayedSpriteType = SpriteType.None;

        var fade = GetFade();
        // Only do the flash-to-full-then-dim animation when coming from full opacity (e.g. after player turn).
        // If the slot is already at dim or invisible (e.g. level start), just snap to dim.
        if (wasVisible && _currentAlpha > 0.5f)
        {
            fade.SetAlpha(1.0f);
            _currentAlpha = 1.0f;
            _fadeToDimDuration = duration;
            _fadeToDimTimer    = 0f;
            _isFadingToDim     = true;
        }
        else
        {
            fade.SetAlpha(0.2f);
            _currentAlpha = 0.2f;
            _isFadingToDim = false;
        }
    }

    // Trigger a brief scale pulse on this slot (called beat-by-beat during player turn)
    public void TriggerScalePulse(float duration)
    {
        if (IsEnemySlot) return;
        _scalePulseTotalDur = duration;
        _scalePulseTimer    = duration;
    }

    // Pop animation for gap (empty) slots — same bounce as ShowKeyPop but uses the empty sprite
    public void ShowEmptyPop()
    {
        var sprite = GetSprite();
        if (!sprite.HasSprite()) return;

        sprite.Texture = IsEnemySlot ? EnemyEmptySprite : PlayerEmptySprite;

        var fade = GetFade();
        fade.SetAlpha(IsEnemySlot ? 1.0f : 0.2f);

        var transform = GetTransform();
        transform.IsVisible = true;
        transform.ScaleX = baseScaleX * PopScale;
        transform.ScaleY = baseScaleY * PopScale;

        isPopping = true;
        popTimer = 0f;
        isShowing = false;
    }

    // FOR ENEMY INPUT EFFECT ONLY!!!!
    public void ShowKeyPop(char key)
    {
        var sprite = GetSprite();
        if (!sprite.HasSprite()) return;

        sprite.Texture = GetSpriteForKey(key);
        _displayedKey = key;
        _displayedSpriteType = SpriteType.EnemyKey;

        var fade = GetFade();
        fade.SetAlpha(1.0f);

        var transform = GetTransform();
        transform.IsVisible = true;
        transform.ScaleX = baseScaleX * PopScale;
        transform.ScaleY = baseScaleY * PopScale;

        isPopping = true;
        popTimer = 0f;
        isShowing = false;
    }
}