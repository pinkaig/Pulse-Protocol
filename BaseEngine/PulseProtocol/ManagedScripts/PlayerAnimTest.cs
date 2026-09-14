using System;
using ScriptAPI;


/// Controls:
///   1 = Force Idle
///   2 = Trigger Attack animation
///   3 = Trigger Hurt animation
///   4 = Toggle auto-cycle (Idle1 -> Idle2 -> Attack -> Hurt -> repeat)
///

public class PlayerAnimTest : Script
{
    private const int KEY_1 = 49;
    private const int KEY_2 = 50;
    private const int KEY_3 = 51;
    private const int KEY_4 = 52;

    private bool initialized = false;
    private bool autoCycling = false;
    private float cycleTimer = 0f;
    private float cycleInterval = 2f;
    private int cycleIndex = 0;

    public override void Update()
    {
        if (!initialized)
        {
            Initialize();
            initialized = true;
            return;
        }

        if (Application.IsPaused())
            return;

        var input = GetInput();

        if (input.IsKeyTriggered(KEY_1))
        {
            Console.WriteLine("[AnimTest] >> Forcing Idle");
            autoCycling = false;
            if (PlayerAnimationController.Instance != null)
                PlayerAnimationController.Instance.ForceIdle();
            else
                Console.WriteLine("[AnimTest] ERROR: PlayerAnimationController.Instance is null!");
        }
        else if (input.IsKeyTriggered(KEY_2))
        {
            Console.WriteLine("[AnimTest] >> Triggering Attack");
            autoCycling = false;
            if (PlayerAnimationController.Instance != null)
                PlayerAnimationController.Instance.PlayAttack(ComboSystem.Instance?.GetExpectedCombo() ?? "");
            else
                Console.WriteLine("[AnimTest] ERROR: PlayerAnimationController.Instance is null!");
        }
        else if (input.IsKeyTriggered(KEY_3))
        {
            Console.WriteLine("[AnimTest] >> Triggering Hurt");
            autoCycling = false;
            if (PlayerAnimationController.Instance != null)
                PlayerAnimationController.Instance.PlayHurt();
            else
                Console.WriteLine("[AnimTest] ERROR: PlayerAnimationController.Instance is null!");
        }
        else if (input.IsKeyTriggered(KEY_4))
        {
            autoCycling = !autoCycling;
            cycleTimer = 0f;
            cycleIndex = 0;
            Console.WriteLine($"[AnimTest] Auto-cycle: {(autoCycling ? "ON" : "OFF")}");
        }

        if (autoCycling)
        {
            cycleTimer += Time.DeltaTime;
            if (cycleTimer >= cycleInterval)
            {
                cycleTimer = 0f;
                RunCycleStep();
            }
        }
    }

    private void Initialize()
    {
        Console.WriteLine("[AnimTest] ========================================");
        Console.WriteLine("[AnimTest] Player Animation Test Script");
        Console.WriteLine("[AnimTest] ========================================");
        Console.WriteLine("[AnimTest] 1 = Force Idle");
        Console.WriteLine("[AnimTest] 2 = Attack");
        Console.WriteLine("[AnimTest] 3 = Hurt");
        Console.WriteLine("[AnimTest] 4 = Toggle auto-cycle");
        Console.WriteLine("[AnimTest] ========================================");

        if (PlayerAnimationController.Instance != null)
            Console.WriteLine($"[AnimTest] PlayerAnimationController found, current state: {PlayerAnimationController.Instance.GetCurrentState()}");
        else
            Console.WriteLine("[AnimTest] WARNING: PlayerAnimationController.Instance not ready yet (will retry on key press)");
    }

    private void RunCycleStep()
    {
        if (PlayerAnimationController.Instance == null)
        {
            Console.WriteLine("[AnimTest] ERROR: Controller is null, stopping cycle");
            autoCycling = false;
            return;
        }

        var ctrl = PlayerAnimationController.Instance;

        switch (cycleIndex)
        {
            case 0:
                Console.WriteLine("[AnimTest] [Cycle] -> Force Idle (Idle1/Idle2 should alternate)");
                ctrl.ForceIdle();
                break;
            case 1:
                Console.WriteLine("[AnimTest] [Cycle] -> Attack");
                ctrl.PlayAttack(ComboSystem.Instance?.GetExpectedCombo() ?? "");
                break;
            case 2:
                Console.WriteLine("[AnimTest] [Cycle] -> Hurt");
                ctrl.PlayHurt();
                break;
            case 3:
                Console.WriteLine("[AnimTest] [Cycle] -> Force Idle (verify return from Hurt)");
                ctrl.ForceIdle();
                break;
        }

        cycleIndex = (cycleIndex + 1) % 4;
    }
}
