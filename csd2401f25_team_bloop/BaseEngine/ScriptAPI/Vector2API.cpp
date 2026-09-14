/******************************************************************************/
/**
* @file        Vector2API.cpp
* @project     Pulse Protocol
* @author      Ban Kai Wei Benjamin - 100%
* @brief       2D vector type exposed to the scripting API.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#define generic generic_workaround
#include "Vector2API.h"
#include "CoreEngine/Core/ScriptingBridge.h"
#undef generic
#include <cmath> 
#include <algorithm>

namespace ScriptAPI {


	Vector2::Vector2(float x, float y) : x(x), y(y) {}

    float Vector2::X::get() { return x; }
    void  Vector2::X::set(float value) { x = value; }

    float Vector2::Y::get() { return y; }
    void  Vector2::Y::set(float value) { y = value; }

    // ---- magnitude / normalized ----
    float Vector2::magnitude::get()
    {
        return static_cast<float>(std::sqrt(x * x + y * y));
    }

    Vector2 Vector2::normalized::get()
    {
        float len = magnitude;
        if (len <= 1e-6f)     // epsilon to avoid divide-by-zero
            return Vector2(0.f, 0.f);

        return Vector2(x / len, y / len);
    }

    // ---- static property zero ----
    Vector2 Vector2::zero::get()
    {
        return Vector2(0.f, 0.f);
    }

    // ---- static methods ----
    Vector2 Vector2::Normalize(Vector2 v)
    {
        return v.normalized;
    }

    float Vector2::Angle(Vector2 a, Vector2 b)
    {
        float magA = a.magnitude;
        float magB = b.magnitude;

        if (magA <= 1e-6f || magB <= 1e-6f)
            return 0.f;
        //cosQ = a.b / |a||b|
        float dot = a.x * b.x + a.y * b.y;
        float cosTheta = dot / (magA * magB);

        // clamp for numeric safety
        //since cosQ range must be [-1,1]
        if (cosTheta > 1.f) cosTheta = 1.f;
        if (cosTheta < -1.f) cosTheta = -1.f;

        float radians = static_cast<float>(std::acos(cosTheta));
        float degrees = radians * (180.0f / 3.14159265358979323846f);

        return degrees;
    }

    Vector2 Vector2::Min(Vector2 a, Vector2 b)
    {
        return Vector2(
            (std::min)(a.x, b.x),
            (std::min)(a.y, b.y)
        );
    }

    Vector2 Vector2::Max(Vector2 a, Vector2 b)
    {
        return Vector2(
            (std::max)(a.x, b.x),
            (std::max)(a.y, b.y)
        );
    }

    // ---- operators ----
    Vector2 Vector2::operator+(Vector2 a, Vector2 b)
    {
        return Vector2(a.x + b.x, a.y + b.y);
    }

    Vector2 Vector2::operator-(Vector2 a, Vector2 b)
    {
        return Vector2(a.x - b.x, a.y - b.y);
    }

    Vector2 Vector2::operator-(Vector2 v)
    {
        return Vector2(-v.x, -v.y);
    }

    Vector2 Vector2::operator*(Vector2 v, float s)
    {
        return Vector2(v.x * s, v.y * s);
    }

    Vector2 Vector2::operator*(float s, Vector2 v)
    {
        return Vector2(v.x * s, v.y * s);
    }

    Vector2 Vector2::operator/(Vector2 v, float s)
    {
        // up to you: handle s==0 differently
        return Vector2(v.x / s, v.y / s);
    }

    bool Vector2::operator==(Vector2 a, Vector2 b)
    {
        // exact float compare 
        return (a.x == b.x) && (a.y == b.y);
    }

    bool Vector2::operator!=(Vector2 a, Vector2 b)
    {
        return !(a == b);
    }


}// end of name space