/*****************************************************************************
 * @file        ParticleSystem.cpp
 * @project     Pulse Protocol
 * @author		Goh Pin Kai
 *
 * @brief       Implements ParticleSystem pool management, burst spawning with random
 *              polar velocities, and per-frame simulation (lifetime countdown and
 *              position integration). Inactive particles are reused from the pool.
 *
 * @copyright  Copyright (C) 2026 DigiPen Institute of Technology.
 *             Reproduction or disclosure of this file or its contents without the
 *             prior written consent of DigiPen Institute of Technology is prohibited.
 ******************************************************************************/

#include "ParticleSystem.h"
#include <cstdlib>
#include <cmath>

float ParticleSystem::Rand01()
{
    return float(std::rand()) / float(RAND_MAX);
}

void ParticleSystem::CreatePool(size_t maxParticles)
{
    mParticles.clear();
    mParticles.resize(maxParticles); // fixed-size pool
}

void ParticleSystem::Clear()
{
    mParticles.clear();
}

Particle* ParticleSystem::FindFree()
{
    for (auto& p : mParticles)
        if (!p.active) return &p;
    return nullptr;
}

void ParticleSystem::SpawnBurst(const ParticleSpawnDesc& d)
{
    for (int i = 0; i < d.count; ++i)
    {
        Particle* p = FindFree();
        if (!p) break;

        p->active    = true;
        p->pos       = d.position;
        p->textureID = d.textureID;
        p->size      = d.sizeMin + (d.sizeMax - d.sizeMin) * Rand01();

        float a = Rand01() * 6.2831853f;
        float s = d.speedMin + (d.speedMax - d.speedMin) * Rand01();
        p->vel = glm::vec2(std::cos(a), std::sin(a)) * s;

        p->lifeMax = d.lifeMin + (d.lifeMax - d.lifeMin) * Rand01();
        p->life    = p->lifeMax;
    }
}

void ParticleSystem::Update(float dt)
{
    for (auto& p : mParticles)
    {
        if (!p.active) continue;

        p.life -= dt;
        if (p.life <= 0.0f)
        {
            p.active = false;
            continue;
        }

        p.pos += p.vel * dt;
    }
}
