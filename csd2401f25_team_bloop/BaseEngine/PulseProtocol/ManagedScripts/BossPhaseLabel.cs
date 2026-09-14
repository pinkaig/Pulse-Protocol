using System;
using ScriptAPI;

public class BossPhaseLabel : Script
{
    private bool initialized    = false;
    private int  lastPhase      = 0;
    private bool prevGameReady  = false;

    public override void Update()
    {
        // Detect new play session
        if (initialized && prevGameReady && !GameStart.Ready)
        {
            initialized = false;
        }
        prevGameReady = GameStart.Ready;

        if (!initialized)
        {
            Initialize();
            return;
        }

        int phase = BossEnemyController.BossPhase;

        if (phase != lastPhase)
        {
            lastPhase = phase;
            UpdateLabel(phase);
        }
    }

    private void Initialize()
    {
        initialized = true;

        BossEnemyController.ResetPhase();
        lastPhase = 0;

        var text = GetTextComponent();
        text.SetAlignment(1);
        text.SetColor(1f, 1f, 1f);
        text.SetVisible(false);
    }

    private void UpdateLabel(int phase)
    {
        var text = GetTextComponent();
        switch (phase)
        {
            case 1:
                text.SetText("Phase 1");
                text.SetVisible(true);
                break;
            case 2:
                text.SetText("Phase 2");
                text.SetVisible(true);
                break;
            default:
                text.SetVisible(false);
                break;
        }
    }
}
