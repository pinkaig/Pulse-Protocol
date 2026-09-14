/******************************************************************************/
/**
* @file        VFXTestScript.cs
* @project     Pulse Protocol
* @author      Goh Pin Kai
* @brief       Interactive test script for the VFX API.
*              Attach to any entity in the scene and enter Play mode.
*
*  --- CAMERA SHAKE ONLY ---
*  Z  :  ShakeCamera strong   (MISS-grade   0.08, decay 10)
*  X  :  ShakeCamera moderate (GOOD-grade   0.05, decay 8)
*  C  :  ShakeCamera light    (PERFECT-grade 0.02, decay 6)
*  V  :  BeatPulse — camera zoom punch
*
*  --- PARTICLES ONLY ---
*  Q  :  SpawnParticles MISS burst    — 10 colored circles
*  E  :  SpawnParticles GOOD burst    — 18 colored circles
*  T  :  SpawnParticles PERFECT burst — 26 colored circles
*  1  :  SpawnParticles — 20 colored circles at entity pos
*  2  :  SpawnParticles — 12 textured quads ("particle" asset) size 64
*
*  --- SCREEN FADE (FadeAPI — fullscreen overlay) ---
*  3  :  FadeOut to black (1.0 s)
*  4  :  FadeIn  from black (1.0 s)
*  5  :  FadeOut to red (0.5 s)
*  6  :  Stop  — kill overlay immediately
*
*  --- ENTITY FADE (FadeComponent — this entity's alpha) ---
*  7  :  FadeOut — this entity fades to 0 alpha (1.0 s)
*  8  :  FadeIn  — this entity fades to 1 alpha  (1.0 s)
*  9  :  FadeTo 50% opacity (1.0 s)
*  0  :  SetAlpha 1.0 — instantly restore full opacity
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
******************************************************************************/
using System;
using ScriptAPI;

public class VFXTestScript : Script
{
    private float _statusTimer    = 0.0f;
    private float _statusInterval = 2.0f;

    public override void Start()
    {
        Console.WriteLine("[VFXTest] =============================================");
        Console.WriteLine("[VFXTest] VFX API Test Script — keys active in Play");
        Console.WriteLine("[VFXTest] --- CAMERA SHAKE ONLY ---");
        Console.WriteLine("[VFXTest]   Z -> ShakeCamera strong   (MISS-grade)");
        Console.WriteLine("[VFXTest]   X -> ShakeCamera moderate (GOOD-grade)");
        Console.WriteLine("[VFXTest]   C -> ShakeCamera light    (PERFECT-grade)");
        Console.WriteLine("[VFXTest]   V -> BeatPulse (zoom punch)");
        Console.WriteLine("[VFXTest] --- PARTICLES ONLY ---");
        Console.WriteLine("[VFXTest]   Q -> SpawnParticles MISS  burst (10 colored)");
        Console.WriteLine("[VFXTest]   E -> SpawnParticles GOOD  burst (18 colored)");
        Console.WriteLine("[VFXTest]   T -> SpawnParticles PERFECT burst (26 colored)");
        Console.WriteLine("[VFXTest]   1 -> SpawnParticles 20 colored circles");
        Console.WriteLine("[VFXTest]   2 -> SpawnParticles textured (asset: 'particle')");
        Console.WriteLine("[VFXTest] --- SCREEN FADE (FadeAPI) ---");
        Console.WriteLine("[VFXTest]   3 -> FadeOut to black (1.0s)");
        Console.WriteLine("[VFXTest]   4 -> FadeIn  from black (1.0s)");
        Console.WriteLine("[VFXTest]   5 -> FadeOut to red (0.5s)");
        Console.WriteLine("[VFXTest]   6 -> Stop overlay");
        Console.WriteLine("[VFXTest] --- ENTITY FADE (FadeComponent) ---");
        Console.WriteLine("[VFXTest]   7 -> FadeOut this entity (1.0s)");
        Console.WriteLine("[VFXTest]   8 -> FadeIn  this entity (1.0s)");
        Console.WriteLine("[VFXTest]   9 -> FadeTo 50% opacity  (1.0s)");
        Console.WriteLine("[VFXTest]   0 -> SetAlpha 1.0 (instant restore)");
        Console.WriteLine("[VFXTest] =============================================");
    }

    public override void Update()
    {
        var input  = GetInput();
        var fade   = GetFade();
        float spawnX = GetTransform().X;
        float spawnY = GetTransform().Y;

        // --- Camera shake only ---

        if (input.IsKeyTriggered(90))   // Z — MISS-grade shake
        {
            Console.WriteLine("[VFXTest] ShakeCamera MISS-grade");
            VFXAPI.ShakeCamera(0.08f, 10.0f);
        }

        if (input.IsKeyTriggered(88))   // X — GOOD-grade shake
        {
            Console.WriteLine("[VFXTest] ShakeCamera GOOD-grade");
            VFXAPI.ShakeCamera(0.05f, 8.0f);
        }

        if (input.IsKeyTriggered(67))   // C — PERFECT-grade shake
        {
            Console.WriteLine("[VFXTest] ShakeCamera PERFECT-grade");
            VFXAPI.ShakeCamera(0.02f, 6.0f);
        }

        if (input.IsKeyTriggered(86))   // V — beat zoom punch
        {
            Console.WriteLine("[VFXTest] BeatPulse");
            VFXAPI.BeatPulse();
        }

        // --- Particles only ---

        if (input.IsKeyTriggered(81))   // Q — MISS particle burst
        {
            Console.WriteLine($"[VFXTest] SpawnParticles MISS burst at ({spawnX:F1}, {spawnY:F1})");
            VFXAPI.SpawnParticles(spawnX, spawnY, 10);
        }

        if (input.IsKeyTriggered(69))   // E — GOOD particle burst
        {
            Console.WriteLine($"[VFXTest] SpawnParticles GOOD burst at ({spawnX:F1}, {spawnY:F1})");
            VFXAPI.SpawnParticles(spawnX, spawnY, 18);
        }

        if (input.IsKeyTriggered(84))   // T — PERFECT particle burst
        {
            Console.WriteLine($"[VFXTest] SpawnParticles PERFECT burst at ({spawnX:F1}, {spawnY:F1})");
            VFXAPI.SpawnParticles(spawnX, spawnY, 26);
        }

        if (input.IsKeyTriggered(49))   // 1 — colored circles
        {
            Console.WriteLine($"[VFXTest] SpawnParticles (colored) at ({spawnX:F1}, {spawnY:F1})");
            VFXAPI.SpawnParticles(spawnX, spawnY, 20);
        }

        if (input.IsKeyTriggered(50))   // 2 — textured quads
        {
            Console.WriteLine($"[VFXTest] SpawnParticles (textured 'particle') at ({spawnX:F1}, {spawnY:F1})");
            VFXAPI.SpawnParticles(spawnX, spawnY, 12, "particle", 164f);
        }

        // --- Screen fade (FadeAPI — fullscreen overlay) ---

        if (input.IsKeyTriggered(51))   // 3 — fade to black
        {
            Console.WriteLine("[VFXTest] FadeAPI.FadeOut black (1.0s)");
            FadeAPI.FadeOut(1.0f);
        }

        if (input.IsKeyTriggered(52))   // 4 — fade in from black
        {
            Console.WriteLine("[VFXTest] FadeAPI.FadeIn from black (1.0s)");
            FadeAPI.FadeIn(1.0f);
        }

        if (input.IsKeyTriggered(53))   // 5 — fade to red
        {
            Console.WriteLine("[VFXTest] FadeAPI.FadeOut red (0.5s)");
            FadeAPI.FadeOut(0.5f, 1.0f, 0.0f, 0.0f);
        }

        if (input.IsKeyTriggered(54))   // 6 — stop overlay
        {
            Console.WriteLine("[VFXTest] FadeAPI.Stop");
            FadeAPI.Stop();
        }

        // --- Entity fade (FadeComponent — this entity's sprite alpha) ---

        if (input.IsKeyTriggered(55))   // 7 — entity fade out
        {
            Console.WriteLine("[VFXTest] FadeComponent.FadeOut (1.0s)");
            fade.FadeOut(1.0f);
        }

        if (input.IsKeyTriggered(56))   // 8 — entity fade in
        {
            Console.WriteLine("[VFXTest] FadeComponent.FadeIn (1.0s)");
            fade.FadeIn(1.0f);
        }

        if (input.IsKeyTriggered(57))   // 9 — entity fade to 50%
        {
            Console.WriteLine("[VFXTest] FadeComponent.FadeTo 0.5 (1.0s)");
            fade.FadeTo(0.5f, 1.0f);
        }

        if (input.IsKeyTriggered(48))   // 0 — instant restore alpha
        {
            Console.WriteLine("[VFXTest] FadeComponent.SetAlpha 1.0 (instant)");
            fade.SetAlpha(1.0f);
        }

        // --- Periodic status ---

        _statusTimer += Time.DeltaTime;
        if (_statusTimer >= _statusInterval)
        {
            _statusTimer = 0.0f;
            Console.WriteLine(
                $"[VFXTest] Status | " +
                $"EntityPos=({spawnX:F1},{spawnY:F1}) | " +
                $"ScreenFadeDone={FadeAPI.IsFadeDone()} ScreenAlpha={FadeAPI.GetAlpha():F2} | " +
                $"EntityAlpha={fade.GetAlpha():F2} EntityFadeDone={fade.IsDone()} | " +
                $"Frame={Time.FrameCount}");
        }
    }
}
