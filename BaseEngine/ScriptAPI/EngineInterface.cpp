/******************************************************************************/
/**
* @file        EngineInterface.cpp
* @project     Pulse Protocol
* @author      Chia Wei Xuan Rachael - 100%
* @brief       Manages the lifecycle and execution of C# scripts within the native C++ engine by providing script discovery, 
               instantiation, attachment, and update orchestration through the C++/CLI bridge.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
//#using "C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App\\8.0.22\\System.Private.CoreLib.dll"
#using <System.Runtime.dll>
#using <System.Runtime.Loader.dll>
#pragma warning(disable: 4945)  // Suppress assembly conflict warnings for dotnet System.Runtime.CoreLib and Loader :p

// dumb "error" that shows even tho it compiles and is a valid thing??!?!??!
#ifdef __INTELLISENSE__
#pragma diag_suppress 289
#pragma diag_suppress 2242
#endif

#define generic generic_workaround
#include "EngineInterface.h" 
#include "../CoreEngine/ECS/Coordinator.h"
#include "CoreEngine/Core/ScriptingBridge.h"
#include <string>
#pragma comment(lib, "PulseEngine.lib")
#undef generic



namespace ScriptAPI
{
    void EngineInterface::Init(CoreEngine* p_coreEngine)
    {
        using namespace System::IO;

        coreEngine = p_coreEngine;

        if (QueuedScript != nullptr)
        {
            delete QueuedScript;
        }
        QueuedScript = new std::vector<int>();
        
        // Create unloadable assembly context
        loadContext = gcnew System::Runtime::Loader::AssemblyLoadContext(nullptr, true);

        // Load assembly via FileStream (for hot reload)
        FileStream^ managedLibFile = File::Open("ManagedScripts.dll",
            FileMode::Open,
            FileAccess::Read,
            FileShare::Read);

        loadContext->LoadFromStream(managedLibFile);
        managedLibFile->Close();

        // Create the main container that will hold scripts for ALL entities
        scripts = gcnew System::Collections::Generic::List<ScriptList^>();
        const int ENTITY_COUNT = 5000; // same as the one in Types.h 

        // Create an empty script list for each possible entity in the engine
        // Each entity gets its own List<Script^> to hold multiple scripts
        for (int i = 0; i < ENTITY_COUNT; ++i)
        {
            scripts->Add(gcnew ScriptList()); // List<Script^> += ScriptList 
        }
        
        // Update list of scripts 
        updateScriptTypeList();
    }
    
    void EngineInterface::Reload()
    {
        System::Console::WriteLine("Reloading scripts!");

        // Clear ALL references to types from the old assembly
        scripts->Clear();
        scriptTypeList = nullptr;

        // Unload the assembly context
        loadContext->Unload();
        loadContext = nullptr;

        //// Force garbage collection to complete unloading
        System::GC::Collect();
        System::GC::WaitForPendingFinalizers();

        // Reload everything (new assembly)
        Init(coreEngine);

        System::Console::WriteLine("Scripts reloaded successfully!");
    }
    
    // Update script type list to help LINQ find all Script classes via .NET reflection
    namespace
    {
        // Used to temporarily store an assembly and one of its types together */
        ref struct Pair
        {
            System::Reflection::Assembly^ assembly;
            System::Type^ type;
        };

        // SelectMany helper: Gets all types exported from an assembly 
        System::Collections::Generic::IEnumerable<System::Type^>^ selectorFunc(System::Reflection::Assembly^ assembly)
        {
            return assembly->GetExportedTypes();
        }

        // SelectMany helper: Creates a Pair linking an assembly with one of its types
        Pair^ resultSelectorFunc(System::Reflection::Assembly^ assembly, System::Type^ type)
        {
            Pair^ p = gcnew Pair();
            p->assembly = assembly;
            p->type = type;
            return p;
        }

        // Where filter: Checks if a type is a concrete (non-abstract) Script subclass
        bool predicateFunc(Pair^ pair)
        {
            return pair->type->IsSubclassOf(Script::typeid) && !pair->type->IsAbstract;
        }
        
        // Select helper : Extracts just the Type from a Pair
        System::Type^ selectorFunc(Pair^ pair)
        {
            return pair->type;
        }

    }

    // Discover all available Script classes using .NET reflection
    // This builds a catalog of what scripts exist (PlayerScript, EnemyScript, etc.)
    void EngineInterface::updateScriptTypeList()
    {
        using namespace System;
        using namespace System::Reflection;
        using namespace System::Linq;
        using namespace System::Collections::Generic;

        // STEP 1: Get all loaded assemblies in the current AppDomain
        IEnumerable<Assembly^>^ assemblies = AppDomain::CurrentDomain->GetAssemblies();

        // STEP 2: SelectMany - Get all types from all assemblies and pair them together
        // This flattens: [[Assembly1Types], [Assembly2Types]] → [Type1, Type2, Type3, ...]
        Func<Assembly^, IEnumerable<Type^>^>^ collectionSelector = gcnew Func<Assembly^, IEnumerable<Type^>^>(selectorFunc);
        Func<Assembly^, Type^, Pair^>^ resultSelector = gcnew Func<Assembly^, Type^, Pair^>(resultSelectorFunc);
        IEnumerable<Pair^>^ selectManyResult = Enumerable::SelectMany(assemblies, collectionSelector, resultSelector);
        
        // STEP 3: Where - Filter to keep only concrete Script subclasses
        // Removes abstract classes, non-Script classes, etc.
        Func<Pair^, bool>^ predicate = gcnew Func<Pair^, bool>(predicateFunc);
        IEnumerable<Pair^>^ whereResult = Enumerable::Where(selectManyResult, predicate);
        
        // STEP 4: Select - Extract just the Type objects from the Pairs
        // Result: A list of Type objects representing all usable Script classes
        Func<Pair^, Type^>^ selector = gcnew Func<Pair^, Type^>(selectorFunc);
        scriptTypeList = Enumerable::Select(whereResult, selector);
    }

    // allow the native code to add scripts to different entities via script name by searching the list for a match and 
    // construct an instance of it to be added into the entity's own script list
    bool EngineInterface::AddScriptViaName(int entityId, System::String^ scriptName)
    {
        // Check if valid entityid. our range is 0 - 5000
        if (entityId < 0 || entityId > 5000)
        {
            return false;
        }

        // Remove any whitespaces just in case
        scriptName = scriptName->Trim();

        // Look for the correct script
        System::Type^ scriptType = nullptr;
        for each (System::Type ^ type in scriptTypeList)
        {
            if (type->FullName == scriptName || type->Name == scriptName)
            {
                scriptType = type;
                break;
            }
        }

        // Failed to get any script
        if (scriptType == nullptr)
        {
            return false;
        }

        // Create the script (Default construct an object of the specified type)
        Script^ script = safe_cast<Script^>(System::Activator::CreateInstance(scriptType));

        // make is so ECS ID and script ID is the same
        script->SetEntityID(entityId);

        // Add the script into this entities script container
        scripts[entityId]->Add(script);
        return true;
    }

    void EngineInterface::ClearAllScripts()
    {
        System::Console::WriteLine("Removing all scripts!\n");

        for each (ScriptList ^ entityScripts in scripts)
        {
            if (entityScripts != nullptr && entityScripts->Count > 0)
            {
                // Clear all the script for this entity
                entityScripts->Clear();

                //System::Console::WriteLine("1 Script cleared!\n");
            }
        }

        allScriptsCleared = true;
    }

    void EngineInterface::ClearEntityScript(int EntityID)
    {
        ScriptList^ entityScripts = scripts[EntityID];
        if (entityScripts != nullptr && entityScripts->Count > 0)
        {
            // Clear all the script for this entity
            entityScripts->Clear();

            System::Console::WriteLine("Entity Script is all deleted!\n");
        }
    }

    const char* EngineInterface::GetAvailableScripts()
    {
        using namespace System;
        using namespace System::Text;

        if (scriptTypeList == nullptr)
        {
            Console::WriteLine("[EngineInterface] WARNING: scriptTypeList is null!");
            return "";
        }

        // Create StringBuilder (don't make it static - managed types can't be static)
        StringBuilder^ scriptNames = gcnew StringBuilder();
        int count = 0;

        for each(System::Type ^ type in scriptTypeList)
        {
            if (count > 0)
            {
                scriptNames->Append(",");
            }
            scriptNames->Append(type->Name);
            count++;
        }

        String^ result = scriptNames->ToString();

#ifdef _DEBUG
        Console::WriteLine("[EngineInterface] GetAvailableScripts: Found {0} scripts", count);
        Console::WriteLine("[EngineInterface] Available: {0}", result);
#endif

        // Convert System::String to const char*
        // Use static to keep string alive between calls
        static std::string nativeResult = "";
        System::IntPtr ptr = System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(result);
        nativeResult = std::string(static_cast<const char*>(ptr.ToPointer()));
        System::Runtime::InteropServices::Marshal::FreeHGlobal(ptr);

        return nativeResult.c_str();
    }

    void EngineInterface::RemoveScriptFromEntity(int entityID, System::String^ scriptName)
    {
        // Check if valid entityid. our range is 0 - 5000
        if (entityID < 0 || entityID > 5000)
        {
            return;
        }

        // Remove any whitespaces
        scriptName = scriptName->Trim();

        ScriptList^ entityScripts = scripts[entityID];
        if (entityScripts == nullptr || entityScripts->Count == 0)
        {
            return;
        }

        // Find and remove the script with matching name
        for (int i = 0; i < entityScripts->Count; ++i)
        {
            Script^ script = entityScripts[i];
            System::Type^ scriptType = script->GetType();
            
            if (scriptType->FullName == scriptName || scriptType->Name == scriptName)
            {
                entityScripts->RemoveAt(i);
                System::Console::WriteLine("Removed script: {0} from entity {1}\n", scriptName, entityID);
                return;
            }
        }

        System::Console::WriteLine("Script not found: {0} on entity {1}\n", scriptName, entityID);
    }

    // iterate thru all of our scripts that has been created and execute our scripts.
    void EngineInterface::ExecuteUpdate()
    {
        allScriptsCleared = false;
     
        for each (ScriptList^ entityscriptlist in scripts)
        {
            // update each script available
            for each (Script^ script in entityscriptlist)
            {
                script->Update(); 
     
                if (allScriptsCleared)
                  {
                      System::Console::WriteLine("[executeupdate] all scripts cleared - stopping");
                      return;
                  }
            }
        }

        // add the scripts that was in queue
        ProcessQueuedScript();
    }

    void EngineInterface::InQueueScripts(int entityID)
    {
        System::Console::WriteLine("[EngineInterface] Queued script attachment for entity {0}", entityID);
        QueuedScript->push_back(entityID);
    }

    void EngineInterface::ProcessQueuedScript()
    {
        if (QueuedScript->empty())
        {
            return;
        }

        System::Console::WriteLine("[EngineInterface] Attaching scripts to {0} queued entities", QueuedScript->size());

        auto g_coordinator = Coordinator::GetInstance();

        //add those in the quqeue
        for (int entityID : *QueuedScript)
        {
            try  
            {
                if (!g_coordinator->HasComponent<LogicComponent>(entityID))
                {
                    continue;
                }

                auto& logic = g_coordinator->GetComponent<LogicComponent>(entityID);
                System::Console::WriteLine("  Found {0} scripts to attach", logic.ScriptName.size());

                // add the array of scripts
                for (const auto& scriptName : logic.ScriptName)
                {
                    if (!scriptName.empty())
                    {
                        System::Console::WriteLine("  Attempting to attach: '{0}'", gcnew System::String(scriptName.c_str()));

                        System::String^ managedName = gcnew System::String(scriptName.c_str());
                        bool success = AddScriptViaName(entityID, managedName);
                    }
                }
            }

            //just incase no got error cos i cant fking breakpoint :/
            catch (System::Exception^ ex)  
            {
                System::Console::WriteLine("EXCEPTION processing entity {0}: {1}", entityID, ex->Message);
                System::Console::WriteLine("Stack trace: {0}", ex->StackTrace);
            }
        }

        // clear for next loop
        QueuedScript->clear();
        System::Console::WriteLine("[EngineInterface] Script attachment complete! YIPPEEE\n");
    }

    void EngineInterface::Shutdown()
    {
        System::Console::WriteLine("Shutting down EngineInterface...");

        // Clean up vector
        if (QueuedScript != nullptr)
        {
            delete QueuedScript;
            QueuedScript = nullptr;
        }

        // Clean up scripts
        ClearAllScripts();

        System::Console::WriteLine("EngineInterface shutdown complete");
    }

    void EngineInterface::HelloWorld()
    {
        System::Console::WriteLine("Hello Managed World!");
        //coreEngine->HelloWorld();
    }
} 