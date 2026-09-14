/******************************************************************************/
/**
 * @file        GameMain.cpp
 * @project     Pulse Protocol
 * @author      Goh Pin Kai
 * @brief	    Entry point for the Pulse Protocol standalone game executable.	
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 *
 ******************************************************************************/
#include "pch/pch.h"            // same PCH as engine/editor
#include <windows.h>

int main()
{
    CoreEngine engine;
    Engine = &engine;           // global pointer declared in CoreEngine.h,
                                // defined once in CoreEngine.cpp

    engine.Setup();             // register ECS components + systems
    engine.SetStartupFullscreen(true);
    engine.Init();              // window, GL, FMOD, assets, etc.

    engine.PlayGame(); // this auto-starts the game

    // In Release mode, hide the debug display after engine init
    #ifndef _DEBUG
        auto* coord = Coordinator::GetInstance();
        auto sceneManager = coord->GetSystem<SceneManager>();
        if (coord && sceneManager) {
            const auto& entities = sceneManager->GetSceneEntities();
            for (auto entity : entities) {
                if (coord->HasComponent<Name>(entity)) {
                    auto& name = coord->GetComponent<Name>(entity);
                    if (name.name == "Debug Display Text" && coord->HasComponent<TextComponent>(entity)) {
                        auto& txt = coord->GetComponent<TextComponent>(entity);
                        txt.visible = false;
                    }
                }
            }
        }
    #endif

    engine.GameLoop();          // pure game loop – no MyEditor calls
    engine.Exit();              // shutdown systems, destroy window

    Engine = nullptr;
    return 0;
}