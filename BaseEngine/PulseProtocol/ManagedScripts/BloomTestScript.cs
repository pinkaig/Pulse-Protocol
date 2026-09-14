/******************************************************************************/
/**
* @file        BloomTestScript.cs
* @project     Pulse Protocol
* @author      Goh Pin Kai
* @brief       Interactive test script for PostProcessAPI (bloom post-processing).
*              Attach to any entity in a scene and enter Play mode.
*
*  B      :  Toggle bloom ON / OFF
*  [      :  Decrease intensity  -0.2  (min 0.0)
*  ]      :  Increase intensity  +0.2  (max 3.0)
*  ;      :  Decrease threshold  -0.05 (min 0.0)
*  '      :  Increase threshold  +0.05 (max 1.0)
*  R      :  Reset to defaults   (intensity 1.2, threshold 0.5)
*  P      :  Print current state to console
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without
*              the prior written consent of DigiPen Institute of Technology
*              is prohibited.
/******************************************************************************/
using System;
using ScriptAPI;

public class BloomTestScript : Script
{
    // How often (seconds) to print ongoing state to console
    private float _statusInterval = 1.0f;
    private float _statusTimer    = 0.0f;

    public override void Start()
    {
        Console.WriteLine("[BloomTest] ============================================");
        Console.WriteLine("[BloomTest] POST-PROCESS BLOOM (PostProcessAPI)");
        Console.WriteLine("[BloomTest]   B  -> Toggle bloom ON / OFF");
        Console.WriteLine("[BloomTest]   [  -> Intensity  -0.2");
        Console.WriteLine("[BloomTest]   ]  -> Intensity  +0.2");
        Console.WriteLine("[BloomTest]   ;  -> Threshold  -0.05");
        Console.WriteLine("[BloomTest]   '  -> Threshold  +0.05");
        Console.WriteLine("[BloomTest]   R  -> Reset defaults (intensity 1.2, threshold 0.5)");
        Console.WriteLine("[BloomTest]   P  -> Print current state");
        Console.WriteLine("[BloomTest] ============================================");

        // Start with bloom enabled so the tester sees it immediately
        PostProcessAPI.SetBloomEnabled(true);
        PostProcessAPI.SetBloomIntensity(1.2f);
        PostProcessAPI.SetBloomThreshold(0.5f);
        PrintState();
    }

    public override void Update()
    {
        float dt    = Time.DeltaTime;
        var   input = GetInput();

        // ---- B : toggle bloom ----
        if (input.IsKeyTriggered(66))   // 'B'
        {
            bool nowOn = !PostProcessAPI.IsBloomEnabled();
            PostProcessAPI.SetBloomEnabled(nowOn);
            Console.WriteLine($"[BloomTest] >> Bloom {(nowOn ? "ENABLED" : "DISABLED")}");
        }

        // ---- [ : intensity down ----
        if (input.IsKeyTriggered(91))   // '['
        {
            float cur = PostProcessAPI.GetBloomIntensity();
            float next = MathF.Max(0.0f, cur - 0.2f);
            PostProcessAPI.SetBloomIntensity(next);
            Console.WriteLine($"[BloomTest] >> Intensity: {cur:F2} -> {next:F2}");
        }

        // ---- ] : intensity up ----
        if (input.IsKeyTriggered(93))   // ']'
        {
            float cur = PostProcessAPI.GetBloomIntensity();
            float next = MathF.Min(3.0f, cur + 0.2f);
            PostProcessAPI.SetBloomIntensity(next);
            Console.WriteLine($"[BloomTest] >> Intensity: {cur:F2} -> {next:F2}");
        }

        // ---- ; : threshold down ----
        if (input.IsKeyTriggered(59))   // ';'
        {
            float cur = PostProcessAPI.GetBloomThreshold();
            float next = MathF.Max(0.0f, cur - 0.05f);
            PostProcessAPI.SetBloomThreshold(next);
            Console.WriteLine($"[BloomTest] >> Threshold: {cur:F2} -> {next:F2}");
        }

        // ---- ' : threshold up ----
        if (input.IsKeyTriggered(39))   // '\''
        {
            float cur = PostProcessAPI.GetBloomThreshold();
            float next = MathF.Min(1.0f, cur + 0.05f);
            PostProcessAPI.SetBloomThreshold(next);
            Console.WriteLine($"[BloomTest] >> Threshold: {cur:F2} -> {next:F2}");
        }

        // ---- R : reset to defaults ----
        if (input.IsKeyTriggered(82))   // 'R'
        {
            PostProcessAPI.SetBloomEnabled(true);
            PostProcessAPI.SetBloomIntensity(1.2f);
            PostProcessAPI.SetBloomThreshold(0.5f);
            Console.WriteLine("[BloomTest] >> Reset to defaults");
            PrintState();
        }

        // ---- P : print state ----
        if (input.IsKeyTriggered(80))   // 'P'
        {
            PrintState();
        }

        // ---- Periodic status print (only when bloom is on) ----
        if (PostProcessAPI.IsBloomEnabled())
        {
            _statusTimer += dt;
            if (_statusTimer >= _statusInterval)
            {
                _statusTimer = 0.0f;
                PrintState();
            }
        }
    }

    private void PrintState()
    {
        bool  enabled   = PostProcessAPI.IsBloomEnabled();
        float intensity = PostProcessAPI.GetBloomIntensity();
        float threshold = PostProcessAPI.GetBloomThreshold();
        Console.WriteLine(
            $"[BloomTest] enabled={enabled} | intensity={intensity:F2} | threshold={threshold:F2}");
    }
}
