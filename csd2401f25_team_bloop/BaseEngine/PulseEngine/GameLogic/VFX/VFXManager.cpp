/*****************************************************************************
 * @file        VFXManager.cpp
 * @project     Pulse Protocol
 * @author		Goh Pin Kai
 *
 * @brief       Implements VFXManager behavior: initializes particle pools,
 *              applies manual beat-driven camera punch, maps judgement strings
 *              to distinct particle/shake presets, and updates particles/camera
 *              each frame. All effects are triggered explicitly via VFXAPI -
 *              there is no automatic beat detection.
 *
 * @copyright  Copyright (C) 2026 DigiPen Institute of Technology.
 *             Reproduction or disclosure of this file or its contents without the
 *             prior written consent of DigiPen Institute of Technology is prohibited.
 ******************************************************************************/

#include "VFXManager.h"
#include "CoreEngine/Core/CoreEngine.h"  // for mAssets (GetOrLoadTexture)

void VFXManager::Init(size_t maxParticles)
{
    mParticles.CreatePool(maxParticles);
    mCamFx.SetBaseZoom(1.0f);
}

void VFXManager::OnBeat()
{
    PunchDesc punch;
    punch.zoomAmount = 0.06f;
    punch.zoomReturnSpeed = 12.0f;
    mCamFx.AddBeatPunch(punch);
}

void VFXManager::SpawnForMiss(const glm::vec2& p)
{
    ParticleSpawnDesc d;
    d.position = p;
    d.count = 10;
    d.speedMin = 80.0f;  d.speedMax = 250.0f;
    d.lifeMin = 0.15f;   d.lifeMax = 0.35f;
    mParticles.SpawnBurst(d);

    ShakeDesc s;
    s.amplitude = 0.06f;
    s.decay = 8.0f;
    mCamFx.AddShake(s);
}

void VFXManager::SpawnForGood(const glm::vec2& p)
{
    ParticleSpawnDesc d;
    d.position = p;
    d.count = 18;
    d.speedMin = 150.0f; d.speedMax = 350.0f;
    d.lifeMin = 0.20f;   d.lifeMax = 0.45f;
    mParticles.SpawnBurst(d);

    ShakeDesc s;
    s.amplitude = 0.045f;
    s.decay = 8.0f;
    mCamFx.AddShake(s);
}

void VFXManager::SpawnForPerfect(const glm::vec2& p)
{
    ParticleSpawnDesc d;
    d.position = p;
    d.count = 26;
    d.speedMin = 200.0f; d.speedMax = 500.0f;
    d.lifeMin = 0.25f;   d.lifeMax = 0.55f;
    mParticles.SpawnBurst(d);

    ShakeDesc s;
    s.amplitude = 0.030f;
    s.decay = 9.0f;
    mCamFx.AddShake(s);
}

void VFXManager::ShakeCamera(float amplitude, float decay)
{
    ShakeDesc s;
    s.amplitude = amplitude;
    s.decay     = decay;
    mCamFx.AddShake(s);
}

void VFXManager::SpawnParticles(const ParticleSpawnDesc& desc)
{
    mParticles.SpawnBurst(desc);
}

void VFXManager::SpawnParticles(float x, float y, int count, const std::string& textureName, float size)
{
    ParticleSpawnDesc d;
    d.position  = { x, y };
    d.count     = count;
    d.speedMin  = 80.0f;
    d.speedMax  = 300.0f;
    d.lifeMin   = 0.3f;
    d.lifeMax   = 0.6f;
    d.sizeMin   = size * 0.75f;
    d.sizeMax   = size * 1.25f;
    d.textureID = textureName.empty() ? 0u : mAssets.GetOrLoadTexture(textureName);

    mParticles.SpawnBurst(d);
}

void VFXManager::Update(float dt)
{
    mParticles.Update(dt);
    mCamFx.Update(dt);
}
