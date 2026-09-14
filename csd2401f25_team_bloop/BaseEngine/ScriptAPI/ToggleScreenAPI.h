/******************************************************************************/
/**
 * @file        ToggleScreenAPI.h
 * @project     Pulse Protocol
 * @author      Leu Jun Yong (primary) - 100%
 *
 * @brief       Declared the ToggleScreen ScriptAPI interface for use in C#.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/
#pragma once
using namespace System;

// =========================================================
// ToggleScreenAPI
// - Simple global (static) API for toggling fullscreen
// - Uses CoreEngine::ToggleFullscreen() + IsFullscreen()
// =========================================================

namespace ScriptAPI
{
    public ref class ToggleScreenAPI abstract sealed
    {
    public:
        // Toggle fullscreen <-> windowed
        static void ToggleFullscreen();

        // Convenience: ensure a specific state
        static void SetFullscreen(bool fullscreen);

        // Read current state
        static bool IsFullscreen();
    };
}
