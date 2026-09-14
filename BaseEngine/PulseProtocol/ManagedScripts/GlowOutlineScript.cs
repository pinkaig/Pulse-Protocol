/******************************************************************************/
/**
* @file        GlowOutlineScript.cs
* @project     Pulse Protocol
* @author      Goh Pin Kai
* @brief       make key outline glow
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
/******************************************************************************/
using System;
using ScriptAPI;

public class GlowOutlineScript : Script
{
    bool applied = false;

    public override void Start()
    {
        // optional: try once here, but don't rely on it
        TryApplyGlow();
    }

    public override void Update()
    {
        if (applied) return;

        // Keep trying for a few frames until engine systems are ready
        if (TryApplyGlow())
            applied = true;
    }

    private bool TryApplyGlow()
    {
        var glow = GetGlow();

        glow.SetColor(1.0f, 0.9f, 0.0f);
        glow.SetEnabled(true);
        glow.SetIntensity(6.0f);

        return glow.IsEnabled() && glow.GetIntensity() > 0.0f;
    }
}
