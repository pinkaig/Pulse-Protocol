using ScriptAPI;

// Mirrors CalibrationDisplay but for the visual latency calibrator.
// Attach to a TextComponent entity in the VisualLatencyCalibration scene.
public class VisualCalibrationDisplay : Script
{
    public override void Update()
    {
        if (VisualLatencyCalibrator.Instance == null) return;

        var text = GetTextComponent();

        if (VisualLatencyCalibrator.Instance.IsDone)
        {
            text.SetText($"Saved!   Display offset: {VisualLatencyCalibrator.Instance.GetOffsetMs():F0} ms");
            text.SetVisible(true);
        }
        else if (VisualLatencyCalibrator.Instance.IsAdjusting)
        {
            text.SetText(VisualLatencyCalibrator.Instance.GetOffsetLabel());
            text.SetVisible(true);
        }
        else
        {
            text.SetVisible(false);
        }
    }
}
