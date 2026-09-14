/*****************************************************************************
 * @file        PostProcessAPI.cpp
 * @project     Pulse Protocol
 * @author      Goh Pin Kai
 *
 * @brief       Implementation of the PostProcessAPI C++/CLI bridge.
 *              Resolves GraphicsManager via the ECS Coordinator and
 *              delegates bloom control calls to it.
 *
 * @copyright   Copyright (C) 2026
 *              DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without
 *              prior written consent is prohibited.
 *****************************************************************************/
#pragma once
#define generic generic_workaround
#include "PostProcessAPI.h"
#include "../CoreEngine/ECS/Coordinator.h"
#include "../Graphics/GraphicsManager.h"
#undef generic

namespace ScriptAPI
{
    void PostProcessAPI::SetBloomEnabled(bool enabled)
    {
        auto* coord = Coordinator::GetInstance(); if (!coord) return;
        auto gfx = coord->GetSystem<GraphicsManager>(); if (!gfx) return;
        gfx->SetBloomEnabled(enabled);
    }

    void PostProcessAPI::SetBloomIntensity(float intensity)
    {
        auto* coord = Coordinator::GetInstance(); if (!coord) return;
        auto gfx = coord->GetSystem<GraphicsManager>(); if (!gfx) return;
        gfx->SetBloomIntensity(intensity);
    }

    void PostProcessAPI::SetBloomThreshold(float threshold)
    {
        auto* coord = Coordinator::GetInstance(); if (!coord) return;
        auto gfx = coord->GetSystem<GraphicsManager>(); if (!gfx) return;
        gfx->SetBloomThreshold(threshold);
    }

    float PostProcessAPI::GetBloomIntensity()
    {
        auto* coord = Coordinator::GetInstance(); if (!coord) return 0.0f;
        auto gfx = coord->GetSystem<GraphicsManager>(); if (!gfx) return 0.0f;
        return gfx->GetBloomIntensity();
    }

    float PostProcessAPI::GetBloomThreshold()
    {
        auto* coord = Coordinator::GetInstance(); if (!coord) return 0.0f;
        auto gfx = coord->GetSystem<GraphicsManager>(); if (!gfx) return 0.0f;
        return gfx->GetBloomThreshold();
    }

    bool PostProcessAPI::IsBloomEnabled()
    {
        auto* coord = Coordinator::GetInstance(); if (!coord) return false;
        auto gfx = coord->GetSystem<GraphicsManager>(); if (!gfx) return false;
        return gfx->IsBloomEnabled();
    }
}
