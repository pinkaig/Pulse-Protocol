/*****************************************************************************
 * @file        FadeComponent.cpp
 * @project     Pulse Protocol
 * @author      Goh Pin Kai
 *
 * @brief       Implementation of the FadeComponent C++/CLI bridge.
 *              Resolves GraphicsManager via the ECS system manager and
 *              delegates fade requests to its per-entity fade API.
 *
 * @copyright   Copyright (C) 2026
 *              DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without
 *              prior written consent is prohibited.
 *****************************************************************************/
#pragma once
#define generic generic_workaround
#include "FadeComponent.h"
#include "../CoreEngine/ECS/Coordinator.h"
#include "../Graphics/GraphicsManager.h"
#include "../Graphics/Renderable.h"
#undef generic

namespace ScriptAPI
{
    FadeComponent::FadeComponent(unsigned int ID) : entityID(ID) {}

    void FadeComponent::FadeIn(float duration)
    {
        auto* coord = Coordinator::GetInstance();
        if (!coord) return;
        auto gfx = coord->GetSystem<GraphicsManager>();
        if (!gfx) return;
        gfx->StartEntityFade(static_cast<Entity>(entityID), 1.0f, duration);
    }

    void FadeComponent::FadeOut(float duration)
    {
        auto* coord = Coordinator::GetInstance();
        if (!coord) return;
        auto gfx = coord->GetSystem<GraphicsManager>();
        if (!gfx) return;
        gfx->StartEntityFade(static_cast<Entity>(entityID), 0.0f, duration);
    }

    void FadeComponent::FadeTo(float alpha, float duration)
    {
        auto* coord = Coordinator::GetInstance();
        if (!coord) return;
        auto gfx = coord->GetSystem<GraphicsManager>();
        if (!gfx) return;
        gfx->StartEntityFade(static_cast<Entity>(entityID), alpha, duration);
    }

    bool FadeComponent::IsDone()
    {
        auto* coord = Coordinator::GetInstance();
        if (!coord) return true;
        auto gfx = coord->GetSystem<GraphicsManager>();
        if (!gfx) return true;
        return gfx->IsEntityFadeDone(static_cast<Entity>(entityID));
    }

    float FadeComponent::GetAlpha()
    {
        auto* coord = Coordinator::GetInstance();
        if (!coord) return 1.0f;
        if (!coord->HasComponent<Framework::Renderable>(static_cast<Entity>(entityID)))
            return 1.0f;
        return coord->GetComponent<Framework::Renderable>(
            static_cast<Entity>(entityID)).tintColor.a;
    }

    void FadeComponent::SetAlpha(float alpha)
    {
        auto* coord = Coordinator::GetInstance();
        if (!coord) return;
        if (!coord->HasComponent<Framework::Renderable>(static_cast<Entity>(entityID)))
            return;
        coord->GetComponent<Framework::Renderable>(
            static_cast<Entity>(entityID)).tintColor.a = alpha;
    }
}
