using System;
using ScriptAPI;

public class DamageTestScript : Script
{
    public int testDamage = 10;

    public override void Update()
    {
        var input = GetInput();

        // Press T (keycode 84) to deal damage to the player
        if (input.IsKeyTriggered(84))
        {
            if (PlayerHealth.Instance != null)
            {
                PlayerHealth.Instance.TakeDamage(testDamage);
                Console.WriteLine($"[DamageTest] Dealt {testDamage} damage. HP: {PlayerHealth.Instance.GetCurrentHP()}/{PlayerHealth.Instance.GetMaxHP()}");
            }
            else
            {
                Console.WriteLine("[DamageTest] PlayerHealth.Instance is null!");
            }
        }
    }
}