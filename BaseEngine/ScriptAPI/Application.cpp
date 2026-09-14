/******************************************************************************/
/**
* @file        Application.cpp
* @project     Pulse Protocol
* @author      Chloe Lau Rey En
* @brief       Provides application-level functions like quitting the game.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#pragma managed(push, off)
#define generic generic_workaround
#include "CoreEngine/Core/CoreEngine.h"
#include <GLFW/glfw3.h>
#undef generic

static void NativeQuit()
{
    if (Engine && Engine->GetWindow())
        glfwSetWindowShouldClose(Engine->GetWindow(), GLFW_TRUE);
}
#pragma managed(pop)

#include "Application.h"

namespace ScriptAPI
{
    void Application::Quit()
    {
        NativeQuit();
    }

    bool Application::IsPlaying()
    {
        if (Engine)
        {
            return Engine->IsPlaying();
        }
        return false;
    }

    // When paused, CoreEngine::GetDeltaTime() returns 0
    // This stops all game updates since scripts use DeltaTime for frame-independent movement
    void Application::SetPaused(bool paused)
    {
        if (Engine)
        {
            Engine->SetPaused(paused, CoreEngine::PauseSource::Game);
        }
    }

    // Check if game is currently paused
    bool Application::IsPaused()
    {
        if (Engine)
        {
            return Engine->IsPaused();
        }
        return false;
    }
}