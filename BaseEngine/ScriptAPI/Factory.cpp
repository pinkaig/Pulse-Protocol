/******************************************************************************/
/**
* @file        Factory.cpp
* @project     Pulse Protocol
* @author      Chia Wei Xuan Rachael - 100%
* @brief	   Implementation of Factory wrapper providing C# scripts with entity 
*              instantiation from prefabs and entity destruction capabilities.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#pragma once
#include <msclr/marshal_cppstd.h>   // for converting C# to C++ string
#include "EngineInterface.h"
#define generic generic_workaround
#include <optional>
#include "Factory.h" 
#include "../CoreEngine/Factory/Factory.h"
#include "../CoreEngine/ECS/Coordinator.h"
#include "../CoreEngine/Prefab/PrefabManager.h"
#include "CoreEngine/Core/ScriptingBridge.h"
#undef generic


namespace ScriptAPI
{
	// In: Json name of the prefab u wanna spawn in 
	// Out: Entity ID that u can play with in script 
	int Factory::Instantiate(System::String^ scriptname)
	{
		if (System::String::IsNullOrEmpty(scriptname))
		{
			System::Console::WriteLine("Prefab is either empty or null\n");
			return -1;
		}

		// change c# string to c++ , and get prefab json pathway
		msclr::interop::marshal_context context;
		std::string prefabName = context.marshal_as<std::string>(scriptname);

		// Use PrefabManager to resolve the correct path via CoreEngine::GetAssetRoot()
		// This avoids the hardcoded relative path that breaks when the working directory
		// is not exactly where expected (e.g. different build configs / shipped builds).
		PrefabManager prefabMgr;
		std::string prefabPath = prefabMgr.GetPrefabPath(prefabName);

		// Use factory to create entity from .json file
		::Factory factory;
		auto new_entity = factory.CreateEntityFromJson(prefabPath, false);

		if (!new_entity.has_value())
		{
			System::Console::WriteLine("[Factory::Instantiate] Failed to instantiate prefab: " + scriptname);
			return -1;
		}

		int entityID = static_cast<int>(new_entity.value());

		EngineInterface::InQueueScripts(entityID); // add LATER instead!

		return entityID;
	}

	// delete entity using factory delete function :D
	void Factory::Destroy(int entityID)
	{
		::Factory factory;
		factory.DeleteEntity(entityID);
	}
}