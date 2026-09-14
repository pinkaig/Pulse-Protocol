/******************************************************************************/
/**
* @file        Sprite.h
* @project     Pulse Protocol
* @author      Chloe Lau Rey En
* @brief       Wrapper for sprite/texture access from C# scripts
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#pragma once

namespace ScriptAPI
{
    public value struct SpriteComponent
    {
    public:
        // Texture path
        property System::String^ Texture
        {
            System::String^ get();
            void set(System::String^ value);
        }

        // Tint color (RGBA)
        property float TintR
        {
            float get();
            void set(float value);
        }

        property float TintG
        {
            float get();
            void set(float value);
        }

        property float TintB
        {
            float get();
            void set(float value);
        }

        property float TintA
        {
            float get();
            void set(float value);
        }

        // Toggle texture vs solid color
        property bool UseTexture
        {
            bool get();
            void set(bool value);
        }

        // Convenience methods
        void SetTexture(System::String^ texturePath);
        System::String^ GetTexture();
        void SetTint(float r, float g, float b, float a);
        bool HasSprite();

    internal:
        SpriteComponent(unsigned int ID);

    private:
        unsigned int entityID;
    };
}