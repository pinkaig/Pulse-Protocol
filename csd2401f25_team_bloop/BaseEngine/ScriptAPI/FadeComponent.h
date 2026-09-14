/*****************************************************************************
 * @file        FadeComponent.h
 * @project     Pulse Protocol
 * @author      Goh Pin Kai
 *
 * @brief       C++/CLI value struct exposing per-entity alpha fade to C# scripts.
 *              Delegates timer management to GraphicsManager so designers can
 *              call FadeIn/FadeOut once and let the engine handle interpolation.
 *
 * @copyright   Copyright (C) 2026
 *              DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without
 *              prior written consent is prohibited.
 *****************************************************************************/
#pragma once

namespace ScriptAPI
{
    public value struct FadeComponent
    {
    public:
        // Fade the entity's alpha from its current value to 1 over [duration] seconds
        void FadeIn(float duration);

        // Fade the entity's alpha from its current value to 0 over [duration] seconds
        void FadeOut(float duration);

        // Fade the entity's alpha to any target value over [duration] seconds
        void FadeTo(float alpha, float duration);

        // Returns true when no fade is currently in progress for this entity
        bool IsDone();

        // Read the current alpha of this entity's Renderable tintColor
        float GetAlpha();

        // Set alpha instantly (no tween)
        void SetAlpha(float alpha);

    internal:
        FadeComponent(unsigned int ID);

    private:
        unsigned int entityID;
    };
}
