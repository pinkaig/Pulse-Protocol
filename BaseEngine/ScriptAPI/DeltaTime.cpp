/******************************************************************************/
/**
* @file        DeltaTime.cpp
* @project     pulse protocol
* @author      chia wei xuan rachael - 100%
* @brief       Implementation of Time class providing C# scripts with frame timing 
*              and game time data through read-only property accessors.
*
* @copyright   copyright (c) 2026 digipen institute of technology.
*              reproduction or disclosure of this file or its contents without the
*              prior written consent of digipen institute of technology is prohibited.
*
/******************************************************************************/
#pragma once
#define generic generic_workaround
#include "DeltaTime.h"
#include "../CoreEngine/Core/CoreEngine.h"
#undef generic

namespace ScriptAPI
{
    float Time::DeltaTime::get()
    {
        return Engine->GetDeltaTime();
    }

    int Time::FrameCount::get()
    {
        return Engine->GetFrameCount();
    }

    float Time::GameTime::get()
    {
        return Engine->GetTimePassed();
    }

}