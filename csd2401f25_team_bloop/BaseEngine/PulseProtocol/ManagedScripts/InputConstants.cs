/******************************************************************************/
/**
* @file        InputConstants.cs
* @project     Pulse Protocol
* @author      Goh Pin Kai 
* @brief       Shared gamepad button index constants (GLFW gamepad mapping).
*              All scripts that need gamepad button indices should reference
*              these instead of defining their own private copies.
*
*              GLFW standard layout (PS4 / Xbox):
*                Cross  / A  = 0    Circle   / B = 1
*                Square / X  = 2    Triangle / Y = 3
*                L1 / LB     = 4    R1 / RB      = 5
*                Options/Start = 7
*                D-pad Up=11, Right=12, Down=13, Left=14
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/

public static class InputConstants
{
    // Face buttons
    public const int GP_CROSS     = 0;  // PS4: ×   Xbox: A
    public const int GP_CIRCLE    = 1;  // PS4: ○   Xbox: B
    public const int GP_SQUARE    = 2;  // PS4: □   Xbox: X
    public const int GP_TRIANGLE  = 3;  // PS4: △   Xbox: Y

    // Bumpers
    public const int GP_L1        = 4;  // PS4: L1  Xbox: LB
    public const int GP_R1        = 5;  // PS4: R1  Xbox: RB

    // System button
    public const int GP_START     = 7;  // PS4: Options  Xbox: Menu

    // D-pad
    public const int GP_DPAD_UP    = 11;
    public const int GP_DPAD_RIGHT = 12;
    public const int GP_DPAD_DOWN  = 13;
    public const int GP_DPAD_LEFT  = 14;
}
