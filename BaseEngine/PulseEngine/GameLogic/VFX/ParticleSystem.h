/*****************************************************************************
 * @file        ParticleSystem.h
 * @project     Pulse Protocol
 * @author		Goh Pin Kai
 *
 * @brief       Simple CPU particle system using a fixed-size pool. Supports spawning
 *              burst emissions with randomized direction/speed and lifetime ranges,
 *              and updates active particles by integrating velocity and lifetime.
 *              Particles optionally carry a texture ID for sprite-based rendering.
 *
 * @copyright  Copyright (C) 2026 DigiPen Institute of Technology.
 *             Reproduction or disclosure of this file or its contents without the
 *             prior written consent of DigiPen Institute of Technology is prohibited.
 ******************************************************************************/

#pragma once
#include <vector>
#include <glm/vec2.hpp>
#include <cstddef>

struct Particle
{
    bool         active    = false;
    glm::vec2    pos       { 0, 0 };
    glm::vec2    vel       { 0, 0 };
    float        life      = 0.0f;
    float        lifeMax   = 0.0f;
    float        size      = 12.0f;   // world-pixel radius/half-size
    unsigned int textureID = 0;       // 0 = colored circle, >0 = textured quad
};

struct ParticleSpawnDesc
{
    glm::vec2    position  { 0, 0 };
    int          count     = 12;
    float        speedMin  = 0.3f;
    float        speedMax  = 1.2f;
    float        lifeMin   = 0.25f;
    float        lifeMax   = 0.6f;
    float        sizeMin   = 8.0f;    // world pixels
    float        sizeMax   = 16.0f;   // world pixels
    unsigned int textureID = 0;       // 0 = colored circle, >0 = textured quad
};

class ParticleSystem
{
public:
    void CreatePool(size_t maxParticles);
    void Clear();

    void SpawnBurst(const ParticleSpawnDesc& d);
    void Update(float dt);

    const std::vector<Particle>& GetParticles() const { return mParticles; }

private:
    std::vector<Particle> mParticles;

    float Rand01();
    Particle* FindFree();
};
