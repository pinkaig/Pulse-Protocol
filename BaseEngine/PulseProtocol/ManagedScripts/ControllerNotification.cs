using ScriptAPI;

// Attach to a text entity in any scene.
// Detects PS4 controller connect / disconnect and shows a timed on-screen message.
public class ControllerNotification : Script
{
    private bool prevConnected  = false;
    private bool initialized    = false;
    private float timer         = 0f;
    private const float DisplayDuration = 3f;

    public override void Update()
    {
        var   input = GetInput();
        var   text  = GetTextComponent();
        float dt    = Time.DeltaTime > 0f ? Time.DeltaTime : 0.016f; // count down even while paused

        bool connected = input.IsGamepadConnected;

        if (!initialized)
        {
            prevConnected = connected;
            initialized   = true;
            text.SetVisible(false);
            return;
        }

        if (connected && !prevConnected)
        {
            text.SetText("Controller connected");
            text.SetColor(0.2f, 1f, 0.4f);   // green
            text.SetVisible(true);
            timer = DisplayDuration;
        }
        else if (!connected && prevConnected)
        {
            text.SetText("Controller disconnected");
            text.SetColor(1f, 0.4f, 0.1f);   // orange-red
            text.SetVisible(true);
            timer = DisplayDuration;
        }

        prevConnected = connected;

        if (timer > 0f)
        {
            timer -= dt;
            if (timer <= 0f)
                text.SetVisible(false);
        }
    }
}
