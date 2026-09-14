/*****************************************************************************
 * @file        PostProcessAPI.h
 * @project     Pulse Protocol
 * @author      Goh Pin Kai
 *
 * @brief       C++/CLI static API for controlling the bloom post-processing
 *              effect from C# scripts.
 *
 *              Example (C# beat flash):
 *                PostProcessAPI.SetBloomIntensity(2.5f);
 *
 * @copyright   Copyright (C) 2026
 *              DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without
 *              prior written consent is prohibited.
 *****************************************************************************/
#pragma once

namespace ScriptAPI
{
    public ref class PostProcessAPI abstract sealed
    {
    public:
        // Enable or disable the bloom effect entirely
        static void SetBloomEnabled(bool enabled);

        // Bloom intensity multiplier (0.0 = no bloom, 1.2 = default, 3.0 = strong)
        static void SetBloomIntensity(float intensity);

        // Luminance threshold above which pixels contribute to bloom (0.0–1.0)
        static void SetBloomThreshold(float threshold);

        static float GetBloomIntensity();
        static float GetBloomThreshold();
        static bool  IsBloomEnabled();
    };
}
