/******************************************************************************/
/**
* @file        EngineInterface.h
* @project     Pulse Protocol
* @author      Chia Wei Xuan Rachael - 100%
* @brief       Declares the C++/CLI bridge class that serves as the static interface between the native C++ engine 
               and the managed C# scripting system, exposing script lifecycle management functions to native code.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#pragma once
#include "ScriptAPIClass.h"
#include <vector>

class CoreEngine;

namespace ScriptAPI
{
    // ref classes are classes in C#, value classes are structs in C#
    public ref class EngineInterface
    {
    public:
        static void Init(CoreEngine* coreEngine);
        static void Shutdown();
        static void HelloWorld();
        static bool AddScriptViaName(int entityID, System::String^ scriptName); 
        // ^ = A "handle" to something the garbage collector owns and will clean up for you

        static void ExecuteUpdate();
        static void ClearAllScripts();
        static void ClearEntityScript(int EntityID);

        // for adding a script to an enity when the scripts are being iterated
        // TLDR delay the add til after finish iterating thru them
        static void InQueueScripts(int entityID);
        static void ProcessQueuedScript();

        static void RemoveScriptFromEntity(int entityID, System::String^ scriptName);
        static const char* GetAvailableScripts();

        static void Reload();  // For hot reloading scripts
    
    private:
        using ScriptList = System::Collections::Generic::List<Script^>;
        static System::Collections::Generic::List<ScriptList^>^ scripts;                 // stores all scripts for all entities in your scene.
        static System::Collections::Generic::IEnumerable<System::Type^>^ scriptTypeList; // stores all available script types that you can attach to entities.

        // chatgpt easy examplination if u cant picture it (i am dumb thats why):
        // scriptTypeList →[PlayerScript, EnemyScript, CameraScript, ...]  // All available types
        //    ↓
        //    scripts[0] →[PlayerScript instance, CameraScript instance]      // Entity 0's scripts
        //    scripts[1] →[EnemyScript instance]                              // Entity 1's scripts  
        //    scripts[2] →[PlayerScript instance]                             // Entity 2's scripts

        static void updateScriptTypeList();

        // For hot reloading 
        static System::Runtime::Loader::AssemblyLoadContext^ loadContext;

        // A contianer for storing scripts that need to be added later
        // static System::Collections::Generic::List<System::Int32>^ QueuedScript;
        //using QueuedScriptList = System::Collections::Generic::List<int>;
        static std::vector<int>* QueuedScript;

        // To ensure that cleared scripts are not being updated when change scene
        static bool allScriptsCleared = false;

    internal:
        static CoreEngine* coreEngine = nullptr;
    };
}