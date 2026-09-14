/******************************************************************************/
/**
 * @file       SceneManager.h
 * @project    Pulse Protocol
 * @author     Chia Wei Xuan Rachael - 90%
 * @author     Goh Pin Kai (secondary) - 4%
 * @author     Ban Kai Wei Benjamin (secondary) - 5%
 * @author     Leu Jun Yong (secondary) - 1%
 * 
 * @brief      Scene management system that inherits from Systems base class.
 *             Provides interface for scene lifecycle operations, entity tracking,
 *             and JSON serialization of game scenes.
 *
 *             Public Interface:
 *             - Init(): Reads "FirstScene" from config.json and loads it. Falls back to FirstScene.json if not set or missing.
 *               Sets initial CurrentScene and PreviousScene values
 *             - Update(float dt): Passive scene state tracking, monitors scene changes
 *             - LoadSceneFromJson(path): Parses JSON scene file, creates entities with
 *               components, auto-stops game during transition, registers GUI buttons
 *             - SaveCurrentScene(): Serializes current scene state to JSON file in
 *               ../../PulseEngine/JSON/[SceneName].json format
 *             - UnloadCurrentScene(): Destroys all entities in SceneEntityList, clears
 *               tracking container
 * 
 *				Scene State Management:
 *             - GrabCurrentScene(): Returns active scene name
 *             - ChangeCurrentScene(str): Updates CurrentScene identifier
 * 
 *				Entity Tracking:
 *             - IncrementEntityArray(entity): Adds entity to SceneEntityList for
 *               ownership tracking and cleanup
 *             - DecrementEntityArray(entity): Removes specific entity from list using
 *               std::find + erase (removes ONLY first matching entity)
 * 
 * @copyright  Copyright (C) 2026 DigiPen Institute of Technology.
 *             Reproduction or disclosure of this file or its contents without the
 *             prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/
#pragma once
#include "pch/pch_temp.h"

//#include "../PulseEngine/Components/DisplayName.h"
#include "CoreEngine/ECS/Types.h"
#include "CoreEngine/ECS/Coordinator.h"

//for .NET
#include "CoreEngine/Core/ImportExport.h" // for DLL_API


#pragma warning(push, 0)
#include <rapidjson/document.h>     
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h> 
#include <rapidjson/istreamwrapper.h>
#pragma warning(pop)

// .NET stuff
// stupid warning just saying to make sure we are using same compilier type and runtime MDd. WHICH WE ARE SO WHY WARNING STILL??????????
#pragma warning(push)
#pragma warning(disable: 4251)

extern std::vector<Entity> g_entitiesToDelete;

class DLL_API SceneManager: public Systems
{
public:
	SceneManager();

	void Init() override;              
	void Update(float dt) override;    
	//void Exit() override;

	void LoadSceneFromJson(std::string const& path);
	void SaveCurrentScene();
	void UnloadCurrentScene();
	void RestartCurrentScene();
	bool ConsumeSceneLoadedPulse();
	bool HasSceneLoadedPulse() const { return m_sceneLoadCompletedPulse; }

	std::string CurrentScene; // i need this public for me to call in other systems when i press the nbutton in hovered state

	/**
	 * @brief Serializes the entire current scene to a JSON string in memory
	 *
	 * This function is used by the Editor when Play is pressed.
	 * It saves ALL entities and ALL their components to a JSON string
	 * so we can restore the exact scene state when Stop is pressed.
	 *
	 * Unlike SaveCurrentScene() which writes to a file, this returns
	 * a string that stays in memory - faster and doesn't overwrite your saved scene.
	 *
	 * @return JSON string containing the entire scene state
	 */
	std::string SerializeToString();

	/**
	 * @brief Loads a scene from a JSON string in memory
	 *
	 * This function is used by the Editor when Stop is pressed.
	 * It restores the scene to the state captured by SerializeToString().
	 * All runtime changes made during Play are discarded.
	 *
	 * Unlike LoadSceneFromJson() which reads from a file, this reads
	 * from a string in memory - faster and doesn't need file I/O.
	 *
	 * @param jsonString The JSON string from SerializeToString()
	 */
	void LoadFromString(const std::string& jsonString);
	
	std::string GrabCurrentScene() const { return CurrentScene; }
	std::string GrabLastScene() const { return LastScene; }
	void ClearLastScene() { LastScene = ""; }
	void ChangeCurrentScene(std::string const& str) { CurrentScene = str; }
	void IncrementEntityArray(Entity entity) { SceneEntityList.push_back(entity); }
	void DecrementEntityArray(Entity entity)
	{
		auto victim = std::find(SceneEntityList.begin(), SceneEntityList.end(), entity);
		if (victim != SceneEntityList.end())
		{
			SceneEntityList.erase(victim);  // Erases ONLY the first victim that matches and kill it >:D
		}
	}

	bool SceneExists(std::string const& sceneNameOrPath) const;
	

	// to get a hold of coreengine instance for clearing scripts
	void SetCoreEngine(CoreEngine* engine) { m_coreEngine = engine; }

	// Grab SceneEentity container (for CoreEngine, Hot Reload, fill in the script again)
	const std::vector<Entity>& GetSceneEntities() const { return SceneEntityList; }


private:
	std::string PreviousScene;		// For change detection in Update()
	std::string LastScene;			// For ESC navigation - the scene we came FROM
	std::vector<Entity> SceneEntityList;

	std::filesystem::path GetManifestPathForScene(std::string const& sceneNameOrPath) const;
	bool LoadManifestForScene(std::string const& sceneNameOrPath, SceneManifest& outManifest) const;
	void UnloadCurrentSceneAssets();



	void Deserialize(const rapidjson::Value& in);
	void Serialize(rapidjson::Value& out, rapidjson::Document::AllocatorType& alloc) const;

	CoreEngine* m_coreEngine = nullptr;
	bool m_sceneLoadCompletedPulse = false;
};
//ALL ENTITY IN THE ARRAY MUST BE EXACTLY LIKE A PREFAB. But it is not a prefab, just all its component. (for scene json)

#pragma warning(pop)
