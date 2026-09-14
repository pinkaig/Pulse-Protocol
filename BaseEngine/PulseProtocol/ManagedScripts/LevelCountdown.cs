
using System;
using ScriptAPI;

public static class GameStart
{
    public static bool Ready = false;
}

public class LevelCountdown : Script
{
    private bool started = false;
    private float t = 0f;
    private int step = 3; // 3,2,1,0(GO),-1(done)
   // private TextComponentAPI text;

    public float StepSeconds = 1.0f;
    public float GoSeconds = 0.6f;

    public override void Update()
    {

        var text = GetTextComponent();
        if (LoadingScreenScript.BlockLevelStartCountdown)
        {
            text.SetVisible(false);
            return;
        }

        if (!started)
        {
            started = true;
            GameStart.Ready = false;


            //if (text != null)
            // {
            text.SetVisible(true);
            text.SetText("3");
            // text.SetColor(1f, 0.3f, 0.3f);
           // }

            t = 0f;
            step = 3;
            return; // IMPORTANT: don't advance timer on the same frame
        }

        float dt = Time.DeltaTime;

        // Hide/show countdown text based on pause overlay
        if (NavigationButtons.ShowPauseOverlay)
        {
            return;
        }
        else
        {
            // Only restore visibility if countdown is still running
            if (step >= 0)
                text.SetVisible(true);  // show again when unpaused
        }
        
        if (dt == 0f) dt = 0.016f; // keep ticking even if engine paused
        t += dt;

        float limit = (step == 0) ? GoSeconds : StepSeconds;
        if (t < limit) return;

        t = 0f;
        step--;

        if (step > 0)
        {
            Console.WriteLine("TEXT VISBILE OR NOT : " + text.IsVisible());
            Console.WriteLine("TEXT data  : " + text.GetText());
            text.SetText(step.ToString());
            // if (step == 3) text.SetColor(1f, 0.3f, 0.3f); // red
            // else if (step == 2) text.SetColor(1f, 0.6f, 0.2f); // orange
            // else if (step == 1) text.SetColor(1f, 1f, 0.2f);   // yellow
        }
        else if (step == 0)
        {
            text.SetText("GO!");
            // text.SetColor(0.3f, 1f, 0.3f); // green
        }
        else
        {
            text.SetVisible(false);
            GameStart.Ready = true;
            //Console.WriteLine("GameStart.Ready is true");
        }
    }
}
