/******************************************************************************/
/**
 * @file        Factory.h
 * @project     Pulse Protocol
 * @author		Chia Wei Xuan Rachael - 99%
 * @author		Goh Pin Kai (secondary) - 1%
 * @brief		Entity factory system providing CRUD operations for individual game
 *              entities via JSON prefab files and programmatic creation. Serves as
 *              the primary interface for entity instantiation in both editor (ImGui)
 *              and runtime contexts (spawning bullets, enemies, etc.).
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/
#pragma once
#include "pch/pch_temp.h"	
#include "Components/EntityType.h"
#include "Components/Health.h"
#include "Graphics/Transform.h"
//#include "Graphics/Animation.h"
//#include "Graphics/Renderable.h"
#include "GameLogic/GameLogic.h"
#include "CoreEngine/ECS/Types.h"
#include "CoreEngine/ECS/Coordinator.h"

//for .NET
#include "CoreEngine/Core/ImportExport.h" // for DLL_API

#include "Audio/AudioSource.h"
//#include "Graphics/Text.h"
//Serialize
#pragma warning(push, 0)
#include <rapidjson/document.h>     // for Document, Value, AllocatorType
#include <rapidjson/prettywriter.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h> // for StringBuffer
#pragma warning(pop)

// stupid warning just saying to make sure we are using same compilier type and runtime MDd. WHICH WE ARE SO WHY WARNING STILL??????????
#pragma warning(push)
#pragma warning(disable: 4251)


class DLL_API Factory
{
public: 
	std::optional<Entity> CreateEntityFromJson(std::string const& path, bool AttachScripts = true);
	// call onto default object function to get the basic SRT and render
	// then we check the file path from param and check for any components it wanna add 
	// call component deserialize functions with json filepath and entity as param
	// Added a attachscript param as we need to run thru all the scripts first then add this into the script list. 

	// Creates entity with component SRT and RENDER. ITS ALL HARD CODED SO AS TO NOT BE CHANGE IT. ALL entity has these.
	Entity DefaultObj(); 

	// Delete the entity u want. I know that i can just use g_coordinator but i want the option in here so its neater and for ImGui to call this function instead
	void DeleteEntity(Entity entity);

	// Save this entity into the json file, might make a save then delete function if needed.
	bool SaveEntityToJson(Entity entity, std::filesystem::path const& path);
private:

	// calls onto prefab json file only, it will just load what the param PrefabName is.
	//bool LoadPrefab(Entity entity, std::string const& PrefabName);

	// set at runtime in init from FilePathToGame
	std::string PrefabPathway; 

	CoreEngine* m_coreEngine = nullptr;
};

#pragma warning(pop)

// Prefab: something u want reuse for runtime(enemy, bullet), OH U SHOT A GUN, BULLET APPEARS, NOT PREFAB THEN modify
// default obj: all entity has SRT and render. anything else u add urself
// SCENE WILL JUST ONE SHOT RENDER ALL THE DIFF LOCATION AND STUFF. IT MUST ALL BE FILLED!!!!!
// For deserialize function, it will create the object, grab from json, then call g_coordinator->addcomponent(entity,obj), param is entity and json file
// save scene, change scene. ADD COMPONENT, REMOVE COMPONENT, ADD AND DELETE ENTITY