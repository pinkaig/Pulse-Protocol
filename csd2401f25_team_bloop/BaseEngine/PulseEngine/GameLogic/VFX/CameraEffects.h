/*****************************************************************************
 * @file        CameraEffects.h
 * @project     Pulse Protocol
 * @author		Goh Pin Kai
 *
 * @brief       Camera effects container for rhythm/gameplay feedback. Provides a
 *              beat "punch" (temporary zoom-in) and event-based camera shake with
 *              configurable strength and decay. Exposes per-frame outputs that can
 *              be applied to a camera's zoom and center/position.
 *
 * @copyright  Copyright (C) 2026 DigiPen Institute of Technology.
 *             Reproduction or disclosure of this file or its contents without the
 *             prior written consent of DigiPen Institute of Technology is prohibited.
 ******************************************************************************/

#pragma once
#include <glm/vec2.hpp>

struct PunchDesc
{
    float zoomAmount = 0.08f;        // zoom-in amount (0.08 = +8%)
    float zoomReturnSpeed = 10.0f;   // how fast it returns to 0
};

struct ShakeDesc
{
    float amplitude = 0.03f; // offset amount (in your world/camera space scale)
    float decay = 8.0f;      // higher = stops faster
};

class CameraEffects
{
public:
    void SetBaseZoom(float z) { mBaseZoom = z; }
    float GetBaseZoom() const { return mBaseZoom; }

    // Call when beat happens
    void AddBeatPunch(const PunchDesc& d);

    // Call when hit/miss/etc happens
    void AddShake(const ShakeDesc& d);

    // Update every frame
    void Update(float dt);

    // Apply on your camera values each frame:
    //   cam.zoom = baseZoom * (1 + punchZoom)
    //   cam.center += shakeOffset
    float GetPunchZoom01() const { return mPunchZoom01; }   // 0..zoomAmount
    glm::vec2 GetShakeOffset() const { return mShakeOffset; }

private:
    float mBaseZoom = 1.0f;

    // Beat punch state
    float mPunchZoom01 = 0.0f;
    float mPunchReturnSpeed = 10.0f;

    // Shake state
    float mShakeStrength = 0.0f;
    float mShakeDecay = 8.0f;
    glm::vec2 mShakeOffset{ 0.0f, 0.0f };

    float Rand01();
};
