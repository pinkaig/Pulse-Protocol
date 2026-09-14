/******************************************************************************/
/**
* @file        Application.h
* @project     Pulse Protocol
* @author      Chloe Lau Rey En
* @brief       Provides application-level functions like quitting the game.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#pragma once

namespace ScriptAPI
{
    public ref class Application abstract sealed
    {
    public:
        static void Quit();
        static bool IsPlaying();

        static void SetPaused(bool paused);
        static bool IsPaused();
    };
}