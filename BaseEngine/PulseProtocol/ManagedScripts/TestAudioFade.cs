using System;
using ScriptAPI;

public class TestFadeKey : Script
{
    private bool triggered = false;

    public override void Update()
    {
        InputComponent input = GetInput();

        // F key
        if (!triggered && input.IsKeyTriggered(70))
        {
            triggered = true;
            Console.WriteLine("F pressed -> FadeOutMusic(2.0)");
            GetAudio().FadeOutMusic(2.0f);
        }
    }
}