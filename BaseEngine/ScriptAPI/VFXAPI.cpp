/*****************************************************************************
 * @file        VFXAPI.cpp
 * @project     Pulse Protocol
 * @author		Goh Pin Kai
 *
 * @brief       Implements the VFXAPI managed-to-native forwarding layer.
 *
 * @copyright   Copyright (C) 2026
 *              DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without
 *              prior written consent is prohibited.
 ******************************************************************************/
#pragma once
#define generic generic_workaround
#include "VFXAPI.h"
#include "../GameLogic/GameLogic.h"
#include "../CoreEngine/ECS/Coordinator.h"
#include <msclr/marshal_cppstd.h>
#undef generic

namespace ScriptAPI
{
    void VFXAPI::BeatPulse()
    {
        auto* c = Coordinator::GetInstance();
        if (!c) return;
        auto logic = c->GetSystem<LogicSystem>();
        if (!logic) return;

        logic->GetVfx().OnBeat();
    }

    void VFXAPI::ShakeCamera(float amplitude, float decay)
    {
        auto* c = Coordinator::GetInstance();
        if (!c) return;
        auto logic = c->GetSystem<LogicSystem>();
        if (!logic) return;

        logic->GetVfx().ShakeCamera(amplitude, decay);
    }

    void VFXAPI::SpawnParticles(float x, float y, int count)
    {
        auto* c = Coordinator::GetInstance();
        if (!c) return;
        auto logic = c->GetSystem<LogicSystem>();
        if (!logic) return;

        ParticleSpawnDesc d;
        d.position  = glm::vec2{ x, y };
        d.count     = count;
        d.speedMin  = 80.0f;
        d.speedMax  = 300.0f;
        d.lifeMin   = 0.25f;
        d.lifeMax   = 0.55f;
        d.sizeMin   = 8.0f;
        d.sizeMax   = 16.0f;
        d.textureID = 0;  // rendered as colored circle
        logic->GetVfx().SpawnParticles(d);
    }

    void VFXAPI::SpawnParticles(float x, float y, int count, System::String^ textureName, float size)
    {
        auto* c = Coordinator::GetInstance();
        if (!c) return;
        auto logic = c->GetSystem<LogicSystem>();
        if (!logic) return;

        // Texture resolution happens natively in VFXManager using mAssets (no C++/CLI boundary issues)
        std::string name = System::String::IsNullOrEmpty(textureName)
            ? ""
            : msclr::interop::marshal_as<std::string>(textureName);
        logic->GetVfx().SpawnParticles(x, y, count, name, size);
    }
}
