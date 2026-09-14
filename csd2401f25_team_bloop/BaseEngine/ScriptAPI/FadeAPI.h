/*****************************************************************************
 * @file        FadeAPI.h
 * @project     Pulse Protocol
 * @author      Goh Pin Kai
 *
 * @brief       C++/CLI static API for screen-wide fade transitions.
 *              Renders a fullscreen colored overlay on top of all game
 *              content, letting designers drive scene transitions and
 *              dramatic effects from C# scripts.
 *
 *              FadeOut: overlay alpha 0 -> 1 (screen fills with color)
 *              FadeIn:  overlay alpha 1 -> 0 (screen clears from color)
 *
 * @copyright   Copyright (C) 2026
 *              DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without
 *              prior written consent is prohibited.
 *****************************************************************************/
#pragma once

namespace ScriptAPI
{
    public ref class FadeAPI abstract sealed
    {
    public:
        // Fade the screen OUT to [r,g,b] color over [duration] seconds
        // (overlay alpha 0 -> 1, screen becomes opaque)
        static void FadeOut(float duration, float r, float g, float b);

        // Fade the screen IN from [r,g,b] color over [duration] seconds
        // (overlay alpha 1 -> 0, screen clears)
        static void FadeIn(float duration, float r, float g, float b);

        // Convenience overloads — default color is black
        static void FadeOut(float duration);
        static void FadeIn(float duration);

        // Returns true once the current fade has finished
        static bool IsFadeDone();

        // Read the current overlay alpha (0 = transparent, 1 = opaque)
        static float GetAlpha();

        // Set overlay alpha instantly with no tween
        static void SetAlpha(float alpha);

        // Immediately stop and hide the overlay
        static void Stop();
    };
}
