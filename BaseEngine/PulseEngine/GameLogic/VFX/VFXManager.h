/*****************************************************************************
 * @file        VFXManager.h
 * @project     Pulse Protocol
 * @author		Goh Pin Kai
 *
 * @brief       Central VFX controller for camera feedback and particle bursts.
 *              All effects are triggered explicitly via the VFXAPI - there is no
 *              automatic beat tracking. Reacts to hit judgements (MISS/GOOD/PERFECT)
 *              and manual beat pulses, producing camera punch/shake outputs and
 *              particle data for rendering.
 *
 * @copyright  Copyright (C) 2026 DigiPen Institute of Technology.
 *             Reproduction or disclosure of this file or its contents without the
 *             prior written consent of DigiPen Institute of Technology is prohibited.
 ******************************************************************************/

#pragma once
#include <string>
#include <glm/vec2.hpp>
#include "ParticleSystem.h"
#include "CameraEffects.h"

 //for .NET
#include "CoreEngine/Core/ImportExport.h" // for DLL_API
#pragma warning(push)
#pragma warning(disable: 4251)


class DLL_API VFXManager
{
public:
    void Init(size_t maxParticles);

    void Update(float dt);

    // Call explicitly to trigger a camera zoom punch (e.g. from Conductor.cs on each beat)
    void OnBeat();

    // --- Separate designer APIs ---
    // Trigger camera shake only (amplitude = NDC displacement strength, ~0.02-0.08)
    void ShakeCamera(float amplitude, float decay);

    // Spawn particles only (no shake). Fill desc with position, count, texture, size, speed.
    void SpawnParticles(const ParticleSpawnDesc& desc);

    // Convenience overload: resolves texture name to GL ID via mAssets on the native side.
    void SpawnParticles(float x, float y, int count, const std::string& textureName, float size);

    // Camera outputs (read by GraphicsManager each frame)
    float GetPunchZoom01() const { return mCamFx.GetPunchZoom01(); }
    glm::vec2 GetShakeOffset() const { return mCamFx.GetShakeOffset(); }

    const std::vector<Particle>& GetParticles() const { return mParticles.GetParticles(); }

private:
    ParticleSystem mParticles;
    CameraEffects  mCamFx;

    // Tuning per judgement
    void SpawnForMiss(const glm::vec2& p);
    void SpawnForGood(const glm::vec2& p);
    void SpawnForPerfect(const glm::vec2& p);
};
