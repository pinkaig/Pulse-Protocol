using ScriptAPI;

// Hides the "Press ENTER to begin" prompt once calibration has started.
public class CalibrationInstructionText : Script
{
    public override void Update()
    {
        if (LatencyCalibrator.Instance == null) return;
        if (!LatencyCalibrator.Instance.IsCalibrating) return;

        GetTextComponent().SetVisible(false);
    }
}
