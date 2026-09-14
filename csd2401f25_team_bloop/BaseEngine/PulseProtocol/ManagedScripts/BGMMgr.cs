/******************************************************************************/
/**
* @file        HoverableButton.cs
* @project     Pulse Protocol
* @author      Chloe Lau Rey En (primary) - 70%
               Leu Jun Yong (secondary) - 30%
* @brief       Script for hovering of buttons, and transition of scenes and showing overlay for all 4 buttons
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/

using System;
using ScriptAPI;

public class GeneralBGM : Script
{
    private bool started = false;
    private bool wasPaused = false;

    public override void Update()
    {
        AudioComponent audio = GetAudio();

        if (!started)
        {
            // Apply saved user mix at scene start so startup and settings-open behavior match.
            audio.SetMasterVolume(UserSettingsStore.GetSliderValue("Volume", 1.0f));
            audio.SetMusicVolume(UserSettingsStore.GetSliderValue("Music", 1.0f));
            audio.SetSfxVolume(UserSettingsStore.GetSliderValue("SFX", 0.25f));
            audio.PlayBGM();
            BGMManager.CurrentTrack = "General";
            Console.WriteLine("BGM: Playing General music");
            started = true;
        }

        
        bool isPaused = Application.IsPaused(); 

        if (isPaused && !wasPaused)
        {
            audio.PauseBGM();
            Console.WriteLine("BGM: Paused");
        }
        else if (!isPaused && wasPaused)
        {
            audio.ResumeBGM();
            Console.WriteLine("BGM: Resumed");
        }

        wasPaused = isPaused;
    }
}

// ============================================
// USE THIS FOR: Level1
// ============================================
public class LevelBGM : Script
{
    private bool started = false;
    private bool wasPaused = false;

    public override void Update()
    {
        // Wait until gameplay is ready before starting level BGM
        if (!GameStart.Ready) return;

        AudioComponent audio = GetAudio();

        if (!started)
        {
            // Apply saved user mix at scene start so startup and settings-open behavior match.
            audio.SetMasterVolume(UserSettingsStore.GetSliderValue("Volume", 1.0f));
            audio.SetMusicVolume(UserSettingsStore.GetSliderValue("Music", 1.0f));
            audio.SetSfxVolume(UserSettingsStore.GetSliderValue("SFX", 0.1f));

            audio.PlayBGM();
            //audio.PlayBGM("LVL 1 BGM 90 MILI OFF TEST");
            BGMManager.CurrentTrack = "Level";
            Console.WriteLine("BGM: Playing Level music");

            // Reset Conductor clock so beat tracking starts at t=0 to uh hopefully be in sync with audio if not i kms.
          
            Conductor.Instance?.ResetClock();

            started = true;
        }

        // Pause / Resume BGM based on game pause state
        bool isPaused = Application.IsPaused(); // If your API uses another getter, swap this line
        bool stageClearActive = StageClearElement.IsCleared;

        if (!stageClearActive && isPaused && !wasPaused)
        {
            audio.PauseBGM();   // If your API uses another name, replace here
            Console.WriteLine("BGM: Paused");
        }
        else if (!stageClearActive && !isPaused && wasPaused)
        {
            audio.ResumeBGM();  // If your API uses another name, replace here
            Console.WriteLine("BGM: Resumed");
        }

        // During stage clear, keep BGM running even though gameplay is paused.
        wasPaused = !stageClearActive && isPaused;
    }
}

// ============================================
// STATIC MANAGER
// ============================================
public static class BGMManager
{
    public static string CurrentTrack = "";
}

// ============================================
// USE THIS FOR: Cutscene (starting)
// ============================================
public class CutsceneBGM : Script
{
    private const float FadeInDuration  = 1.5f;
    private const float FadeOutDuration = 5.0f;

    private bool started     = false;
    private bool fadeOutDone = false;

    public override void Update()
    {
        AudioComponent audio = GetAudio();

        if (!started)
        {
            audio.SetMasterVolume(UserSettingsStore.GetSliderValue("Volume", 1.0f));
            audio.SetMusicVolume(UserSettingsStore.GetSliderValue("Music", 1.0f));
            audio.SetSfxVolume(UserSettingsStore.GetSliderValue("SFX", 0.25f));
            audio.PlayBGM("BGM_Cutscene");
            audio.FadeInMusic(FadeInDuration);
            BGMManager.CurrentTrack = "Cutscene";
            Console.WriteLine("BGM: Playing Cutscene music (fade in)");
            started = true;
        }

        // Begin fade-out FadeOutDuration seconds before the cutscene ends so it
        // finishes gracefully rather than being cut off by the scene load.
        if (!fadeOutDone && started &&
            (CutscenePanel.IsCutsceneFinished || CutscenePanel.TimeRemaining <= FadeOutDuration))
        {
            audio.FadeOutMusic(FadeOutDuration);
            fadeOutDone = true;
            Console.WriteLine($"[CutsceneBGM] Fading out over {FadeOutDuration}s");
        }
    }
}

// ============================================
// USE THIS FOR: EndingCutscene
// ============================================
public class EndingCutsceneBGM : Script
{
    private const float FadeInDuration  = 1.5f;
    private const float FadeOutDuration = 5.0f;

    private bool started     = false;
    private bool fadeOutDone = false;

    public override void Update()
    {
        AudioComponent audio = GetAudio();

        if (!started)
        {
            audio.SetMasterVolume(UserSettingsStore.GetSliderValue("Volume", 1.0f));
            audio.SetMusicVolume(UserSettingsStore.GetSliderValue("Music", 1.0f));
            audio.SetSfxVolume(UserSettingsStore.GetSliderValue("SFX", 0.25f));
            audio.PlayBGM("BGM_Cutscene");
            audio.FadeInMusic(FadeInDuration);
            BGMManager.CurrentTrack = "Cutscene";
            Console.WriteLine("BGM: Playing EndingCutscene music (fade in)");
            started = true;
        }

        // Begin fade-out FadeOutDuration seconds before the cutscene ends.
        if (!fadeOutDone && started &&
            (EndingCutscenePanel.IsCutsceneFinished || EndingCutscenePanel.TimeRemaining <= FadeOutDuration))
        {
            audio.FadeOutMusic(FadeOutDuration);
            fadeOutDone = true;
            Console.WriteLine($"[EndingCutsceneBGM] Fading out over {FadeOutDuration}s");
        }
    }
}

// ============================================
// USE THIS FOR: GameLose scene
// ============================================
public class GameLoseBGM : Script
{
    private bool started = false;

    public override void Update()
    {
        if (started) return;
        started = true;

        AudioComponent audio = GetAudio();
        audio.SetMasterVolume(UserSettingsStore.GetSliderValue("Volume", 1.0f));
        audio.SetMusicVolume(UserSettingsStore.GetSliderValue("Music", 1.0f));
        audio.SetSfxVolume(UserSettingsStore.GetSliderValue("SFX", 0.25f));
        audio.PlayBGM("BGM_Game_Lose");
        BGMManager.CurrentTrack = "GameLose";
        Console.WriteLine("BGM: Playing Game Lose music");
    }
}

public class LevelSelectBGM : Script
{
    
    private static readonly System.Collections.Generic.Dictionary<string, string> s_trackMap
        = new System.Collections.Generic.Dictionary<string, string>
    {
        { "Level1", "BGM_L1_TESTING" },
        { "Level2", "BGM_L2_TESTING" },
        { "Level3", "BGM_L3" },
    };

    private const float FadeDuration    = 1.5f;   // fade-in / fade-out length (seconds)
    private const float CrossFadeDuration = 0.6f; // quick crossfade when switching levels
    private const float PreviewDuration = 20.0f;  // seconds of full-volume play before looping

    private enum State { Idle, FadingOut, FadingIn, Playing }

    private State  _state           = State.Idle;
    private float  _timer           = 0.0f;
    private string _activeTrack     = "";
    private string _lastSelected    = "";
    private bool   _wasOverlayOpen  = false;
    private bool   _restoreGeneral  = false; // true = fade-out is heading back to general BGM

    public override void Update()
    {
        AudioComponent audio  = GetAudio();
        bool   overlayOpen    = NavigationButtons.ShowLevelSelectOverlay;
        string selected       = NavigationButtons.SelectedLevelScene;

        bool justOpened      = overlayOpen  && !_wasOverlayOpen;
        bool justClosed      = !overlayOpen && _wasOverlayOpen;
        bool selectionChanged = overlayOpen && selected != _lastSelected;

        _wasOverlayOpen = overlayOpen;

        
        if (justClosed)
        {
            _lastSelected  = "";
            _activeTrack   = "";
            _restoreGeneral = true;

            if (_state != State.Idle)
            {
                audio.FadeOutMusic(FadeDuration);
                _timer = FadeDuration;
                _state = State.FadingOut;
            }
            return;
        }

        
        if (justOpened || selectionChanged)
        {
            _lastSelected   = selected;
            _restoreGeneral = false;

            if (!s_trackMap.TryGetValue(selected, out string? newTrack))
            {
               
                if (_state != State.Idle)
                {
                    audio.FadeOutMusic(FadeDuration);
                    _activeTrack = "";
                    _timer = FadeDuration;
                    _state = State.FadingOut;
                }
                return;
            }

            if (newTrack != _activeTrack)
            {
                _activeTrack = newTrack;
                
                float fadeTime = selectionChanged && !justOpened ? CrossFadeDuration : FadeDuration;
                audio.FadeOutMusic(fadeTime);
                _timer = fadeTime;
                _state = State.FadingOut;
            }
            return;
        }

        
        if (_state == State.Idle)
            return;

        
        _timer -= Time.DeltaTime;

        switch (_state)
        {
            case State.FadingOut:
                if (_timer <= 0.0f)
                {
                    if (_restoreGeneral)
                    {
                        audio.PlayBGM("BGM_GENERAL");
                        audio.FadeInMusic(FadeDuration);
                        _restoreGeneral = false;
                        _state = State.Idle;
                    }
                    else if (!string.IsNullOrEmpty(_activeTrack))
                    {
                        audio.PlayBGM(_activeTrack);
                        audio.FadeInMusic(FadeDuration);
                        _timer = FadeDuration;
                        _state = State.FadingIn;
                    }
                    else
                    {
                        _state = State.Idle;
                    }
                }
                break;

            case State.FadingIn:
                if (_timer <= 0.0f)
                {
                    _timer = PreviewDuration;
                    _state = State.Playing;
                }
                break;

            case State.Playing:
                if (_timer <= 0.0f)
                {
                    audio.FadeOutMusic(FadeDuration);
                    _timer = FadeDuration;
                    _state = State.FadingOut;
                }
                break;
        }
    }
}
