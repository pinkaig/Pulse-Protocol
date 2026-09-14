/******************************************************************************/
/**
* @file        Animation.cpp
* @project     Pulse Protocol
* @author      Chia Wei Xuan Rachael - 100%
* @brief       Implementation of Animation wrapper class providing C# scripts access 
*              to entity animation and sprite management through ECS component modification.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#pragma once
#define generic generic_workaround
#include "Animation.h"
#include <msclr/marshal_cppstd.h>   // for converting C# to C++ string
#include "../Graphics/Animation.h"
#include "../Graphics/Renderable.h"
#include "../CoreEngine/Core/CoreEngine.h"   // for FilePathToGame
#undef generic

namespace ScriptAPI
{
    // constrcutor for entityID since we need to know the ID to chaneg the Entity Component Value 
    Animation::Animation(unsigned int ID) : entityID(ID) {}

    void Animation::SetAnimation(System::String^ SpriteName, int rows, int columns, int TotalFrames, float speed)
    {
        auto* g_coordinator = Coordinator::GetInstance();

        if (g_coordinator->HasComponent<Framework::Animation>(entityID))
        {
            auto& Comp = g_coordinator->GetComponent<Framework::Animation>(entityID);
            Comp.currentFrame = 0;           // Reset to da first frame
            Comp.elapsedTime = 0.0f;         // Reset der timer

            // gib new valueeeee
            Comp.rows = rows;
            Comp.columns = columns;
            Comp.totalFrames = TotalFrames;
            Comp.animationSpeed = speed;
        }

        // Convert C# string to C++ string, then use SpriteName as the texture ID which will be looked up at texHandle_key container. 
        std::string nativeSpriteName = msclr::interop::marshal_as<std::string>(SpriteName);

        if (g_coordinator->HasComponent<Framework::Renderable>(entityID))
        {
            auto& Comp = g_coordinator->GetComponent<Framework::Renderable>(entityID);
            Comp.spriteName = nativeSpriteName;
            Comp.needsTextureReload = true;
        }

    }

    // Get the current SpriteName from this entity
    System::String^ Animation::GetCurrentAnimation()
    {
        auto* g_coordinator = Coordinator::GetInstance();
        auto& comp = g_coordinator->GetComponent<Framework::Renderable>(entityID);

        // Convert C++ to C# string :D
        std::string cppString = comp.spriteName;
        System::String^ csString = gcnew System::String(cppString.c_str());

        return csString;
    }

    //change animation speed(duh)
    void Animation::SetAnimationSpeed(float speed)
    {
        auto* g_coordinator = Coordinator::GetInstance();
        auto& comp = g_coordinator->GetComponent<Framework::Animation>(entityID);
        comp.animationSpeed = speed;
    }

    void Animation::SetCurrentFrame(int frame)
    {
        auto* g_coordinator = Coordinator::GetInstance();
        auto& comp = g_coordinator->GetComponent<Framework::Animation>(entityID);
        comp.currentFrame = frame;
        comp.elapsedTime = 0.0f;
    }

}