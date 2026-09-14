using System;
using ScriptAPI;

// These will be where the cheat code are for:
//1.Kill enemy instatly.(999 DMG)
//2. Skip lvl(win) (smth + level number)
//3. Become invinsible

public class Cheats: Script
{
    // GLFW NOT ASCII
    public int Key1 = 49;
    public int Key2 = 50;
    public int Key3 = 51;
    public int KeyI = 73;
    public int KeyK = 75;
    public int KeyO = 79;
    public int KeyShift = 340;

    // fuck you I like it this neat
    public string Level1 = "Level1";
    public string Level2 = "Level2";
    public string Level3 = "Level3";
    public string LoadingScreen = "LoadingScreenScene";

    public static bool invincible = false;
    
    public override void Update()
    {
        if (!GameStart.Ready) return;

        InputComponent input = GetInput();

        // Level switch
        if(input.IsKeyPressed(KeyShift))
        {
            if(input.IsKeyTriggered(Key1))
            {
                LoadingScreenScript.NextLevel = Level1;
                Scene.LoadScene(LoadingScreen);
            }

            else if(input.IsKeyTriggered(Key2))
            {
                LoadingScreenScript.NextLevel = Level2;
                Scene.LoadScene(LoadingScreen);
            }

            else if(input.IsKeyTriggered(Key3))
            {
                LoadingScreenScript.NextLevel = Level3;
                Scene.LoadScene(LoadingScreen);
            }
        }

        // Clear wave / skip level
        if(input.IsKeyTriggered(KeyO) && !Application.IsPaused())
        {
            StageClearElement.StageClear();
        }

        // Kill current Enemy
        if(input.IsKeyTriggered(KeyK))
        {
            if(EnemyRhythmController.Instance != null)
            {
                EnemyRhythmController.Instance.TakeDamage(99999);
                EnemyRhythmController.Instance.TriggerHurtAnimation(); // pendingDeath is true so this skips hurt and goes straight to Die()
            }
            else
            {
                Console.WriteLine("There isn't an enemy on screen right now.....");
            }
        }

        // Make player invincible(take no dmg), Inside EnemyRhythmController.cs (AttackPlayer)
        if (input.IsKeyTriggered(KeyI))
        {
            invincible = (invincible? false : true);
            Console.WriteLine($"You are invisible? {invincible}!");
        }

    }
}
