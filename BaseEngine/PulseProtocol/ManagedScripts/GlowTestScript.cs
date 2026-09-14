/******************************************************************************/
/**
* @file        GlowTestScript.cs
* @project     Pulse Protocol
* @author      Goh Pin Kai
* @brief       Interactive test script for GlowComponent (per-entity glow halo).
*              Attach to any entity with a Renderable component and press the
*              keys below in Play mode.
*
*  --- ENABLE / DISABLE ---
*  E  :  Enable glow on this entity
*  D  :  Disable glow on this entity
*
*  --- COLORS ---
*  R  :  Red   glow
*  G  :  Green glow
*  B  :  Blue  glow
*  Y  :  Yellow glow  (R+G)
*  C  :  Cyan  glow   (G+B)
*  W  :  White glow   (R+G+B)
*
*  --- INSTANT INTENSITY ---
*  1  :  Intensity 0.5  (dim)
*  2  :  Intensity 1.0  (normal)
*  3  :  Intensity 2.0  (bright)
*  4  :  Intensity 4.0  (very bright)
*
*  --- TWEENS ---
*  Q  :  GlowTo(0.0, 1.0s)  — fade glow out over 1 s
*  F  :  GlowTo(1.0, 0.5s)  — fade to normal over 0.5 s
*  H  :  GlowTo(3.0, 0.3s)  — flash to intense over 0.3 s
*  P  :  Pulse sequence: flash bright then ease back
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
/******************************************************************************/
using System;
using ScriptAPI;

public class GlowTestScript : Script
{
    // Pulse sequence state machine
    private enum PulseState { Idle, FlashUp, EaseDown }
    private PulseState _pulse = PulseState.Idle;

    // How often to print status to console (seconds)
    private float _statusInterval = 0.5f;
    private float _statusTimer    = 0.0f;

    public override void Start()
    {
        Console.WriteLine("[GlowTest] ============================================");
        Console.WriteLine("[GlowTest] ENABLE/DISABLE");
        Console.WriteLine("[GlowTest]   E -> Enable glow");
        Console.WriteLine("[GlowTest]   D -> Disable glow");
        Console.WriteLine("[GlowTest] COLORS");
        Console.WriteLine("[GlowTest]   R -> Red    G -> Green   B -> Blue");
        Console.WriteLine("[GlowTest]   Y -> Yellow C -> Cyan    W -> White");
        Console.WriteLine("[GlowTest] INSTANT INTENSITY");
        Console.WriteLine("[GlowTest]   1 -> 0.5   2 -> 1.0   3 -> 2.0   4 -> 4.0");
        Console.WriteLine("[GlowTest] TWEENS");
        Console.WriteLine("[GlowTest]   Q -> GlowTo(0.0, 1.0s)  fade out");
        Console.WriteLine("[GlowTest]   F -> GlowTo(1.0, 0.5s)  normal");
        Console.WriteLine("[GlowTest]   H -> GlowTo(3.0, 0.3s)  flash");
        Console.WriteLine("[GlowTest]   P -> Pulse (flash->ease back)");
        Console.WriteLine("[GlowTest] ============================================");

        // Start enabled with a white glow so the effect is visible immediately
        var glow = GetGlow();
        glow.SetColor(1f, 1f, 1f);
        glow.SetIntensity(1.0f);
        glow.SetEnabled(true);
        Console.WriteLine("[GlowTest] Glow enabled (white, intensity 1.0)");
    }

    public override void Update()
    {
        float dt    = Time.DeltaTime;
        var   input = GetInput();
        var   glow  = GetGlow();

        // ---- Enable / Disable ----
        if (input.IsKeyTriggered(69))   // 'E'
        {
            glow.SetEnabled(true);
            Console.WriteLine("[GlowTest] >> Glow ENABLED");
        }

        if (input.IsKeyTriggered(68))   // 'D'
        {
            glow.SetEnabled(false);
            Console.WriteLine("[GlowTest] >> Glow DISABLED");
        }

        // ---- Color presets ----

        if (input.IsKeyTriggered(82))   // 'R'
        {
            glow.SetColor(1f, 0f, 0f);
            Console.WriteLine("[GlowTest] >> Color: Red");
        }

        if (input.IsKeyTriggered(71))   // 'G'
        {
            glow.SetColor(0f, 1f, 0f);
            Console.WriteLine("[GlowTest] >> Color: Green");
        }

        if (input.IsKeyTriggered(66))   // 'B'
        {
            glow.SetColor(0f, 0f, 1f);
            Console.WriteLine("[GlowTest] >> Color: Blue");
        }

        if (input.IsKeyTriggered(89))   // 'Y'
        {
            glow.SetColor(1f, 1f, 0f);
            Console.WriteLine("[GlowTest] >> Color: Yellow");
        }

        if (input.IsKeyTriggered(67))   // 'C'
        {
            glow.SetColor(0f, 1f, 1f);
            Console.WriteLine("[GlowTest] >> Color: Cyan");
        }

        if (input.IsKeyTriggered(87))   // 'W'
        {
            glow.SetColor(1f, 1f, 1f);
            Console.WriteLine("[GlowTest] >> Color: White");
        }

        // ---- Instant intensity ----

        if (input.IsKeyTriggered(49))   // '1'
        {
            glow.SetIntensity(0.5f);
            Console.WriteLine("[GlowTest] >> Intensity: 0.5 (dim)");
        }

        if (input.IsKeyTriggered(50))   // '2'
        {
            glow.SetIntensity(1.0f);
            Console.WriteLine("[GlowTest] >> Intensity: 1.0 (normal)");
        }

        if (input.IsKeyTriggered(51))   // '3'
        {
            glow.SetIntensity(2.0f);
            Console.WriteLine("[GlowTest] >> Intensity: 2.0 (bright)");
        }

        if (input.IsKeyTriggered(52))   // '4'
        {
            glow.SetIntensity(4.0f);
            Console.WriteLine("[GlowTest] >> Intensity: 4.0 (very bright)");
        }

        // ---- Tween keys ----

        if (input.IsKeyTriggered(81))   // 'Q'
        {
            glow.GlowTo(0.0f, 1.0f);
            _pulse = PulseState.Idle;
            Console.WriteLine("[GlowTest] >> GlowTo(0.0, 1.0s) — fading out");
        }

        if (input.IsKeyTriggered(70))   // 'F'
        {
            glow.GlowTo(1.0f, 0.5f);
            _pulse = PulseState.Idle;
            Console.WriteLine("[GlowTest] >> GlowTo(1.0, 0.5s) — easing to normal");
        }

        if (input.IsKeyTriggered(72))   // 'H'
        {
            glow.GlowTo(3.0f, 0.3f);
            _pulse = PulseState.Idle;
            Console.WriteLine("[GlowTest] >> GlowTo(3.0, 0.3s) — flash to intense");
        }

        if (input.IsKeyTriggered(80))   // 'P'
        {
            // Start pulse: flash up first
            glow.SetEnabled(true);
            glow.GlowTo(3.5f, 0.2f);
            _pulse = PulseState.FlashUp;
            Console.WriteLine("[GlowTest] >> Pulse started (flash -> ease back)");
        }

        // ---- Pulse state machine ----

        if (_pulse == PulseState.FlashUp && glow.IsDone())
        {
            // Flash reached peak — ease back to normal
            glow.GlowTo(0.8f, 0.6f);
            _pulse = PulseState.EaseDown;
            Console.WriteLine("[GlowTest] >> Pulse: peak reached, easing down");
        }
        else if (_pulse == PulseState.EaseDown && glow.IsDone())
        {
            _pulse = PulseState.Idle;
            Console.WriteLine("[GlowTest] >> Pulse: complete");
        }

        // ---- Periodic status print ----

        _statusTimer += dt;
        if (_statusTimer >= _statusInterval)
        {
            _statusTimer = 0.0f;

            bool  enabled   = glow.IsEnabled();
            float intensity = glow.GetIntensity();
            bool  done      = glow.IsDone();

            // Only print while something is active or tweening
            if (enabled || !done)
            {
                Console.WriteLine(
                    $"[GlowTest] enabled={enabled} intensity={intensity:F2} tweenDone={done}");
            }
        }
    }
}
