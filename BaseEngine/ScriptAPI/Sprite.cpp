/******************************************************************************/
/**
* @file        Sprite.cpp
* @project     Pulse Protocol
* @author      Chloe Lau Rey En
* @brief       Implementation of sprite/texture wrapper for C# scripts
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#pragma once
#define generic generic_workaround

#include "Sprite.h"
#include <msclr/marshal_cppstd.h>
#include "CoreEngine/Core/ScriptingBridge.h"
#include "../CoreEngine/ECS/Coordinator.h"
#include "../Graphics/Renderable.h"

#undef generic

namespace ScriptAPI
{
    SpriteComponent::SpriteComponent(unsigned int ID) : entityID(ID) {}

    // ============== Texture ==============
    System::String^ SpriteComponent::Texture::get()
    {
        return GetTexture();
    }

    void SpriteComponent::Texture::set(System::String^ value)
    {
        SetTexture(value);
    }

    void SpriteComponent::SetTexture(System::String^ texturePath)
    {
        auto* g_coordinator = Coordinator::GetInstance();

        if (!g_coordinator->HasComponent<Framework::Renderable>(entityID))
            return;

        std::string nativePath = msclr::interop::marshal_as<std::string>(texturePath);

        auto& comp = g_coordinator->GetComponent<Framework::Renderable>(entityID);
        comp.spriteName = nativePath;
        comp.needsTextureReload = true;  // Trigger hot-swap
    }

    System::String^ SpriteComponent::GetTexture()
    {
        auto* g_coordinator = Coordinator::GetInstance();

        if (!g_coordinator->HasComponent<Framework::Renderable>(entityID))
            return gcnew System::String("");

        auto& comp = g_coordinator->GetComponent<Framework::Renderable>(entityID);
        return gcnew System::String(comp.spriteName.c_str());
    }

    // ============== Tint Color ==============
    float SpriteComponent::TintR::get()
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator->HasComponent<Framework::Renderable>(entityID))
            return 1.0f;
        return g_coordinator->GetComponent<Framework::Renderable>(entityID).tintColor.r;
    }

    void SpriteComponent::TintR::set(float value)
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator->HasComponent<Framework::Renderable>(entityID))
            return;
        g_coordinator->GetComponent<Framework::Renderable>(entityID).tintColor.r = value;
    }

    float SpriteComponent::TintG::get()
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator->HasComponent<Framework::Renderable>(entityID))
            return 1.0f;
        return g_coordinator->GetComponent<Framework::Renderable>(entityID).tintColor.g;
    }

    void SpriteComponent::TintG::set(float value)
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator->HasComponent<Framework::Renderable>(entityID))
            return;
        g_coordinator->GetComponent<Framework::Renderable>(entityID).tintColor.g = value;
    }

    float SpriteComponent::TintB::get()
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator->HasComponent<Framework::Renderable>(entityID))
            return 1.0f;
        return g_coordinator->GetComponent<Framework::Renderable>(entityID).tintColor.b;
    }

    void SpriteComponent::TintB::set(float value)
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator->HasComponent<Framework::Renderable>(entityID))
            return;
        g_coordinator->GetComponent<Framework::Renderable>(entityID).tintColor.b = value;
    }

    float SpriteComponent::TintA::get()
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator->HasComponent<Framework::Renderable>(entityID))
            return 1.0f;
        return g_coordinator->GetComponent<Framework::Renderable>(entityID).tintColor.a;
    }

    void SpriteComponent::TintA::set(float value)
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator->HasComponent<Framework::Renderable>(entityID))
            return;
        g_coordinator->GetComponent<Framework::Renderable>(entityID).tintColor.a = value;
    }

    void SpriteComponent::SetTint(float r, float g, float b, float a)
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator->HasComponent<Framework::Renderable>(entityID))
            return;

        auto& comp = g_coordinator->GetComponent<Framework::Renderable>(entityID);
        comp.tintColor.r = r;
        comp.tintColor.g = g;
        comp.tintColor.b = b;
        comp.tintColor.a = a;
    }

    // ============== UseTexture ==============
    bool SpriteComponent::UseTexture::get()
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator->HasComponent<Framework::Renderable>(entityID))
            return true;
        return g_coordinator->GetComponent<Framework::Renderable>(entityID).useTexture;
    }

    void SpriteComponent::UseTexture::set(bool value)
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator->HasComponent<Framework::Renderable>(entityID))
            return;
        g_coordinator->GetComponent<Framework::Renderable>(entityID).useTexture = value;
    }

    // ============== HasSprite ==============
    bool SpriteComponent::HasSprite()
    {
        auto* g_coordinator = Coordinator::GetInstance();
        return g_coordinator->HasComponent<Framework::Renderable>(entityID);
    }
}