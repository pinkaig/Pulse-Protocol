/******************************************************************************/
/**
* @file        ScriptAPIClass.h
* @project     Pulse Protocol
* @author      Chia Wei Xuan Rachael - 95%
* @author      Goh Pin Kai - 5%
* @brief       Provides the C++/CLI bridge with Script class that all C# game scripts inherit from, 
               managing entity-script associations and exposing component access methods to managed code.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#pragma once
#include "TransformComponent.h"
#include "Health.h"
#include "DeltaTime.h"
#include "InputAPI.h"
#include "Collision.h"
#include "AudioAPI.h"
#include "Sprite.h"
#include "Application.h"
#include "FadeComponent.h"
#include "GlowAPI.h"

namespace ScriptAPI
{
    //forward delcaration
    value struct TransformComponent;
    value struct HealthComponent;
    value struct Animation;
    value struct TextComponentAPI;
    value struct CollisionComponent;
    value struct AudioComponent;
    value struct SpriteComponent;
    value struct FadeComponent;
    value struct GlowComponent;

    public ref class Script abstract
    {
    public:
        virtual void Start() {}
        void virtual Update() {};

        TransformComponent GetTransform();
        HealthComponent GetHealth();
        Animation GetAnimation();
        InputComponent GetInput();
        CollisionComponent GetCollision(); 
        AudioComponent GetAudio();
        SpriteComponent GetSprite();
        TextComponentAPI GetTextComponent();
        FadeComponent GetFade();
        GlowComponent GetGlow();
        //DeltaTime GetDeltaTime();
        //LogicComponent GetScriptName();

    internal:
        // Store which entity this script is attached too
        void SetEntityID(int ID)
        {
            EntityID = ID;
        }
        
    protected:
        property int entityID
        {
            int get()
            {
                return EntityID;
            }
        }

    private:
        int EntityID;
    };
}