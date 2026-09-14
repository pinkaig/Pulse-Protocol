/*****************************************************************************
 * @file        VFXAPI.h
 * @project     Pulse Protocol
 * @author		Goh Pin Kai
 *
 * @brief       C++/CLI bridge API for triggering gameplay VFX from managed code.
 *              All effects are explicitly opt-in — nothing fires automatically.
 *
 *  VFXAPI.BeatPulse()               — camera zoom punch
 *  VFXAPI.ShakeCamera(amp, decay)   — camera shake only
 *  VFXAPI.SpawnParticles(...)       — particle burst only (colored or textured)
 *
 * @copyright   Copyright (C) 2026
 *              DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without
 *              prior written consent is prohibited.
 ******************************************************************************/

#pragma once

namespace ScriptAPI
{
    public ref class VFXAPI abstract sealed
    {
    public:
        // Camera zoom punch (e.g. call from Conductor.cs on each beat)
        static void BeatPulse();

        // Camera shake only.
        // amplitude: NDC displacement strength (0.02 = subtle, 0.06 = strong)
        // decay: exponential falloff rate (higher = stops faster, ~8-14 is typical)
        static void ShakeCamera(float amplitude, float decay);

        // Particle burst only — colored circles (no texture).
        // x, y: world-space spawn position (same coordinate system as entity transforms)
        static void SpawnParticles(float x, float y, int count);

        // Particle burst only — textured quads.
        // textureName: asset ID registered in texture.json (e.g. "particle_star")
        // size: world-pixel size of each particle (e.g. 64 = about 64px at default zoom)
        static void SpawnParticles(float x, float y, int count, System::String^ textureName, float size);
    };
}
