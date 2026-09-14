/*****************************************************************************
 * @file        CameraEffects.cpp
 * @project     Pulse Protocol
 * @author		Goh Pin Kai
 *
 * @brief       Implements CameraEffects: stacks/limits punch and shake inputs,
 *              decays them over time, and generates per-frame random shake offsets.
 *              The punch zoom eases back to zero at a configurable return speed.
 *
 * @copyright  Copyright (C) 2026 DigiPen Institute of Technology.
 *             Reproduction or disclosure of this file or its contents without the
 *             prior written consent of DigiPen Institute of Technology is prohibited.
 ******************************************************************************/

#include "CameraEffects.h"
#include <cstdlib>
#include <cmath>

float CameraEffects::Rand01()
{
    return float(std::rand()) / float(RAND_MAX);
}

void CameraEffects::AddBeatPunch(const PunchDesc& d)
{
    // �Punch� just sets the target amount; Update() eases it back
    mPunchZoom01 = (mPunchZoom01 < d.zoomAmount) ? d.zoomAmount : mPunchZoom01;
    mPunchReturnSpeed = d.zoomReturnSpeed;
}

void CameraEffects::AddShake(const ShakeDesc& d)
{
    // Stack shake strength (keep max)
    mShakeStrength = (mShakeStrength < d.amplitude) ? d.amplitude : mShakeStrength;
    mShakeDecay = d.decay;
}

void CameraEffects::Update(float dt)
{
    // Punch return to 0
    if (mPunchZoom01 > 0.0f)
    {
        mPunchZoom01 -= mPunchReturnSpeed * dt;
        if (mPunchZoom01 < 0.0f) mPunchZoom01 = 0.0f;
    }

    // Shake decay (exponential so small amplitudes persist across frames)
    if (mShakeStrength > 0.0001f)
    {
        mShakeStrength *= std::exp(-mShakeDecay * dt);
        float ox = (Rand01() * 2.0f - 1.0f) * mShakeStrength;
        float oy = (Rand01() * 2.0f - 1.0f) * mShakeStrength;
        mShakeOffset = { ox, oy };
    }
    else
    {
        mShakeStrength = 0.0f;
        mShakeOffset = { 0.0f, 0.0f };
    }
}
