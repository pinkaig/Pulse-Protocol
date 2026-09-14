/******************************************************************************/
/**
* @file        FadeTestScript.cs
* @project     Pulse Protocol
* @author      Goh Pin Kai
* @brief       Interactive test script for FadeAPI (screen-wide) and
*              FadeComponent (per-entity). Attach to any entity with a
*              Renderable component and hit the keys below in Play mode.
*
*  --- SCREEN-WIDE FADE (FadeAPI) ---
*  1  :  Fade to BLACK   (2.0s)
*  2  :  Fade from BLACK (2.0s)
*  3  :  Fade to RED     (1.5s)
*  4  :  Stop/hide overlay instantly
*
*  --- PER-ENTITY FADE (FadeComponent) ---
*  F  :  FadeOut this entity (1.5s)
*  G  :  FadeIn  this entity (1.5s)
*  H  :  SetAlpha 0 instantly (invisible)
*  J  :  SetAlpha 1 instantly (fully visible)
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
/******************************************************************************/
using System;
using ScriptAPI;

public class FadeTestScript : Script
{
    // How often (seconds) to print ongoing-fade status to console
    private float _statusInterval = 0.5f;
    private float _statusTimer    = 0.0f;

    public override void Start()
    {
        Console.WriteLine("[FadeTest] ============================================");
        Console.WriteLine("[FadeTest] SCREEN-WIDE FADE (FadeAPI)");
        Console.WriteLine("[FadeTest]   1 -> FadeOut to Black (2.0s)");
        Console.WriteLine("[FadeTest]   2 -> FadeIn  from Black (2.0s)");
        Console.WriteLine("[FadeTest]   3 -> FadeOut to Red (1.5s)");
        Console.WriteLine("[FadeTest]   4 -> Stop overlay instantly");
        Console.WriteLine("[FadeTest] PER-ENTITY FADE (this entity)");
        Console.WriteLine("[FadeTest]   F -> FadeOut entity (1.5s)");
        Console.WriteLine("[FadeTest]   G -> FadeIn  entity (1.5s)");
        Console.WriteLine("[FadeTest]   H -> SetAlpha 0 (instant invisible)");
        Console.WriteLine("[FadeTest]   J -> SetAlpha 1 (instant visible)");
        Console.WriteLine("[FadeTest] ============================================");
    }

    public override void Update()
    {
        float dt    = Time.DeltaTime;
        var   input = GetInput();
        var   fade  = GetFade();

        // ---- Screen-wide fade keys ----

        if (input.IsKeyTriggered(49))   // '1'
        {
            Console.WriteLine("[FadeTest] >> FadeOut to Black (2.0s)");
            FadeAPI.FadeOut(2.0f);
        }

        if (input.IsKeyTriggered(50))   // '2'
        {
            Console.WriteLine("[FadeTest] >> FadeIn from Black (2.0s)");
            FadeAPI.FadeIn(2.0f);
        }

        if (input.IsKeyTriggered(51))   // '3'
        {
            Console.WriteLine("[FadeTest] >> FadeOut to Red (1.5s)");
            FadeAPI.FadeOut(1.5f, 1.0f, 0.0f, 0.0f);
        }

        if (input.IsKeyTriggered(52))   // '4'
        {
            Console.WriteLine("[FadeTest] >> FadeIn to Red (1.5s)");
            FadeAPI.FadeIn(1.5f, 1.0f, 0.0f, 0.0f);
        }

        if (input.IsKeyTriggered(53))   // '5'
        {
            Console.WriteLine("[FadeTest] >> Stop screen overlay");
            FadeAPI.Stop();
        }

        // ---- Per-entity fade keys ----

        if (input.IsKeyTriggered(70))   // 'F'
        {
            Console.WriteLine("[FadeTest] >> Entity FadeOut (1.5s)");
            fade.FadeOut(1.5f);
        }

        if (input.IsKeyTriggered(71))   // 'G'
        {
            Console.WriteLine("[FadeTest] >> Entity FadeIn (1.5s)");
            fade.FadeIn(1.5f);
        }

        if (input.IsKeyTriggered(72))   // 'H'
        {
            Console.WriteLine("[FadeTest] >> Entity SetAlpha(0) - instant invisible");
            fade.SetAlpha(0.0f);
        }

        if (input.IsKeyTriggered(74))   // 'J'
        {
            Console.WriteLine("[FadeTest] >> Entity SetAlpha(1) - instant visible");
            fade.SetAlpha(1.0f);
        }

        // ---- Periodic status print ----

        _statusTimer += dt;
        if (_statusTimer >= _statusInterval)
        {
            _statusTimer = 0.0f;

            bool screenDone  = FadeAPI.IsFadeDone();
            float screenAlpha = FadeAPI.GetAlpha();
            float entityAlpha = fade.GetAlpha();
            bool entityDone  = fade.IsDone();

            // Only print when something is actively fading
            if (!screenDone || !entityDone)
            {
                Console.WriteLine(
                    $"[FadeTest] Screen alpha={screenAlpha:F2} done={screenDone} | " +
                    $"Entity alpha={entityAlpha:F2} done={entityDone}");
            }
        }
    }
}
