/*****************************************************************************
 * @file        GlowAPI.cpp
 * @project     Pulse Protocol
 * @author      Goh Pin Kai
 *
 * @brief       Implementation of the GlowComponent C++/CLI bridge.
 *              Delegates glow state to Framework::Renderable::GlowState and
 *              intensity tweens to GraphicsManager.
 *
 * @copyright   Copyright (C) 2026
 *              DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without
 *              prior written consent is prohibited.
 *****************************************************************************/
#pragma once
#define generic generic_workaround
#include "GlowAPI.h"
#include "../CoreEngine/ECS/Coordinator.h"
#include "../Graphics/GraphicsManager.h"
#include "../Graphics/Renderable.h"
#undef generic

namespace ScriptAPI
{
    GlowComponent::GlowComponent(unsigned int ID) : entityID(ID) {}

    void GlowComponent::SetEnabled(bool on)
    {
        auto* coord = Coordinator::GetInstance();
        if (!coord) return;
        auto gfx = coord->GetSystem<GraphicsManager>();
        if (!gfx) return;
        gfx->SetEntityGlowEnabled(static_cast<Entity>(entityID), on);
    }

    bool GlowComponent::IsEnabled()
    {
        auto* coord = Coordinator::GetInstance();
        if (!coord) return false;
        if (!coord->HasComponent<Framework::Renderable>(static_cast<Entity>(entityID)))
            return false;
        return coord->GetComponent<Framework::Renderable>(
            static_cast<Entity>(entityID)).glow.enabled;
    }

    void GlowComponent::SetIntensity(float v)
    {
        auto* coord = Coordinator::GetInstance();
        if (!coord) return;
        auto gfx = coord->GetSystem<GraphicsManager>();
        if (!gfx) return;
        gfx->SetEntityGlowIntensity(static_cast<Entity>(entityID), v);
    }

    float GlowComponent::GetIntensity()
    {
        auto* coord = Coordinator::GetInstance();
        if (!coord) return 0.0f;
        if (!coord->HasComponent<Framework::Renderable>(static_cast<Entity>(entityID)))
            return 0.0f;
        return coord->GetComponent<Framework::Renderable>(
            static_cast<Entity>(entityID)).glow.intensity;
    }

    void GlowComponent::SetColor(float r, float g, float b)
    {
        auto* coord = Coordinator::GetInstance();
        if (!coord) return;
        auto gfx = coord->GetSystem<GraphicsManager>();
        if (!gfx) return;
        gfx->SetEntityGlowColor(static_cast<Entity>(entityID), r, g, b);
    }

    void GlowComponent::GlowTo(float intensity, float duration)
    {
        auto* coord = Coordinator::GetInstance();
        if (!coord) return;
        auto gfx = coord->GetSystem<GraphicsManager>();
        if (!gfx) return;
        gfx->GlowTo(static_cast<Entity>(entityID), intensity, duration);
    }

    bool GlowComponent::IsDone()
    {
        auto* coord = Coordinator::GetInstance();
        if (!coord) return true;
        auto gfx = coord->GetSystem<GraphicsManager>();
        if (!gfx) return true;
        return gfx->IsGlowTweenDone(static_cast<Entity>(entityID));
    }
}
