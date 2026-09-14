/******************************************************************************/
/**
 * @file        StageClear.cs
 * @project     Pulse Protocol
 * @author      Leu Jun Yong (primary) - 100%
 * @brief       Tracks stage-clear state and coordinates related UI/game flow hooks.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/

using System;
using ScriptAPI;

public class StageClearElement : Script
{
    // =========================
    // GLOBAL STAGE CLEAR STATE
    // =========================
    public static bool IsCleared { get; private set; } = false;
    private static bool s_winBgmStarted = false;
    private static bool s_gameplaySfxStopped = false;

    public static void StageClear()
    {
        NavigationButtons.SetStageClearPaused(true);
        Application.SetPaused(true);
        GameStart.Ready = false; // freeze core gameplay loops that key off Ready
        NavigationButtons.ReportLevelCleared(Scene.GetCurrentScene());
        IsCleared = true;
        s_winBgmStarted = false;
        s_gameplaySfxStopped = false;
        Console.WriteLine("[StageClear]  StageClear() called");
    }

    public static void ResetStageClear()
    {
        NavigationButtons.SetStageClearPaused(false);
        IsCleared = false;
        s_winBgmStarted = false;
        s_gameplaySfxStopped = false;
        Console.WriteLine("[StageClear]  ResetStageClear()");
    }

    // =========================
    // PER-ENTITY SETTINGS
    // =========================
    public bool HideOnStart = true;

    private bool initialized = false;
    private bool shown = false;

    public override void Update()
    {
        if (!initialized)
        {
            Initialize();
            return;
        }

        // When StageClear is triggered, show THIS entity
        if (!shown && IsCleared)
        {
            Show();
        }

        // Safety: keep gameplay frozen while stage clear is active.
        if (IsCleared && !Application.IsPaused())
            Application.SetPaused(true);
    }

    private void Initialize()
    {
        if (HideOnStart && !IsCleared)
        {
            var tr = GetTransform();
            tr.IsVisible = false;
        }

        shown = IsCleared;
        if (shown) Show();

        initialized = true;
    }

    private void Show()
    {
        var tr = GetTransform();
        tr.IsVisible = true;
        shown = true;

        if (!s_gameplaySfxStopped)
        {
            s_gameplaySfxStopped = true;
            GetAudio().StopAllSFX();
        }

        if (!s_winBgmStarted)
        {
            s_winBgmStarted = true;
            GetAudio().PlayBGM("BGM_Game_Win");
            Console.WriteLine("[StageClear] Playing win BGM (looped): BGM_Game_Win");
        }

        Console.WriteLine("[StageClear] Showing Stage Clear UI element");
    }
}
