/******************************************************************************/
/**
* @file        PauseLabels.cs
* @project     Pulse Protocol
* @author      Chloe Lau Rey En
* @brief       Animates the pause screen background with randomised frame timing,
*              including occasional freezes and flickers for an organic feel.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/

using System;
using ScriptAPI;

public class PauseLabels : Script
{
    public override void Update()
    {
        GetTextComponent().SetVisible(NavigationButtons.ShowPauseOverlay);
    }
}