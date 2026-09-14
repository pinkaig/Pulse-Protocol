/*****************************************************************************
 * @file        GlowAPI.h
 * @project     Pulse Protocol
 * @author      Goh Pin Kai
 *
 * @brief       C++/CLI value struct exposing per-entity glow to C# scripts.
 *              Delegates intensity tween management to GraphicsManager so
 *              designers can call GlowTo once and let the engine interpolate.
 *
 * @copyright   Copyright (C) 2026
 *              DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without
 *              prior written consent is prohibited.
 *****************************************************************************/
#pragma once

namespace ScriptAPI
{
    public value struct GlowComponent
    {
    public:
        // Enable or disable the glow effect on this entity
        void SetEnabled(bool on);
        bool IsEnabled();

        // Set glow intensity instantly (0 = off, 1 = normal, 2+ = intense)
        void  SetIntensity(float v);
        float GetIntensity();

        // Set the glow color (each channel 0.0–1.0)
        void SetColor(float r, float g, float b);

        // Tween glow intensity from current to [intensity] over [duration] seconds
        void GlowTo(float intensity, float duration);

        // Returns true when no intensity tween is currently running
        bool IsDone();

    internal:
        GlowComponent(unsigned int ID);

    private:
        unsigned int entityID;
    };
}
