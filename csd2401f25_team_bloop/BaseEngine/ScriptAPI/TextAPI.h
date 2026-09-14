/******************************************************************************/
/**
* @file        TextAPI.h
* @project     Pulse Protocol
* @author      Ban Kai Wei Benjamin
* @brief       Wrapper for TextComponent access from C# scripts
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#pragma once

namespace ScriptAPI
{
    public value struct TextComponentAPI
    {
    public:
        void SetText(System::String^ text);
        System::String^ GetText();

        void SetColor(float r, float g, float b);
        void SetFontSize(float size);
        void SetVisible(bool visible);
        bool IsVisible();
        void SetAlignment(int alignment);
        void SetAlpha(float a);
        float GetAlpha();

    internal:
        TextComponentAPI(unsigned int ID);
    private:
        unsigned int entityID;
    };
}