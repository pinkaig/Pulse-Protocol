using System;
using ScriptAPI;

public class DebugEnemyCheats : Script
{
    public int KillKey = 75;

    // PS4 controller: hold L1 + R1, then press Triangle
    private const int GP_TRIANGLE = InputConstants.GP_TRIANGLE;
    private const int GP_L1       = InputConstants.GP_L1;
    private const int GP_R1       = InputConstants.GP_R1;

    private bool m_started = false;

    public override void Update()
    {
        if (!m_started)
        {
            m_started = true;
            Console.WriteLine("[Cheat] DebugEnemyCheats running — L1+R1+Triangle to kill enemy");
        }

        try
        {
            InputComponent inp = GetInput();

            bool l1  = inp.IsGamepadButtonPressed(GP_L1);
            bool r1  = inp.IsGamepadButtonPressed(GP_R1);
            bool tri = inp.IsGamepadButtonTriggered(GP_TRIANGLE);

            bool killTriggered = inp.IsKeyTriggered(KillKey) || (l1 && r1 && tri);

            if (killTriggered)
            {
                if (EnemyRhythmController.Instance == null)
                {
                    Console.WriteLine("[Cheat] No active enemy to kill.");
                    return;
                }

                Console.WriteLine("[Cheat] Forcing enemy death...");
                EnemyRhythmController.Instance!.TakeDamage(999999f);
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[Cheat] Exception: {ex.Message}");
        }
    }
}