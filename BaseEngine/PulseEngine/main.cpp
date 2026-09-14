///******************************************************************************/
///**
// * @file        main.cpp
// * @project     Pulse Protocol
// * @author      Chia Wei Xuan Rachael (primary) - 50%
// * @author      Chloe Lau Rey En, Ban Kai Wei Benjamin,
// *              Reginald Lew Yee Ren, Goh Pin Kai, Leu Jun Yong (secondary) - 10% each
// * @brief       Entry point of the Pulse Protocol engine. Initializes the
// *              CoreEngine, logging, and editor (ImGui), sets up game objects,
// *              and runs the main game loop. Includes debug utilities for
// *              performance profiling, memory leak detection, and object
// *              visibility toggling.
// * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
// *              Reproduction or disclosure of this file or its contents without
// *              the prior written consent of DigiPen Institute of Technology is
// *              prohibited.
// */
// /******************************************************************************/
//
//#if defined(_WIN32)
//#ifndef NOMINMAX
//#define NOMINMAX
//#endif
//#ifndef WIN32_LEAN_AND_MEAN
//#define WIN32_LEAN_AND_MEAN
//#endif
//#include <windows.h>   // for OutputDebugString, etc.
//#endif
//
//#include <iostream>
//#include <cstdlib>
//#include <ctime>
//#include "CoreEngine/ECS/Coordinator.h"
//#include "pch/pch.h"
//#include "CoreEngine/Assets.h"
//#include "CoreEngine/Log/Log.h" // <-- include your logging system
//#include <GLFW/glfw3.h>
//
//#if (_DEBUG)
//// MEM LEAKS LIBRARIES (See _CrtMemCheckpoint for examples to check for mem leaks)
//#define _CRTDBG_MAP_ALLOC //to get more details
//#include <crtdbg.h>   //for malloc and free
//static _CrtMemState sOld;
//static _CrtMemState sNew;
//static _CrtMemState sDiff;
//#endif
//
//// Game window dimensions and title
//const char WindowTitle[] = "Game Window";
//const int Game_Window_Height = 900;
//const int Game_Window_Width = 1600;
//
//Matrix3x3 beforeTranpose = {1.f,2.f,3.f,4.f,5.f,6.f,7.f,8.f,9.f};
//Matrix3x3 A{};
//
//
//// just concepts for game states, will be in GSM 
//struct GameStates {
//    bool MainMenu = true;
//    bool IsRunning = false;
//    bool IsPaused = false;  
//    bool GameOver = false;
//
//    //libs that are good to use.
//        //fmod
//        //imgui
//        //glm 
//        //glfw
//        //freetype for font
//        //stb
//        //xcore lib for game engine
//        //spdlog
//        //rapid json
//} gamestate;
//
//// Global variables for object management
//bool objectsVisible = false;
//std::vector<Entity> renderObjects;
//
//// Function to create multiple objects for rendering
////void CreateRenderObjects(int count) {
////    auto* g_coordinator = Coordinator::GetInstance();
////    
////    // Clear existing objects
////    renderObjects.clear();
////    
////    for (int i = 0; i < count; ++i) {
////        // Create a new entity
////        Entity newEntity = g_coordinator->CreateEntity();
////        
////        // Create a Transform component with random properties
////        Framework::Transform transform;
////        Framework::Renderable renderable;
////        
////        // Random position within the game window bounds (accounting for viewport)
////        // The game viewport is centered and has aspect ratio 16:9
////        // We'll spawn objects within a reasonable area of the viewport
////        
////        // Spawn within the game area, leaving some margin
////        //float margin = 50.0f;
////        float halfW = Game_Window_Width * 0.5f;
////        float halfH = Game_Window_Height * 0.5f;
////        transform.Pos = Vector2(
////           (rand() % Game_Window_Width) - halfW,
////            (rand() % Game_Window_Height) - halfH
////        );
////        
////        // Random size between 20x20 and 60x60 pixels
////        float size = 200.0f + static_cast<float>(rand() % 40);
////        transform.Scale = Vector2(size, size);
////        
////        // Random rotation
////        transform.angle_disp = static_cast<float>(rand() % 360);
////        transform.angle_speed = static_cast<float>(rand() % 10) - 5.0f; // -5 to +5 degrees per frame
////
////        renderable.spriteName = "../../MyEngine/Assets/Textures/Characters/hero.png";
////        
////        // Set model and shader references for sprite rendering
////        renderable.mdl_ref = 1.0f;  // Use box model (1) instead of background (0)
////        renderable.shd_ref = 2.0f;  // Use sprite shader (2) for texture rendering
////        
////        // Initially hidden - will be toggled with Z key
////        transform.isVisible = objectsVisible;
////        
////        // Add the Transform component to the entity
////        // This automatically adds the entity to the GraphicsManager system
////        g_coordinator->AddComponent(newEntity, transform);
////        
////        // Store entity reference for toggle functionality
////        renderObjects.push_back(newEntity);
////    }
////    
////    std::cout << "Successfully created " << count << " objects!" << std::endl;
////    std::cout << "Press Z to toggle object visibility!" << std::endl;
////}
//
//// Function to toggle object visibility
//void ToggleObjectVisibility() {
//    auto* g_coordinator = Coordinator::GetInstance();
//    objectsVisible = !objectsVisible;
//    
//    for (Entity entity : renderObjects) {
//        auto& transform = g_coordinator->GetComponent<Framework::Transform>(entity);
//        transform.isVisible = objectsVisible;
//    }
//}
//
//CoreEngine* Engine = nullptr;
//
//int main() {
//#ifdef _DEBUG
//    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//#endif
//
//     // ----------------- Initialize Logging System -----------------
//    // GAM200graphics::Log::Init();
//    // GAM200_CORE_INFO("Core Logger Initialized.");
//    // GAM200_INFO("Client Logger Initialized.");
//
//    // ----------------- Create the Engine instance -----------------
//    Engine = new CoreEngine();
//
//    Engine->Setup();
//    // If CoreEngine::Init() also calls glewInit() or GraphicsManager->Initialize(),
//    // REMOVE those calls from CoreEngine::Init() or call Engine.Init() *here* only
//    // for non-GL things:
//    Engine->Init(); // <-- only if it does NOT touch OpenGL
//
//    // After you create the GLFW window and GL context
//    #ifdef ENABLE_EDITOR
//    MyEditor::Init(Engine->GetWindow());  // or your window pointer
//    #endif
//    // Initialize random seed
//    srand(static_cast<unsigned int>(time(nullptr)));
//    
//    // Create 2500 objects for rendering
//    //CreateRenderObjects(2500);
//      
//    // this is where the update() is 
//    Engine->GameLoop();
//
//
//    #ifdef ENABLE_EDITOR
//    MyEditor::Shutdown();
//    #endif
//
//    Engine->Exit();
//
//    MessageManager::DestroyInstance();
//    ProfilingManager::DestroyInstance();
//
//    // Clean up
//    delete Engine;
//    Engine = nullptr;
//
//#if 0
//    _CrtMemCheckpoint(&sNew); //take a snapshot (lock in the memory use again and now we compare :3)
//    if (_CrtMemDifference(&sDiff, &sOld, &sNew)) // if there is a difference
//    {
//        OutputDebugString("-----------_CrtMemDumpStatistics ---------\n");
//        _CrtMemDumpStatistics(&sDiff);
//        OutputDebugString("-----------_CrtMemDumpAllObjectsSince ---------\n");
//        _CrtMemDumpAllObjectsSince(&sOld);
//        OutputDebugString("-----------_CrtDumpMemoryLeaks ---------\n");
//        _CrtDumpMemoryLeaks();
//    }
//    else
//    {
//        OutputDebugString(" ////////////////////////////////////////////////////// \n");
//        OutputDebugString(" //                                                  // \n");
//        OutputDebugString(" //                NO MEMORY LEAKS !!!               // \n");
//        OutputDebugString(" //                                                  // \n");
//        OutputDebugString(" ////////////////////////////////////////////////////// \n");
//    }
//#endif
//
//    return 0;
//}