using System;
using ScriptAPI;

/// <summary>
/// MASTER DIAGNOSTIC SCRIPT
/// Attach this to any entity to check all game systems
/// Runs comprehensive checks and reports issues
/// </summary>
public class MasterDiagnostic : Script
{
    private int frameCount = 0;
    private const int CHECK_INTERVAL = 60; // Check every 60 frames (1 second at 60fps)
    
    public override void Update()
    {
        frameCount++;
        
        // Run diagnostic every second
        if (frameCount % CHECK_INTERVAL != 0) return;
        
        Console.WriteLine("\n╔════════════════════════════════════════════╗");
        Console.WriteLine("║      MASTER DIAGNOSTIC REPORT              ║");
        Console.WriteLine($"║      Frame: {Time.FrameCount,-6}  Time: {Time.GameTime:F1}s     ║");
        Console.WriteLine("╠════════════════════════════════════════════╣");
        
        // Core Systems
        CheckSystem("Conductor", Conductor.Instance != null, () => {
            if (Conductor.Instance != null)
                return $"Beat: {Conductor.Instance.GetCurrentBeat()}, BPM: {Conductor.Instance.BPM}";
            return "";
        });
        
        CheckSystem("TurnManager", TurnManager.Instance != null, () => {
            if (TurnManager.Instance != null)
                return $"Phase: {TurnManager.Instance.GetCurrentPhase()}";
            return "";
        });
        
        CheckSystem("ComboSystem", ComboSystem.Instance != null, () => {
            if (ComboSystem.Instance != null)
            {
                string expected = ComboSystem.Instance.GetExpectedCombo();
                if (string.IsNullOrEmpty(expected))
                    return "⚠️ NO WEAKNESS SET";
                return $"Expected: {expected}, Progress: {ComboSystem.Instance.GetComboProgress()}";
            }
            return "";
        });
        
        CheckSystem("Enemy", EnemyRhythmController.Instance != null, () => {
            if (EnemyRhythmController.Instance != null)
                return $"Combo: {EnemyRhythmController.Instance.WeaknessCombo}";
            return "";
        });
        
        CheckSystem("PlayerHealth", PlayerHealth.Instance != null, () => {
            if (PlayerHealth.Instance != null)
                return $"HP: {PlayerHealth.Instance.GetCurrentHP()}/{PlayerHealth.Instance.GetMaxHP()}";
            return "";
        });
        
        CheckSystem("InputBridge", true, () => "Checking inputs...");
        
        // UI Systems
        Console.WriteLine("╠════════════════════════════════════════════╣");
        Console.WriteLine("║ UI SYSTEMS                                 ║");
        Console.WriteLine("╠════════════════════════════════════════════╣");
        
        CheckSystem("ComboDisplayManager", ComboDisplayManager.Instance != null, () => "");
        
        // Check if key slots are registered
        for (int i = 0; i < 4; i++)
        {
            // We can't check the private dictionary, so just note it
            Console.WriteLine($"║ KeySlot {i}: (check console for registration msg)  ║");
        }
        
        CheckSystem("HealthBGController", HealthBGController.Instance != null, () => "");
        
        // Application State
        Console.WriteLine("╠════════════════════════════════════════════╣");
        Console.WriteLine("║ APPLICATION STATE                          ║");
        Console.WriteLine("╠════════════════════════════════════════════╣");
        
        Console.WriteLine($"║ FPS: {ProfilingAPI.GetFPS():F1}                                    ║");
        Console.WriteLine($"║ Paused: {(Application.IsPaused() ? "YES ⏸" : "NO ▶")}                           ║");
        Console.WriteLine($"║ DeltaTime: {Time.DeltaTime:F4}s                          ║");
        
        Console.WriteLine("╚════════════════════════════════════════════╝\n");
        
        // Warnings
        PrintWarnings();
    }
    
    private void CheckSystem(string name, bool exists, Func<string> getDetails)
    {
        string status = exists ? "✓" : "✗";
        string details = exists ? getDetails() : "MISSING";
        
        // Pad name to 20 chars
        string paddedName = name.PadRight(20);
        
        if (exists && !string.IsNullOrEmpty(details))
        {
            Console.WriteLine($"║ {status} {paddedName}                      ║");
            Console.WriteLine($"║   {details.PadRight(40)} ║");
        }
        else
        {
            Console.WriteLine($"║ {status} {paddedName} {details.PadRight(19)} ║");
        }
    }
    
    private void PrintWarnings()
    {
        bool hasWarnings = false;
        
        // Check for common issues
        if (ComboSystem.Instance != null && string.IsNullOrEmpty(ComboSystem.Instance.GetExpectedCombo()))
        {
            if (!hasWarnings)
            {
                Console.WriteLine("⚠️  WARNINGS:");
                hasWarnings = true;
            }
            Console.WriteLine("   - ComboSystem has no weakness set!");
            Console.WriteLine("   - Enemy should set this in TrySetWeakness()");
        }
        
        if (TurnManager.Instance == null)
        {
            if (!hasWarnings)
            {
                Console.WriteLine("⚠️  WARNINGS:");
                hasWarnings = true;
            }
            Console.WriteLine("   - TurnManager missing - turn system won't work");
        }
        
        if (Conductor.Instance == null)
        {
            if (!hasWarnings)
            {
                Console.WriteLine("⚠️  WARNINGS:");
                hasWarnings = true;
            }
            Console.WriteLine("   - Conductor missing - no beat tracking");
        }
        
        if (ComboDisplayManager.Instance == null)
        {
            if (!hasWarnings)
            {
                Console.WriteLine("⚠️  WARNINGS:");
                hasWarnings = true;
            }
            Console.WriteLine("   - ComboDisplayManager missing - no visual feedback");
        }
        
        if (PlayerHealth.Instance == null)
        {
            if (!hasWarnings)
            {
                Console.WriteLine("⚠️  WARNINGS:");
                hasWarnings = true;
            }
            Console.WriteLine("   - PlayerHealth missing - player can't take damage");
        }
        
        if (!hasWarnings)
        {
            Console.WriteLine("✓ No warnings - all systems operational!");
        }
        
        Console.WriteLine("");
    }
}
