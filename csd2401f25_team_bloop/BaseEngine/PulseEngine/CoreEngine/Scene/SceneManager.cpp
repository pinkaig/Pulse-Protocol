/******************************************************************************/
/**
 * @file       SceneManager.cpp
 * @project    Pulse Protocol
 * @author     Chia Wei Xuan Rachael - 90%
 * @author     Goh Pin Kai (secondary) - 4%
 * @author     Ban Kai Wei Benjamin (secondary) - 5%
 * @author     Leu Jun Yong (secondary) - 1%
 * 
 * @brief      Manages scene lifecycle, serialization, and entity ownership across
 *             game scenes (MainMene and level1 for now) .
 *
 *             Core Responsibilities:
 *             - Scene Loading: Parses JSON scene files, creates entities with components,
 *               registers GUI buttons, and assigns default LayerTag if missing
 *             - Scene Unloading: Destroys all entities in SceneEntityList and clears tracking
 *             - Scene Saving: Serializes current scene state to JSON with all entity
 *               components (Transform, Renderable, Animation, LogicComponent, Health,
 *               AABB, GUIButton, Name, NamingComponent)
 *             - Scene Tracking: Maintains SceneEntityList for ownership, PreviousScene
 *               for save operations, CurrentScene for active scene name
 *
 * @copyright  Copyright (C) 2026 DigiPen Institute of Technology.
 *             Reproduction or disclosure of this file or its contents without the
 *             prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once
#include "SceneManager.h"
#include "Graphics/Text.h"
#include "Graphics/Layer.h"
#include "CoreEngine/Assets.h"
#include "Components/Health.h"
#include "Graphics/Animation.h"
#include "Graphics/Transform.h"
#include "Graphics/Renderable.h"
#include "GameLogic/GameLogic.h"
#include "Components/EntityType.h"
#include "Components/ParentChild.h"
#include "Components/DisplayName.h"
#include "CoreEngine/Factory/Factory.h"
#include "CoreEngine/Core/CoreEngine.h" // for FilePathToGame
//#include "CoreEngine/Core/CoreEngine.h"  // For GetFunctionPtr
#include "CoreEngine/Prefab/PrefabInstance.h"
//#include "../Components/AABB.h"

#include "SceneManifest.h"
#include "CoreEngine/Asset/AssetsManager.h"

extern bool g_countdownFinished;

//Starting scene will be Main Menu
SceneManager::SceneManager()
{

}


// For now its 
// 1. When u start a new game, have a asset and a JSon folder alr inside
// 2. When u start the game, have a default scene which u cant change unless u change it in config.
//std::filesystem::path IsThereStartingSceneInConfig()
//{
//	if (true) // can open the file given?  
//	{
//		
//	}
//
//	else
//	{
//		// make a scene for them and add a folder too? 
//	}
//}

std::filesystem::path GetScenePath(const std::string& sceneNameOrPath) {
	std::filesystem::path basePath = FilePathToGame;

	// Strip "../../{GameName}/" prefix so we get a path relative to the game root
	if (sceneNameOrPath.find("../../") == 0) 
	{
		// so like prefix = ../../PulseProtocol/
		std::string prefix = "../../" + FilePathToGame.filename().string() + "/";
		std::string relativePath = sceneNameOrPath.substr(prefix.length());
		return (basePath / relativePath).make_preferred();
	}

	// If it's just a scene name (e.g., "Level1"), build the full path
	if (sceneNameOrPath.find(".json") == std::string::npos) {
		return (basePath / "JSON" / (sceneNameOrPath + ".json")).make_preferred();
	}

	// Otherwise assume it's already a relative path from PulseEngine
	return (basePath / sceneNameOrPath).make_preferred();
}

void SceneManager::Init()
{
	std::filesystem::path jsonDir = FilePathToGame / "JSON";

	// Open config.json and grab the "FirstScene" value	
	std::string firstSceneName = "";
	std::filesystem::path configPath = jsonDir / "config.json";
	if (std::filesystem::exists(configPath))
	{
		std::ifstream f(configPath);
		if (f.is_open())
		{
			rapidjson::IStreamWrapper isw(f);
			rapidjson::Document doc;
			doc.ParseStream(isw);

			// seralized the scene name in config.json
			if (doc.IsObject() && doc.HasMember("config") && doc["config"].IsObject())
			{
				const auto& cfg = doc["config"];
				if (cfg.HasMember("FirstScene") && cfg["FirstScene"].IsString())
					firstSceneName = cfg["FirstScene"].GetString();
			}
		}
	}

	// Decide which scene to load
	std::filesystem::path scenePath;
	if (!firstSceneName.empty())
	{
		// load whatever scene that is in config.json
		scenePath = jsonDir / (firstSceneName + ".json"); 

		// if cannot find the file in the config.json, just load default.
		if (!std::filesystem::exists(scenePath))
		{
			std::cout << "[SceneManager] '" << firstSceneName << ".json' not found, loading FirstScene.json instead!" << std::endl;
			scenePath = jsonDir / "FirstScene.json";
		}
	}
	else
	{
		// No FirstScene in config.json so just load the default blank scene
		scenePath = jsonDir / "FirstScene.json";
	}

	std::cout << "[SceneManager] Loading: " << scenePath << std::endl;
	LoadSceneFromJson(scenePath.string());

	PreviousScene = CurrentScene;
}

void SceneManager::Update(float dt)
{
	(void)dt;

	if (CurrentScene != PreviousScene)
	{
		// Save the scene we're LEAVING for ESC navigation
		LastScene = PreviousScene;

		std::filesystem::path scenePath = GetScenePath(CurrentScene);
		LoadSceneFromJson(scenePath.string());

		// Update PreviousScene for change detection next frame
		PreviousScene = CurrentScene;
	}
}

bool SceneManager::ConsumeSceneLoadedPulse()
{
	if (!m_sceneLoadCompletedPulse)
	{
		return false;
	}

	m_sceneLoadCompletedPulse = false;
	return true;
}



void SceneManager::SaveCurrentScene()
{
#ifdef _DEBUG
	std::cout << "\n================================================" << std::endl;
	std::cout << "   [SceneManager]: Saving scene in progress!" << std::endl;
	std::cout << "================================================" << std::endl;
#endif

	using namespace rapidjson;
	auto* g_coordinator = Coordinator::GetInstance();
	Factory factory;

	Document d;
	d.SetObject();
	auto& alloc = d.GetAllocator();

	// Make it so that it saves from the BaseEngine onwards, which we can then save to PulseEngfie/Json file
	char exePath[MAX_PATH];
	GetModuleFileNameA(GetModuleHandleA("PulseEngine.dll"), exePath, MAX_PATH);
	std::filesystem::path basePath = std::filesystem::path(exePath).parent_path(); // Remove exe name

	std::cout << std::filesystem::absolute(basePath) << std::endl;
	std::cout << std::filesystem::absolute(basePath.parent_path()) << std::endl;
	std::cout << std::filesystem::absolute(basePath.parent_path().parent_path()) << std::endl;
	std::cout << std::filesystem::absolute(basePath.parent_path().parent_path().parent_path()) << std::endl;

	std::filesystem::path path = FilePathToGame / "JSON" / (CurrentScene + ".json");
	

	// grab scene name for the json file name
#ifdef _DEBUG
	//std::filesystem::path path = "../../PulseEngine/JSON/" + CurrentScene + ".json"; // file we want to save data into
	std::cout << std::filesystem::absolute(path) << std::endl;
#endif
	// Save scene name first
	Value SceneName;
	SceneName.SetString(CurrentScene.c_str(), static_cast<SizeType>(CurrentScene.length()), alloc);
	d.AddMember("scene", SceneName, alloc);


	// First we make a container for entity
	Value JsonEntityArray(kArrayType);

	// ═══════════════════════════════════════════════════════════════════════════
	// Build entity-to-index map (must match save order exactly)
	// ═══════════════════════════════════════════════════════════════════════════
	std::unordered_map<Entity, int> entityToIndex;
	int entityIndex = 0;
	for (auto const& entity : g_coordinator->GetAllEntities())
	{
		if (g_coordinator->HasComponent<Name>(entity)) {
			auto& name = g_coordinator->GetComponent<Name>(entity).name;
			if (!name.empty() && name[0] == '_') {
				continue;  // Skip engine-managed entities
			}
		}
		entityToIndex[entity] = entityIndex;
		entityIndex++;
	}

	// save one entity one at a time into 1 Json file
	for (auto const& entity : g_coordinator->GetAllEntities())
	{
		// Skip engine-managed entities (names starting with underscore)
		if (g_coordinator->HasComponent<Name>(entity)) {
			auto& name = g_coordinator->GetComponent<Name>(entity).name;
			if (!name.empty() && name[0] == '_') {
				continue;  // Don't save _FPS_Display, _TopSystem_Display, etc.
			}
		} 

		// We make an object first for JsonEntityArray[i]
		Value entityObj(kObjectType);

		// tehn for our components, we make it an array of components
		Value componentsArray(kArrayType);

		// check all the components if they have it
		if (g_coordinator->HasComponent<NamingComponent>(entity))
		{
			Value ComponentObj(rapidjson::kObjectType);
			Value NameVal;

			auto& nam = g_coordinator->GetComponent<NamingComponent>(entity);
			nam.Serialize(NameVal, alloc);

			ComponentObj.AddMember("Typing", NameVal, alloc);
			componentsArray.PushBack(ComponentObj, alloc);

		}

		if (g_coordinator->HasComponent<Framework::Transform>(entity))
		{
			Value ComponentObj(rapidjson::kObjectType);
			Value TransfromVal;

			auto& script = g_coordinator->GetComponent<Framework::Transform>(entity);
			script.Serialize(TransfromVal, alloc);

			ComponentObj.AddMember("transform", TransfromVal, alloc);
			componentsArray.PushBack(ComponentObj, alloc);

		}

		if (g_coordinator->HasComponent<Framework::Renderable>(entity))
		{
			Value ComponentObj(rapidjson::kObjectType);
			Value RenderVal;

			auto& render = g_coordinator->GetComponent<Framework::Renderable>(entity);

			/*std::cout << "[DEBUG] Saving entity with spriteName: " << render.spriteName << std::endl;*/

			render.Serialize(RenderVal, alloc);

			ComponentObj.AddMember("render", RenderVal, alloc);
			componentsArray.PushBack(ComponentObj, alloc);

		}

		if (g_coordinator->HasComponent<Framework::Animation>(entity))
		{
			Value ComponentObj(rapidjson::kObjectType);
			Value AnimationVal;

			auto& animation = g_coordinator->GetComponent<Framework::Animation>(entity);
			animation.Serialize(AnimationVal, alloc);

			ComponentObj.AddMember("animation", AnimationVal, alloc);
			componentsArray.PushBack(ComponentObj, alloc);

		}

		if (g_coordinator->HasComponent<LogicComponent>(entity))
		{
			Value ComponentObj(rapidjson::kObjectType);
			Value ScriptVal;

			auto& script = g_coordinator->GetComponent<LogicComponent>(entity);
			script.Serialize(ScriptVal, alloc);

			ComponentObj.AddMember("script", ScriptVal, alloc);
			componentsArray.PushBack(ComponentObj, alloc);
		}


		if (g_coordinator->HasComponent<Name>(entity))
		{
			Value ComponentObj(rapidjson::kObjectType);
			Value NameVal;

			auto& name = g_coordinator->GetComponent<Name>(entity);
			name.Serialize(NameVal, alloc);

			ComponentObj.AddMember("name", NameVal, alloc);
			componentsArray.PushBack(ComponentObj, alloc);

		}

		if (g_coordinator->HasComponent<Health>(entity))
		{
			Value ComponentObj(rapidjson::kObjectType);
			Value HealthVal;

			auto& health = g_coordinator->GetComponent<Health>(entity);
			health.Serialize(HealthVal, alloc);

			ComponentObj.AddMember("Health", HealthVal, alloc);
			componentsArray.PushBack(ComponentObj, alloc);

		}

		if (g_coordinator->HasComponent<AABB>(entity))
		{
			Value ComponentObj(rapidjson::kObjectType);
			Value AABBVal;

			auto& aabb = g_coordinator->GetComponent<AABB>(entity);
			aabb.Serialize(AABBVal, alloc);

			ComponentObj.AddMember("AABB", AABBVal, alloc);
			componentsArray.PushBack(ComponentObj, alloc);

		}

		// Save PrefabInstance component if entity has it
		if (g_coordinator->HasComponent<PrefabInstance>(entity))
		{
			Value ComponentObj(rapidjson::kObjectType);
			Value PrefabInstanceVal;

			auto& prefabInstance = g_coordinator->GetComponent<PrefabInstance>(entity);
			prefabInstance.Serialize(PrefabInstanceVal, alloc);

			ComponentObj.AddMember("prefabInstance", PrefabInstanceVal, alloc);
			componentsArray.PushBack(ComponentObj, alloc);
		}

		// Save LayerTag component if entity has it
		if (g_coordinator->HasComponent<LayerTag>(entity))
		{
			Value ComponentObj(rapidjson::kObjectType);
			Value LayerTagVal;

			auto& layerTag = g_coordinator->GetComponent<LayerTag>(entity);
			layerTag.Serialize(LayerTagVal, alloc);

			ComponentObj.AddMember("layerTag", LayerTagVal, alloc);
			componentsArray.PushBack(ComponentObj, alloc);
		}

		// Save AudiOsource component if entity has it
		if (g_coordinator->HasComponent<AudioSource>(entity))
		{
			Value ComponentObj(rapidjson::kObjectType);
			Value AudioSourceVal;

			auto& audioSrc = g_coordinator->GetComponent<AudioSource>(entity);
			audioSrc.Serialize(AudioSourceVal, alloc);

			ComponentObj.AddMember("audiosource", AudioSourceVal, alloc);
			componentsArray.PushBack(ComponentObj, alloc);
		}

		// Save TextComponent if entity has it
		if (g_coordinator->HasComponent<TextComponent>(entity))
		{
			Value ComponentObj(rapidjson::kObjectType);
			Value TextVal;

			auto& textComp = g_coordinator->GetComponent<TextComponent>(entity);
			textComp.Serialize(TextVal, alloc);

			ComponentObj.AddMember("TextComponent", TextVal, alloc);
			componentsArray.PushBack(ComponentObj, alloc);
		}

		// Save Parent component
		if (g_coordinator->HasComponent<Parent>(entity))
		{
			auto& parent = g_coordinator->GetComponent<Parent>(entity);
			if (parent.parent != static_cast<Entity>(-1))
			{
				auto it = entityToIndex.find(parent.parent);
				if (it != entityToIndex.end())
				{
//#ifdef _DEBUG
//					std::string childName = g_coordinator->HasComponent<n>(entity) ?
//						g_coordinator->GetComponent<n>(entity).name : "Unknown";
//					std::string parentName = g_coordinator->HasComponent<n>(parent.parent) ?
//						g_coordinator->GetComponent<n>(parent.parent).name : "Unknown";
//					std::cout << "[SAVE] " << childName << " (Entity " << entity << ") parent="
//						<< parentName << " (Entity " << parent.parent << ") -> saving index "
//						<< it->second << std::endl;
//#endif
					Value ComponentObj(rapidjson::kObjectType);
					Value ParentVal(kObjectType);
					ParentVal.AddMember("parent", it->second, alloc);
					ComponentObj.AddMember("Parent", ParentVal, alloc);
					componentsArray.PushBack(ComponentObj, alloc);
				}
			}
		}

		// Save Children component
		if (g_coordinator->HasComponent<Children>(entity))
		{
			auto& children = g_coordinator->GetComponent<Children>(entity);
			if (!children.children.empty())
			{
				Value ComponentObj(rapidjson::kObjectType);
				Value ChildrenVal(kObjectType);
				Value ChildArray(kArrayType);

				// ✓ ADD THIS LOOP - iterate through children and add indices
				for (Entity child : children.children)
				{
					auto it = entityToIndex.find(child);
					if (it != entityToIndex.end())
					{
						ChildArray.PushBack(it->second, alloc);
					}
				}

				if (ChildArray.Size() > 0)
				{
					ChildrenVal.AddMember("children", ChildArray, alloc);
					ComponentObj.AddMember("Children", ChildrenVal, alloc);
					componentsArray.PushBack(ComponentObj, alloc);
				}
			}
		}

		// So for this entity, we add compoennt array
		entityObj.AddMember("components", componentsArray, alloc);

		// we add the entity type and its compoents into 1 JsonEntityArray[i]
		JsonEntityArray.PushBack(entityObj, alloc); // add to entity array list
	}

	//make it to an actual array in Json file
	d.AddMember("entities", JsonEntityArray, alloc);

	StringBuffer buffer;
	PrettyWriter<StringBuffer> writer(buffer); // Use PrettyWriter for readable JSON
	d.Accept(writer);

	namespace fs = std::filesystem;
	//fs::create_directories("../../PulseEngine/JSON");  // store in folder Json
	fs::create_directories(path.parent_path());  // store in folder 

	// check if can open, if can then put data into Json file
	std::ofstream ofs{ path };
	if (!ofs.is_open())
	{
		std::cerr << "Error: Cannot open file for writing: " << path << "\n";
		return;
	}

	ofs << buffer.GetString();
	if (!ofs.good())
	{
		std::cerr << "Error: Failed to write JSON to file: " << path << "\n";
		return;
	}

#ifdef _DEBUG
	std::cout << "\n================================================" << std::endl;
	std::cout << "  [SceneManager]: " << CurrentScene << " has been saved!" << std::endl;
	std::cout << "================================================" << std::endl;
#endif
}

void SceneManager::UnloadCurrentScene()
{
#ifdef _DEBUG
	std::cout << "\n================================================" << std::endl;
	std::cout << "       [SceneManager] Unloading in progress!" << std::endl;
	std::cout << "================================================" << std::endl;
#endif

	auto* g_coordinator = Coordinator::GetInstance();
#ifdef _DEBUG
	//std::cout << "[SceneManager::UnloadCurrentScene] Got coordinator: " << (g_coordinator ? "YES" : "NO") << std::endl;
	//std::cout << "[SceneManager::UnloadCurrentScene] SceneEntityList size: " << SceneEntityList.size() << std::endl;
#endif

	//// Call Exit() on all scripts BEFORE destroying entities
	//auto logicSystem = g_coordinator->GetSystem<LogicSystem>();
	//std::cout << "[SceneManager::UnloadCurrentScene] Got LogicSystem: " << (logicSystem ? "YES" : "NO") << std::endl;
	/*if (logicSystem) {
		std::cout << "[SceneManager::UnloadCurrentScene] Calling Exit() on scripts..." << std::endl;
		int exitCount = 0;
		for (auto const& entity : SceneEntityList) {
			std::cout << "  -> Checking entity " << entity << std::endl;
			if (g_coordinator->HasComponent<LogicComponent>(entity)) {
				std::cout << "  -> Entity " << entity << " has LogicComponent" << std::endl;
				auto& logic = g_coordinator->GetComponent<LogicComponent>(entity);
				auto script = logicSystem->GrabScript(logic.ScriptName);
				if (script) {
					std::cout << "  -> Calling Exit() on " << logic.ScriptName << " for entity " << entity << std::endl;
					script->Exit(entity);
					exitCount++;
				}
				else {
					std::cout << "  -> WARNING: Could not get script for " << logic.ScriptName << std::endl;
				}
			}
			else {
				std::cout << "  -> Entity " << entity << " does NOT have LogicComponent" << std::endl;
			}
		}
		std::cout << "[SceneManager::UnloadCurrentScene] Total Exit() calls: " << exitCount << std::endl;
	}
	else {
		std::cout << "[SceneManager::UnloadCurrentScene] WARNING: LogicSystem not found!" << std::endl;
	}*/

	// Stop AudioSource channels before destroying entities
	auto* g_coord = Coordinator::GetInstance();
	auto audioMgr = g_coord->GetSystem<AudioManager>();
	if (audioMgr) {
		for (auto const& entity : SceneEntityList) {

			if (!g_coordinator->IsAlive(entity))
			{
				continue;
			}

			if (g_coord->HasComponent<AudioSource>(entity)) {
				auto& audioSrc = g_coord->GetComponent<AudioSource>(entity);
				if (audioSrc.channel) {
					audioSrc.channel->stop();
					audioSrc.channel = nullptr;
				}
			}
		}

		// IMPORTANT: Stop global BGM (played via AudioComponent.PlayBGM())
		audioMgr->StopBGM();
	}

	// Unload only scene-owned assets from manifest
	UnloadCurrentSceneAssets();

	// CLEAR THE ASSETS!
	//auto gfxMgr = g_coordinator->GetSystem<GraphicsManager>();
	//if (gfxMgr) {
	//	gfxMgr->Clear();  // Calls glDeleteTextures()
	//}


	// Clear the Csharp script when scene change
#ifdef _DEBUG
	std::cout << "[SceneManager::UnloadCurrentScene] Clearing C# scripts..." << std::endl;
#endif
	if (m_coreEngine && m_coreEngine->clearScriptsFunc)
	{
		m_coreEngine->clearScriptsFunc();
	}

	else
	{
		std::cout << "[SceneManager::UnloadCurrentScene] WARNING: CoreEngine pointer not set!" << std::endl;
	}


	// Delete all the entity from SceneEntity Container
	for (auto const& entity : SceneEntityList)
	{
		g_coordinator->DestroyEntity(entity);
	}

	
	//This class owns entity container is cleared
	SceneEntityList.clear();
#ifdef _DEBUG
	std::cout << "\n================================================" << std::endl;
	std::cout << "    [SceneManager] Scene successfully unloaded!" << std::endl;
	std::cout << "================================================" << std::endl;
#endif
}

void SceneManager::LoadSceneFromJson(std::string const& path) // TAKES IN SCENE JSON FILE 
{
	m_sceneLoadCompletedPulse = false;

	// Reset countdown/beat state on scene load
	g_countdownFinished = false;

	UnloadCurrentScene();

	std::string resolvedPath = path;
	if (path.find("../../") == 0) {
		resolvedPath = GetScenePath(path).string();
	}

#ifdef _DEBUG
	std::cout << "\n================================================" << std::endl;
	std::cout << "       [SceneManager] Loading in progress!" << std::endl;
	std::cout << "================================================" << std::endl;
#endif

	SceneManifest manifest;
	if (LoadManifestForScene(resolvedPath, manifest))
	{
		// Load shared groups first, if not loaded yet
		for (auto const& groupName : manifest.sharedGroups)
		{
			if (!mAssets.LoadSharedAssets(groupName))
			{
				std::cerr << "[SceneManager] Failed to load shared asset group: "
					<< groupName << "\n";
			}
			else
			{
				std::cout << "[SceneManager] Loaded shared asset group: "
					<< groupName << "\n";
			}
		}

		// Then load scene-local assets
		if (!mAssets.PreloadSceneAssets(manifest))
		{
			std::cerr << "[SceneManager] Some assets failed to preload for scene: "
				<< manifest.sceneName << "\n";
		}
		else
		{
			std::cout << "[SceneManager] Load Asset Manifest for scene success: "
				<< manifest.sceneName << "\n";
		}
	}


	// just a for loop going thru all the entity arrray list and calling the factory function to create them 
	auto* g_coordinator = Coordinator::GetInstance();
	using namespace rapidjson;
	Factory factory;

	std::ifstream ifs(resolvedPath);
	
	if (!ifs.is_open())
	{
		std::cerr << "Error: Cannot open file for reading: " << resolvedPath << "\n";
		return;
	}

	IStreamWrapper isw(ifs);
	Document d;
	d.ParseStream(isw);

	//check if Json file has a scene
	if (!(d.HasMember("scene")))
	{
		std::cerr << "scene is missing/wrong!" << std::endl;
		return;
	}

	// check if Json file has array of entities
	if (!(d.HasMember("entities")))
	{
		std::cerr << "entity array is missing/wrong!" << std::endl;
		return;
	}

	// Loop thru all the entity in the entity list
	const auto& SceneEntityArray = d["entities"];
	for (SizeType i = 0; i < SceneEntityArray.Size(); ++i)
	{
		auto const& JsonEntity = SceneEntityArray[i];
		// just creatre entity here so u dont have to call function 

		// check if there are compoent for each entity
		if (!(JsonEntity.HasMember("components") && JsonEntity["components"].IsArray()))
		{
			std::cerr << "Components for entities are missing/wrong!" << std::endl;
			continue;
		}

		//Entity entity = factory.DefaultObj();	   // create entity with transform and render component.

		Entity entity = g_coordinator->CreateEntity();
		const auto& components = JsonEntity["components"];  // Get the components array

		// array of all the component in this entity
		for (SizeType j = 0; j < components.Size(); ++j)
		{
			const auto& component = components[j];

			if (component.HasMember("Typing") && component["Typing"].IsObject())
			{
				NamingComponent n;
				n.Deserialize(component["Typing"]);
				g_coordinator->AddComponent<NamingComponent>(entity, n);
			}

			if (component.HasMember("transform") && component["transform"].IsObject())
			{
				Framework::Transform t;
				t.Deserialize(component["transform"]);
				g_coordinator->AddComponent<Framework::Transform>(entity, t);
			}

			if (component.HasMember("render") && component["render"].IsObject())
			{
				Framework::Renderable r;
				r.Deserialize(component["render"]);
				g_coordinator->AddComponent<Framework::Renderable>(entity, r);
			}

			if (component.HasMember("animation") && component["animation"].IsObject())
			{
				Framework::Animation a;
				a.Deserialize(component["animation"]);
				g_coordinator->AddComponent<Framework::Animation>(entity, a);
			}

			if (component.HasMember("script") && component["script"].IsObject())
			{
				LogicComponent l;
				l.Deserialize(component["script"]);
				g_coordinator->AddComponent<LogicComponent>(entity, l);
			}

			if (component.HasMember("name") && component["name"].IsObject())
			{
				Name n;
				n.Deserialize(component["name"]);
				g_coordinator->AddComponent<Name>(entity, n);
			}

			if (component.HasMember("Health") && component["Health"].IsObject())
			{
				Health h;
				h.Deserialize(component["Health"]);
				g_coordinator->AddComponent<Health>(entity, h);
			}

			if (component.HasMember("AABB") && component["AABB"].IsObject())
			{
				AABB aabb;
				aabb.Deserialize(component["AABB"]);
				g_coordinator->AddComponent<AABB>(entity, aabb);
			}

			// Load PrefabInstance component if present
			if (component.HasMember("prefabInstance") && component["prefabInstance"].IsObject())
			{
				PrefabInstance pi;
				pi.Deserialize(component["prefabInstance"]);
				g_coordinator->AddComponent<PrefabInstance>(entity, pi);
			}

			// Load LayerTag
			if (component.HasMember("layerTag") && component["layerTag"].IsObject())
			{
				LayerTag layerTag;
				layerTag.Deserialize(component["layerTag"]);
				g_coordinator->AddComponent<LayerTag>(entity, layerTag);
			}

			if (component.HasMember("audiosource") && component["audiosource"].IsObject())
			{
				AudioSource audioSrc;
				audioSrc.Deserialize(component["audiosource"]);
				g_coordinator->AddComponent<AudioSource>(entity, audioSrc);
			}

			if (component.HasMember("TextComponent") && component["TextComponent"].IsObject())
			{
				TextComponent txtComp;
				txtComp.Deserialize(component["TextComponent"]);
				g_coordinator->AddComponent<TextComponent>(entity, txtComp);
			}
		}

		if (!g_coordinator->HasComponent<LayerTag>(entity)) {
			LayerTag tag{};

			// Sensible defaults:
			//if (g_coordinator->HasComponent<GUIButton>(entity)) {
			//	// UI stuff goes to UI layer and on top
			//	tag.mask = LAYER_UI;
			//	tag.orderInLayer = 2;
			//	tag.z = 0.0f;
			//}
			//else 

			// check for UI first
			//if (g_coordinator->HasComponent<UIElement>(entity)) {
			//	tag.mask = LAYER_UI;
			//	tag.orderInLayer = 10;
			//	tag.z = 100.0f;
			//}

			if (g_coordinator->HasComponent<Framework::Renderable>(entity)) {
				// Background quad heuristic: mdl_ref==0
				auto& r = g_coordinator->GetComponent<Framework::Renderable>(entity);
				const bool isBackground = (static_cast<int>(r.mdl_ref) == 0);
				tag.mask = isBackground ? LAYER_BACKGROUND : LAYER_WORLD;
				tag.orderInLayer = isBackground ? 0 : 1;
				tag.z = 0.0f;
			}
			else {
				// Everything else defaults to world
				tag.mask = LAYER_WORLD;
				tag.orderInLayer = 1;
				tag.z = 0.0f;
			}

			g_coordinator->AddComponent(entity, tag);
		}

		if (g_coordinator->HasComponent<Name>(entity)) {
			auto& name = g_coordinator->GetComponent<Name>(entity).name;
			if (!name.empty() && name[0] == '_') {
				//std::cout << "[SceneManager] Skipping engine-managed entity: " << name << std::endl;
				g_coordinator->DestroyEntity(entity);
				continue;  // Skip to next entity in JSON
			}
		}

		// save obj to a vector for deleting later
		SceneEntityList.push_back(entity);
	}

	for (rapidjson::SizeType i = 0; i < SceneEntityArray.Size(); ++i)
	{
		if (i >= SceneEntityList.size()) continue;
		Entity entity = SceneEntityList[i];

		const auto& entityData = SceneEntityArray[i];
		if (!entityData.HasMember("components") || !entityData["components"].IsArray())
			continue;

		const auto& components = entityData["components"];

		for (rapidjson::SizeType j = 0; j < components.Size(); ++j)
		{
			const auto& component = components[j];

			// NEW: Load Parent (remap index to entity)
			if (component.HasMember("Parent") && component["Parent"].IsObject())
			{
				const auto& parentObj = component["Parent"];
				if (parentObj.HasMember("parent") && parentObj["parent"].IsInt())
				{
					int parentIndex = parentObj["parent"].GetInt();
					if (parentIndex >= 0 && parentIndex < static_cast<int>(SceneEntityList.size()))
					{
						Parent p;
						p.parent = SceneEntityList[parentIndex];
						g_coordinator->AddComponent<Parent>(entity, p);

#ifdef _DEBUG
						std::string childName = g_coordinator->HasComponent<Name>(entity) ?
							g_coordinator->GetComponent<Name>(entity).name : "Unknown";
						std::string parentName = g_coordinator->HasComponent<Name>(p.parent) ?
							g_coordinator->GetComponent<Name>(p.parent).name : "Unknown";
						/*std::cout << "[LOAD] " << childName << " (Entity " << entity
							<< ") parent index " << parentIndex << " -> " << parentName
							<< " (Entity " << p.parent << ")" << std::endl;*/
#endif
					}
				}
			}

			// NEW: Load Children (remap indices to entities)
			if (component.HasMember("Children") && component["Children"].IsObject())
			{
				const auto& childrenObj = component["Children"];
				if (childrenObj.HasMember("children") && childrenObj["children"].IsArray())
				{
					Children c;
					const auto& childArray = childrenObj["children"];

#ifdef _DEBUG
					std::string parentName = g_coordinator->HasComponent<Name>(entity) ?
						g_coordinator->GetComponent<Name>(entity).name : "Unknown";
					/*std::cout << "[LOAD] " << parentName << " (Entity " << entity << ") children: ";*/
#endif

					for (rapidjson::SizeType k = 0; k < childArray.Size(); ++k)
					{
						if (childArray[k].IsInt())
						{
							int childIndex = childArray[k].GetInt();
							if (childIndex >= 0 && childIndex < static_cast<int>(SceneEntityList.size()))
							{
								c.children.push_back(SceneEntityList[childIndex]);

#ifdef _DEBUG
								std::string childName = g_coordinator->HasComponent<Name>(SceneEntityList[childIndex]) ?
									g_coordinator->GetComponent<Name>(SceneEntityList[childIndex]).name : "Unknown";
								/*std::cout << childName << " (index " << childIndex << " -> Entity "
									<< SceneEntityList[childIndex] << "), ";*/
#endif
							}
						}
					}

#ifdef _DEBUG
					/*std::cout << std::endl;*/
#endif

					if (!c.children.empty())
					{
						g_coordinator->AddComponent<Children>(entity, c);
					}
				}
			}
		}
	}

	// ═══════════════════════════════════════════════════════════════════════════
	// DEBUG: Print loaded parent-child relationships
	// ═══════════════════════════════════════════════════════════════════════════
#ifdef _DEBUG
	/*std::cout << "\n[SceneManager] === Parent/Children Debug ===" << std::endl;*/
	for (Entity e : SceneEntityList)
	{
		std::string name = "Unknown";
		if (g_coordinator->HasComponent<Name>(e))
			name = g_coordinator->GetComponent<Name>(e).name;

		bool hasParent = g_coordinator->HasComponent<Parent>(e);
		bool hasChildren = g_coordinator->HasComponent<Children>(e);

		/*std::cout << "Entity " << e << " (" << name << "): ";*/

		if (hasParent)
		{
			Entity parent = g_coordinator->GetComponent<Parent>(e).parent;
			std::string parentName = "Unknown";
			if (g_coordinator->HasComponent<Name>(parent))
				parentName = g_coordinator->GetComponent<Name>(parent).name;
			/*std::cout << "Parent=" << parent << " (" << parentName << ") ";*/
		}

		if (hasChildren)
		{
			auto& children = g_coordinator->GetComponent<Children>(e).children;
			/*std::cout << "Children=[";*/
			for (Entity c : children)
			{
				std::string childName = "Unknown";
				if (g_coordinator->HasComponent<Name>(c))
					childName = g_coordinator->GetComponent<Name>(c).name;
				/*std::cout << c << " (" << childName << "), ";*/
			}
			/*std::cout << "]";*/
		}

		//if (!hasParent && !hasChildren)
		//	std::cout << "(no parent/children)";

		/*std::cout << std::endl;*/
	}
	/*std::cout << "==========================================\n" << std::endl;*/
#endif


	ChangeCurrentScene(d["scene"].GetString());


	// Add them to C# script container 
	auto addScriptFunc = m_coreEngine->GetFunctionPtr<bool(*)(int, const char*)>(
		"ScriptAPI",
		"ScriptAPI.EngineInterface",
		"AddScriptViaName"
	);

#ifdef _DEBUG
	//std::cout << "[SceneManager] addScriptFunc = " << (addScriptFunc ? "FOUND" : "NULL") << "\n";
	//std::cout << "[SceneManager] SceneEntityList.size() = " << SceneEntityList.size() << "\n";
#endif

	/*for (Entity entity : SceneEntityList)
	{
		if (g_coordinator->HasComponent<LogicComponent>(entity))
		{
			auto& logic = g_coordinator->GetComponent<LogicComponent>(entity);

			for (const auto& scriptName : logic.ScriptName)
			{
				if (!scriptName.empty())
				{
					addScriptFunc(entity, scriptName.c_str());
				}
			}
		}
	}*/

	if (!addScriptFunc)
	{
		std::cerr << "[SceneManager] CRITICAL ERROR: AddScriptViaName not found!\n";
		std::cerr << "  Script engine may not be initialized!\n";
	}
	else
	{
		int totalAttached = 0;

		for (Entity entity : SceneEntityList)
		{
			/*std::cout << "[SceneManager] Checking entity " << entity << "\n";*/

			if (g_coordinator->HasComponent<LogicComponent>(entity))
			{
				auto& logic = g_coordinator->GetComponent<LogicComponent>(entity);
				/*std::cout << "  -> Has LogicComponent\n";
				std::cout << "  -> ScriptName.size() = " << logic.ScriptName.size() << "\n";*/

				for (size_t i = 0; i < logic.ScriptName.size(); ++i)
				{
					const auto& scriptName = logic.ScriptName[i];
					/*std::cout << "  -> ScriptName[" << i << "] = '" << scriptName << "'\n";*/

					if (!scriptName.empty())
					{
						/*std::cout << "  -> Calling AddScriptViaName(" << entity << ", '" << scriptName << "')\n";*/

						bool success = addScriptFunc(entity, scriptName.c_str());

						/*std::cout << "  -> Result: " << (success ? "SUCCESS" : "FAILED") << "\n";*/

						if (success)
						{
							totalAttached++;
						}
						else
						{
							std::cerr << "  -> ERROR: Failed to attach '" << scriptName << "' to entity " << entity << "\n";
						}
					}
					else
					{
						std::cout << "  -> Skipped (empty name)\n";
					}
				}
			}
			else
			{
				//std::cout << "  -> No LogicComponent\n";
			}
		}

#ifdef _DEBUG
		std::cout << "\n[SceneManager] ========== SUMMARY ==========\n";
		std::cout << "[SceneManager] Total entities checked: " << SceneEntityList.size() << "\n";
		std::cout << "[SceneManager] Total scripts attached: " << totalAttached << "\n";
		std::cout << "[SceneManager] ============================\n";
#endif
	}


	// Force Debug Display Text visibility based on build mode
	for (auto entity : SceneEntityList)
	{
		if (g_coordinator->HasComponent<TextComponent>(entity) && g_coordinator->HasComponent<Name>(entity))
		{
			auto& name = g_coordinator->GetComponent<Name>(entity).name;
			if (name == "Debug Display Text")
			{
				auto& txt = g_coordinator->GetComponent<TextComponent>(entity);
#ifdef _DEBUG
				txt.visible = true;
#else
				txt.visible = false;
#endif
				break;
			}
		}
	}


	PreviousScene = CurrentScene;
	m_sceneLoadCompletedPulse = true;

#ifdef _DEBUG
	std::cout << "\n===================================================" << std::endl;
	std::cout << "   [Scene Manager]: " << CurrentScene << " loaded successfully!" << std::endl;
	std::cout << "===================================================" << std::endl;

	std::cout << std::filesystem::absolute(path) << std::endl;
#endif
}

void SceneManager::RestartCurrentScene()
{
#ifdef _DEBUG
	std::cout << "\n======================================================" << std::endl;
	std::cout << " [SceneManager] Relaoding current scene in progress!" << std::endl;
	std::cout << "======================================================" << std::endl;
#endif

	auto* g_coordinator = Coordinator::GetInstance();

	// Clear the Csharp script when scene change
#ifdef _DEBUG
	std::cout << "[SceneManager::UnloadCurrentScene] Clearing C# scripts..." << std::endl;
#endif
	if (m_coreEngine && m_coreEngine->clearScriptsFunc)
	{
		m_coreEngine->clearScriptsFunc();
	}

	else
	{
		std::cout << "[SceneManager::UnloadCurrentScene] WARNING: CoreEngine pointer not set!" << std::endl;
	}

	//kill all entity
	for(auto entity: SceneEntityList)
	{
		g_coordinator->DestroyEntity(entity);
	}


	//This class owns entity container is cleared
	SceneEntityList.clear();
	
	// Reload the scene without unloading the assets, 
	std::string sceneToRestart = CurrentScene;
	UnloadCurrentScene();
	std::filesystem::path scenePath = GetScenePath(sceneToRestart);
	LoadSceneFromJson(scenePath.string());

#ifdef _DEBUG
	std::cout << "\n================================================" << std::endl;
	std::cout << "    [SceneManager] Scene successfully reloaded!" << std::endl;
	std::cout << "================================================" << std::endl;
#endif
}



void SceneManager::Serialize(rapidjson::Value& out, rapidjson::Document::AllocatorType& alloc) const
{
	using namespace rapidjson;
	out.SetObject();
	out.AddMember("SceneName", Value(CurrentScene.c_str(), alloc), alloc);
}

void SceneManager::Deserialize(const rapidjson::Value& in)
{
	if (!in.IsObject()) throw std::runtime_error("Invalid JSON for LogicComponent");
	if (in.HasMember("SceneName")) CurrentScene = in["SceneName"].GetString(); // already assigns it
}

/**
 * @brief Serializes the entire current scene to a JSON string
 *
 * This is basically a copy of SaveCurrentScene() but instead of writing
 * to a file, it returns the JSON as a string. Used by Editor for Play/Stop.
 *
 * @return JSON string containing all entities and their components
 */
std::string SceneManager::SerializeToString()
{
#ifdef _DEBUG
	std::cout << "\n================================================" << std::endl;
	std::cout << "   [SceneManager]: Serializing scene to string..." << std::endl;
	std::cout << "================================================" << std::endl;
#endif

	using namespace rapidjson;
	auto* g_coordinator = Coordinator::GetInstance();

	// Create JSON document
	Document d;
	d.SetObject();
	auto& alloc = d.GetAllocator();

	// ─────────────────────────────────────────────────────────────────────────
	// Save scene name first
	// ─────────────────────────────────────────────────────────────────────────
	Value SceneName;
	SceneName.SetString(CurrentScene.c_str(), static_cast<SizeType>(CurrentScene.length()), alloc);
	d.AddMember("scene", SceneName, alloc);

	// Create array for all entities
	Value JsonEntityArray(kArrayType);

	// ─────────────────────────────────────────────────────────────────────────
	// Build entity-to-index map for parent/child relationships
	// This maps each Entity ID to its position in the save order
	// ─────────────────────────────────────────────────────────────────────────
	std::unordered_map<Entity, int> entityToIndex;
	int entityIndex = 0;
	for (auto const& entity : g_coordinator->GetAllEntities())
	{
		// Skip engine-managed entities (names starting with underscore like _FPS_Display)
		if (g_coordinator->HasComponent<Name>(entity))
		{
			auto& name = g_coordinator->GetComponent<Name>(entity).name;
			if (!name.empty() && name[0] == '_')
			{
				continue;
			}
		}
		entityToIndex[entity] = entityIndex;
		entityIndex++;
	}

	// ─────────────────────────────────────────────────────────────────────────
	// Serialize each entity and all its components
	// ─────────────────────────────────────────────────────────────────────────
	for (auto const& entity : g_coordinator->GetAllEntities())
	{
		// Skip engine-managed entities
		if (g_coordinator->HasComponent<Name>(entity))
		{
			auto& name = g_coordinator->GetComponent<Name>(entity).name;
			if (!name.empty() && name[0] == '_')
			{
				continue;
			}
		}

		Value entityObj(kObjectType);
		Value componentsArray(kArrayType);

		// ═══════════════════════════════════════════════════════════════════
		// Serialize ALL component types (same as SaveCurrentScene)
		// ═══════════════════════════════════════════════════════════════════

		// NamingComponent
		if (g_coordinator->HasComponent<NamingComponent>(entity))
		{
			Value ComponentObj(kObjectType);
			Value val;
			auto& comp = g_coordinator->GetComponent<NamingComponent>(entity);
			comp.Serialize(val, alloc);
			ComponentObj.AddMember("Typing", val, alloc);
			componentsArray.PushBack(ComponentObj, alloc);
		}

		// Transform
		if (g_coordinator->HasComponent<Framework::Transform>(entity))
		{
			Value ComponentObj(kObjectType);
			Value val;
			auto& comp = g_coordinator->GetComponent<Framework::Transform>(entity);
			comp.Serialize(val, alloc);
			ComponentObj.AddMember("transform", val, alloc);
			componentsArray.PushBack(ComponentObj, alloc);
		}

		// Renderable
		if (g_coordinator->HasComponent<Framework::Renderable>(entity))
		{
			Value ComponentObj(kObjectType);
			Value val;
			auto& comp = g_coordinator->GetComponent<Framework::Renderable>(entity);
			comp.Serialize(val, alloc);
			ComponentObj.AddMember("render", val, alloc);
			componentsArray.PushBack(ComponentObj, alloc);
		}

		// Animation
		if (g_coordinator->HasComponent<Framework::Animation>(entity))
		{
			Value ComponentObj(kObjectType);
			Value val;
			auto& comp = g_coordinator->GetComponent<Framework::Animation>(entity);
			comp.Serialize(val, alloc);
			ComponentObj.AddMember("animation", val, alloc);
			componentsArray.PushBack(ComponentObj, alloc);
		}

		// LogicComponent (Scripts)
		if (g_coordinator->HasComponent<LogicComponent>(entity))
		{
			Value ComponentObj(kObjectType);
			Value val;
			auto& comp = g_coordinator->GetComponent<LogicComponent>(entity);
			comp.Serialize(val, alloc);
			ComponentObj.AddMember("script", val, alloc);
			componentsArray.PushBack(ComponentObj, alloc);
		}

		// Name (n component)
		if (g_coordinator->HasComponent<Name>(entity))
		{
			Value ComponentObj(kObjectType);
			Value val;
			auto& comp = g_coordinator->GetComponent<Name>(entity);
			comp.Serialize(val, alloc);
			ComponentObj.AddMember("name", val, alloc);
			componentsArray.PushBack(ComponentObj, alloc);
		}

		// Health
		if (g_coordinator->HasComponent<Health>(entity))
		{
			Value ComponentObj(kObjectType);
			Value val;
			auto& comp = g_coordinator->GetComponent<Health>(entity);
			comp.Serialize(val, alloc);
			ComponentObj.AddMember("Health", val, alloc);
			componentsArray.PushBack(ComponentObj, alloc);
		}

		// AABB (collision)
		if (g_coordinator->HasComponent<AABB>(entity))
		{
			Value ComponentObj(kObjectType);
			Value val;
			auto& comp = g_coordinator->GetComponent<AABB>(entity);
			comp.Serialize(val, alloc);
			ComponentObj.AddMember("AABB", val, alloc);
			componentsArray.PushBack(ComponentObj, alloc);
		}

		// PrefabInstance
		if (g_coordinator->HasComponent<PrefabInstance>(entity))
		{
			Value ComponentObj(kObjectType);
			Value val;
			auto& comp = g_coordinator->GetComponent<PrefabInstance>(entity);
			comp.Serialize(val, alloc);
			ComponentObj.AddMember("prefabInstance", val, alloc);
			componentsArray.PushBack(ComponentObj, alloc);
		}

		// LayerTag
		if (g_coordinator->HasComponent<LayerTag>(entity))
		{
			Value ComponentObj(kObjectType);
			Value val;
			auto& comp = g_coordinator->GetComponent<LayerTag>(entity);
			comp.Serialize(val, alloc);
			ComponentObj.AddMember("layerTag", val, alloc);
			componentsArray.PushBack(ComponentObj, alloc);
		}

		// AudioSource
		if (g_coordinator->HasComponent<AudioSource>(entity))
		{
			Value ComponentObj(kObjectType);
			Value val;
			auto& comp = g_coordinator->GetComponent<AudioSource>(entity);
			comp.Serialize(val, alloc);
			ComponentObj.AddMember("audiosource", val, alloc);
			componentsArray.PushBack(ComponentObj, alloc);
		}

		// TextComponent - THIS IS THE IMPORTANT ONE FOR YOUR COUNTDOWN!
		if (g_coordinator->HasComponent<TextComponent>(entity))
		{
			Value ComponentObj(kObjectType);
			Value val;
			auto& comp = g_coordinator->GetComponent<TextComponent>(entity);
			comp.Serialize(val, alloc);
			ComponentObj.AddMember("TextComponent", val, alloc);
			componentsArray.PushBack(ComponentObj, alloc);
		}

		// Parent component (for hierarchy)
		if (g_coordinator->HasComponent<Parent>(entity))
		{
			auto& parent = g_coordinator->GetComponent<Parent>(entity);
			if (parent.parent != static_cast<Entity>(-1))
			{
				auto it = entityToIndex.find(parent.parent);
				if (it != entityToIndex.end())
				{
					Value ComponentObj(kObjectType);
					Value ParentVal(kObjectType);
					ParentVal.AddMember("parent", it->second, alloc);
					ComponentObj.AddMember("Parent", ParentVal, alloc);
					componentsArray.PushBack(ComponentObj, alloc);
				}
			}
		}

		// Children component (for hierarchy)
		if (g_coordinator->HasComponent<Children>(entity))
		{
			auto& children = g_coordinator->GetComponent<Children>(entity);
			if (!children.children.empty())
			{
				Value ComponentObj(kObjectType);
				Value ChildrenVal(kObjectType);
				Value ChildArray(kArrayType);

				for (Entity child : children.children)
				{
					auto it = entityToIndex.find(child);
					if (it != entityToIndex.end())
					{
						ChildArray.PushBack(it->second, alloc);
					}
				}

				if (ChildArray.Size() > 0)
				{
					ChildrenVal.AddMember("children", ChildArray, alloc);
					ComponentObj.AddMember("Children", ChildrenVal, alloc);
					componentsArray.PushBack(ComponentObj, alloc);
				}
			}
		}

		// Add components to entity object
		entityObj.AddMember("components", componentsArray, alloc);

		// Add entity to array
		JsonEntityArray.PushBack(entityObj, alloc);
	}

	// Add entities array to document
	d.AddMember("entities", JsonEntityArray, alloc);

	// ─────────────────────────────────────────────────────────────────────────
	// Convert JSON document to string (instead of writing to file)
	// ─────────────────────────────────────────────────────────────────────────
	StringBuffer buffer;
	PrettyWriter<StringBuffer> writer(buffer);
	d.Accept(writer);

	return buffer.GetString();
}

bool SceneManager::SceneExists(std::string const& sceneNameOrPath) const
{
    // Reuse the exact same path resolver used by scene loading
    std::filesystem::path scenePath = GetScenePath(sceneNameOrPath);

#ifdef _DEBUG
    std::cout << "SceneExists check path: " << scenePath.string() << std::endl;
#endif

    return std::filesystem::exists(scenePath) &&
           std::filesystem::is_regular_file(scenePath);
}


/**
 * @brief Loads a scene from a JSON string in memory
 *
 * This is used by the Editor when Stop is pressed to restore the scene
 * to its state before Play was pressed.
 *
 * It works by:
 * 1. Writing the JSON string to a temporary file
 * 2. Calling LoadSceneFromJson() to load from that temp file
 * 3. Deleting the temporary file
 *
 * This reuses existing loading logic rather than duplicating it.
 *
 * @param jsonString The JSON string from SerializeToString()
 */
void SceneManager::LoadFromString(const std::string& jsonString)
{
	if (jsonString.empty())
	{
		return;
	}

	// ─────────────────────────────────────────────────────────────────────────
	// Write JSON string to a temporary file
	// We use a temp file so we can reuse the existing LoadSceneFromJson() logic
	// ─────────────────────────────────────────────────────────────────────────
	std::string tempPath = GetScenePath("JSON/_temp_editor_restore.json").string();

	std::ofstream ofs(tempPath);
	if (!ofs.is_open())
	{
		std::cerr << "[SceneManager] ERROR: Could not create temp file!" << std::endl;
		return;
	}

	ofs << jsonString;
	ofs.close();

	// ─────────────────────────────────────────────────────────────────────────
	// Load from the temp file (reuses all existing loading logic)
	// This handles clearing scripts, destroying entities, creating new ones, etc.
	// ─────────────────────────────────────────────────────────────────────────
	LoadSceneFromJson(tempPath);

	// ─────────────────────────────────────────────────────────────────────────
	// Delete the temporary file (cleanup)
	// ─────────────────────────────────────────────────────────────────────────
	std::remove(tempPath.c_str());
}

std::filesystem::path SceneManager::GetManifestPathForScene(std::string const& sceneNameOrPath) const
{
	std::filesystem::path p(sceneNameOrPath);

	// If caller already passed a full manifest path, keep it
	std::string pStr = p.string();
	if (pStr.size() >= std::string(".manifest.json").size() &&
		pStr.ends_with(".manifest.json"))
	{
		return p;
	}

	if (pStr.size() >= std::string(".json").size() &&
		pStr.ends_with(".json"))
	{
		return p.replace_extension(".manifest.json");
	}

	std::filesystem::path scenePath = GetScenePath(sceneNameOrPath);
	std::string sceneStr = scenePath.string();

	if (sceneStr.size() >= std::string(".manifest.json").size() &&
		sceneStr.ends_with(".manifest.json"))
	{
		return scenePath;
	}

	return scenePath.replace_extension(".manifest.json");
}

bool SceneManager::LoadManifestForScene(std::string const& sceneNameOrPath, SceneManifest& outManifest) const
{
	std::filesystem::path manifestPath = GetManifestPathForScene(sceneNameOrPath);

	if (!std::filesystem::exists(manifestPath))
	{
		std::cout << "[SceneManager] No manifest found for scene: "
			<< manifestPath << "\n";
		return false;
	}

	if (!LoadSceneManifestFile(outManifest, manifestPath.string()))
	{
		std::cerr << "[SceneManager] Failed to load manifest: "
			<< manifestPath << "\n";
		return false;
	}

	return true;
}

void SceneManager::UnloadCurrentSceneAssets()
{
	if (CurrentScene.empty())
	{
		return;
	}

	SceneManifest oldManifest;
	if (!LoadManifestForScene(CurrentScene, oldManifest))
	{
		std::cout << "[SceneManager] No unload manifest for current scene: "
			<< CurrentScene << "\n";
		return;
	}

	mAssets.UnloadSceneAssets(oldManifest);
}

