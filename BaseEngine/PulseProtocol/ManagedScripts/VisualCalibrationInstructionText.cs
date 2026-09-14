using ScriptAPI;

// Hides the "Press ENTER to begin" prompt once visual calibration has started.
// Mirrors CalibrationInstructionText for the visual calibration scene.
public class VisualCalibrationInstructionText : Script
{
    public override void Update()
    {
        if (VisualLatencyCalibrator.Instance == null) return;
        if (!VisualLatencyCalibrator.Instance.IsAdjusting) return;

        GetTextComponent().SetVisible(false);
    }
}
