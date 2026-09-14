using System;
using ScriptAPI;

/// <summary>
/// Debug script to diagnose initialization order issues.
/// Attach to the same entity as your game systems.
/// This will print the initialization status of each system.
/// </summary>
public class InitializationDiagnostic : Script
{
    private int frameCount = 0;
    private const int checkUntilFrame = 10; // Check for first 10 frames

    public override void Update()
    {
        frameCount++;

        // Run diagnostics for first few frames
        if (frameCount <= checkUntilFrame)
        {
            Console.WriteLine($"\n========== Frame {frameCount} Diagnostic ==========");
            
            // Check Conductor
            if (Conductor.Instance != null)
                Console.WriteLine($"[Diagnostic] ✅ Conductor.Instance exists (Beat: {Conductor.Instance.GetCurrentBeat()})");
            else
                Console.WriteLine("[Diagnostic] ❌ Conductor.Instance is NULL");

            // Check TurnManager
            if (TurnManager.Instance != null)
                Console.WriteLine($"[Diagnostic] ✅ TurnManager.Instance exists (Phase: {TurnManager.Instance.GetCurrentPhase()})");
            else
                Console.WriteLine("[Diagnostic] ❌ TurnManager.Instance is NULL");

            // Check ComboSystem
            if (ComboSystem.Instance != null)
            {
                Console.WriteLine($"[Diagnostic] ✅ ComboSystem.Instance exists");
                Console.WriteLine($"[Diagnostic]    Expected Combo: '{ComboSystem.Instance.GetExpectedCombo()}'");
                Console.WriteLine($"[Diagnostic]    Progress: {ComboSystem.Instance.GetComboProgress()}/{ComboSystem.Instance.maxComboLength}");
            }
            else
                Console.WriteLine("[Diagnostic] ❌ ComboSystem.Instance is NULL");

            // Check Enemy
            if (EnemyRhythmController.Instance != null)
                Console.WriteLine($"[Diagnostic] ✅ EnemyRhythmController.Instance exists (Combo: {EnemyRhythmController.Instance.WeaknessCombo})");
            else
                Console.WriteLine("[Diagnostic] ❌ EnemyRhythmController.Instance is NULL");

            Console.WriteLine("==========================================\n");
        }

        // Final summary at frame 10
        if (frameCount == checkUntilFrame)
        {
            Console.WriteLine("\n========== INITIALIZATION SUMMARY ==========");
            
            bool allGood = true;

            if (Conductor.Instance == null)
            {
                Console.WriteLine("[Diagnostic] ❌ CRITICAL: Conductor never initialized!");
                allGood = false;
            }

            if (TurnManager.Instance == null)
            {
                Console.WriteLine("[Diagnostic] ❌ CRITICAL: TurnManager never initialized!");
                allGood = false;
            }

            if (ComboSystem.Instance == null)
            {
                Console.WriteLine("[Diagnostic] ❌ CRITICAL: ComboSystem never initialized!");
                allGood = false;
            }
            else if (string.IsNullOrEmpty(ComboSystem.Instance.GetExpectedCombo()))
            {
                Console.WriteLine("[Diagnostic] ⚠️ WARNING: ComboSystem has no weakness set!");
                allGood = false;
            }

            if (EnemyRhythmController.Instance == null)
            {
                Console.WriteLine("[Diagnostic] ❌ CRITICAL: Enemy never initialized!");
                allGood = false;
            }

            if (allGood)
            {
                Console.WriteLine("[Diagnostic] ✅ ALL SYSTEMS INITIALIZED CORRECTLY!");
                Console.WriteLine($"[Diagnostic] Expected combo: {ComboSystem.Instance!.GetExpectedCombo()}");
                Console.WriteLine($"[Diagnostic] Current phase: {TurnManager.Instance!.GetCurrentPhase()}");
            }

            Console.WriteLine("==========================================\n");
        }
    }
}
