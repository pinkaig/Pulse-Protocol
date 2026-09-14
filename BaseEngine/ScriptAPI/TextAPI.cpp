/******************************************************************************/
/**
* @file        TextAPI.cpp
* @project     Pulse Protocol
* @author      Ban Kai Wei Benjamin
* @brief       Implementation of TextComponent wrapper for C# scripts
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#pragma once
#define generic generic_workaround

#include "TextAPI.h"
#include <msclr/marshal_cppstd.h>
#include "CoreEngine/Core/ScriptingBridge.h"
#include "../CoreEngine/ECS/Coordinator.h"
#include "../Graphics/Text.h"

#undef generic

namespace ScriptAPI
{
    TextComponentAPI::TextComponentAPI(unsigned int ID) : entityID(ID) {}

    void TextComponentAPI::SetText(System::String^ text)
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator->HasComponent<::TextComponent>(entityID))
            return;

        std::string nativeText = msclr::interop::marshal_as<std::string>(text);
        auto& comp = g_coordinator->GetComponent<::TextComponent>(entityID);
        comp.text = nativeText;
    }

    System::String^ TextComponentAPI::GetText()
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator->HasComponent<::TextComponent>(entityID))
            return gcnew System::String("");

        auto& comp = g_coordinator->GetComponent<::TextComponent>(entityID);
        return gcnew System::String(comp.text.c_str());
    }

    void TextComponentAPI::SetColor(float r, float g, float b)
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator->HasComponent<::TextComponent>(entityID))
            return;

        auto& comp = g_coordinator->GetComponent<::TextComponent>(entityID);
        comp.color = { r, g, b };
    }

    void TextComponentAPI::SetFontSize(float size)
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator->HasComponent<::TextComponent>(entityID))
            return;

        auto& comp = g_coordinator->GetComponent<::TextComponent>(entityID);
        comp.font_size = size;
    }

    void TextComponentAPI::SetVisible(bool visible)
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator->HasComponent<::TextComponent>(entityID))
            return;

        auto& comp = g_coordinator->GetComponent<::TextComponent>(entityID);
        comp.visible = visible;
    }

    bool TextComponentAPI::IsVisible()
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator->HasComponent<::TextComponent>(entityID))
            return false;

        auto& comp = g_coordinator->GetComponent<::TextComponent>(entityID);
        return comp.visible;
    }

    void TextComponentAPI::SetAlignment(int alignment)
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator->HasComponent<::TextComponent>(entityID))
            return;

        auto& comp = g_coordinator->GetComponent<::TextComponent>(entityID);
        comp.alignment = static_cast<::TextAlignment>(alignment);
    }

    void TextComponentAPI::SetAlpha(float a)
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator->HasComponent<::TextComponent>(entityID))
            return;
        auto& comp = g_coordinator->GetComponent<::TextComponent>(entityID);
        comp.alpha = a;
    }

    float TextComponentAPI::GetAlpha()
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator->HasComponent<::TextComponent>(entityID))
            return 1.0f;
        auto& comp = g_coordinator->GetComponent<::TextComponent>(entityID);
        return comp.alpha;
    }
}