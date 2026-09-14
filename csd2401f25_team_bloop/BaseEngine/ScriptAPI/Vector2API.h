/******************************************************************************/
/**
* @file        Vector2API.h
* @project     Pulse Protocol
* @author      Ban Kai Wei Benjamin - 100%
* @brief       2D vector type exposed to the scripting API. 
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/

#pragma once
namespace ScriptAPI
{
    //property -> .Net property that compile into get_XXX()/set_XXX() methods
    public value struct Vector2
    {
    public:
        //dont need protect this
        float x;
        float y;

        Vector2(float x, float y);

        property float X
        {
            float get();
            void set(float value);
        }

        property float Y
        {
            float get();
            void set(float value);
        }

        property float magnitude
        {
            float get();
        }

        property Vector2 normalized
        {
            Vector2 get();
        }

        //
        // property belongs to an instance.
        // static property belongs to the type.
        //
        static property Vector2 zero
        {
            Vector2 get();
        }

        //static method usage
        //Vector2.Distance(a, b);
        //depends on 2 vectors no "this"

        static Vector2 Normalize(Vector2 v);
        static float  Angle(Vector2 a, Vector2 b);
        static Vector2 Min(Vector2 a, Vector2 b);
        static Vector2 Max(Vector2 a, Vector2 b);

        // arithmetic
        static Vector2 operator+(Vector2 a, Vector2 b);
        static Vector2 operator-(Vector2 a, Vector2 b);
        static Vector2 operator-(Vector2 v);      // unary
        static Vector2 operator*(Vector2 v, float s);
        static Vector2 operator*(float s, Vector2 v);
        static Vector2 operator/(Vector2 v, float s);

        // comparison
        static bool operator==(Vector2 a, Vector2 b);
        static bool operator!=(Vector2 a, Vector2 b);

    };
}