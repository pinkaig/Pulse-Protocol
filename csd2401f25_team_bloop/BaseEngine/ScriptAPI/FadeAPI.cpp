/*****************************************************************************
 * @file        FadeAPI.cpp
 * @project     Pulse Protocol
 * @author      Goh Pin Kai
 *
 * @brief       Implementation of the FadeAPI C++/CLI bridge.
 *              Resolves GraphicsManager via the ECS system manager and
 *              delegates all calls to its screen-wide fade API.
 *
 * @copyright   Copyright (C) 2026
 *              DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without
 *              prior written consent is prohibited.
 *****************************************************************************/
#pragma once
#define generic generic_workaround
#include "FadeAPI.h"
#include "../CoreEngine/ECS/Coordinator.h"
#include "../Graphics/GraphicsManager.h"
#undef generic

namespace ScriptAPI
{
    void FadeAPI::FadeOut(float duration, float r, float g, float b)
    {
        auto* coord = Coordinator::GetInstance();
        if (!coord) return;
        auto gfx = coord->GetSystem<GraphicsManager>();
        if (!gfx) return;
        // Overlay alpha 0 -> 1 (screen fills with color)
        float fromAlpha = gfx->GetScreenAlpha();
        gfx->StartScreenFade(fromAlpha, 1.0f, r, g, b, duration);
    }

    void FadeAPI::FadeIn(float duration, float r, float g, float b)
    {
        auto* coord = Coordinator::GetInstance();
        if (!coord) return;
        auto gfx = coord->GetSystem<GraphicsManager>();
        if (!gfx) return;
        // Overlay alpha 1 -> 0 (screen clears from color)
        float fromAlpha = gfx->GetScreenAlpha();
        gfx->StartScreenFade(fromAlpha, 0.0f, r, g, b, duration);
    }

    void FadeAPI::FadeOut(float duration)
    {
        FadeOut(duration, 0.0f, 0.0f, 0.0f);
    }

    void FadeAPI::FadeIn(float duration)
    {
        FadeIn(duration, 0.0f, 0.0f, 0.0f);
    }

    bool FadeAPI::IsFadeDone()
    {
        auto* coord = Coordinator::GetInstance();
        if (!coord) return true;
        auto gfx = coord->GetSystem<GraphicsManager>();
        if (!gfx) return true;
        return gfx->IsScreenFadeDone();
    }

    float FadeAPI::GetAlpha()
    {
        auto* coord = Coordinator::GetInstance();
        if (!coord) return 0.0f;
        auto gfx = coord->GetSystem<GraphicsManager>();
        if (!gfx) return 0.0f;
        return gfx->GetScreenAlpha();
    }

    void FadeAPI::SetAlpha(float alpha)
    {
        auto* coord = Coordinator::GetInstance();
        if (!coord) return;
        auto gfx = coord->GetSystem<GraphicsManager>();
        if (!gfx) return;
        float clamped = alpha < 0.0f ? 0.0f : (alpha > 1.0f ? 1.0f : alpha);
        // Instant set: start == target with near-zero duration
        gfx->StartScreenFade(clamped, clamped, 0.0f, 0.0f, 0.0f, 0.0001f);
    }

    void FadeAPI::Stop()
    {
        auto* coord = Coordinator::GetInstance();
        if (!coord) return;
        auto gfx = coord->GetSystem<GraphicsManager>();
        if (!gfx) return;
        gfx->StopScreenFade();
    }
}
