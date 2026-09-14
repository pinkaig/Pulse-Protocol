/******************************************************************************/
/**
 * @file        Factory.cpp
 * @project     Pulse Protocol
 * @author		Chia Wei Xuan Rachael (primary) - 80%
 * @author      Ban Kai Wei Benjamin (secondary) - 10%
 * @author      Reginald Lew Yee Ren (secondary) - 10%
 * @brief		Entity factory system for creating, deleting, and serializing individual
 *              game entities from JSON prefab files or programmatically. Handles entity
 *              lifecycle integration with SceneManager tracking and ECS Coordinator.
 *
 *				CreateEntityFromJson(path) -> std::optional<Entity>:
 *             - Loads entity from JSON prefab file (single entity definition)
 *             - Parses components array, deserializes each component type
 *             - Creates entity via Coordinator::CreateEntity()
 *             - Registers entity with SceneManager::IncrementEntityArray() for tracking
 *             - Auto-assigns default LayerTag{ LAYER_WORLD, 0, 0.0f } if missing
 *             - Returns std::optional<Entity>: entity ID on success, std::nullopt on failure
 *             - Supported components: NamingComponent (Typing), Transform, Renderable,
 *               Animation, LogicComponent (script), Name, Health, AABB
 *
 *
 *				SaveEntityToJson(entity, path) -> bool:
 *             - Serializes single entity to JSON prefab file
 *             - Checks for each component type, serializes if present
 *             - Creates components array structure (no scene wrapper)
 *             - Uses RapidJSON PrettyWriter for human-readable output
 *             - Auto-creates parent directory structure via std::filesystem
 *             - Returns true on successful write, false on file errors
 *
 *			   DeleteEntity(entity):
 *             - Two-phase deletion: scene tracking removal + ECS destruction
 *             - Calls SceneManager::DecrementEntityArray() to remove from scene ownership
 *             - Calls Coordinator::DestroyEntity() to clean up ECS components/systems
 *             - Used by ImGui editor and game logic (e.g., EnemyScript collision death)
 *             - Critical for preventing dangling entity references in SceneEntityList
 *
 *             DefaultObj() -> Entity:
 *             - Creates minimal entity for ImGui editor manipulation
 *             - Default components: Transform (200x200 scale, zero rotation/position),
 *               empty Renderable, empty Animation, LayerTag (LAYER_WORLD)
 *             - Automatically registered with SceneManager tracking
 *             - Provides base template for editor-created entities
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/
#pragma once
#include "Factory.h"
#include "Graphics/Text.h"
#include "Graphics/Animation.h"
#include "Graphics/Renderable.h"
#include "CoreEngine/Assets.h"
#include "Components/DisplayName.h"
#include "Components/ParentChild.h"


using namespace rapidjson;

// change to optional for bool(if loaded succeessfully) and entity ID(for chloe)
// added file path that will be given from either scene manager/ImGui
// Read from file, loop thru a check if have type and component. Add component base on type.
std::optional<Entity> Factory::CreateEntityFromJson(std::string const &path, bool attachScripts)
{
	auto *g_coordinator = Coordinator::GetInstance();

	// path iis the name of the prefab you want to create
	// std::string filePath = "../../PulseEngine/JSON/test.json";//AssetsManager.mPrefabMap[path];
	// std::string savefilePath = "../../PulseEngine/JSON/test2.json";

	std::ifstream ifs(path);

	std::cerr << "Trying to open: " << std::filesystem::absolute(path) << "\n";

	if (!ifs.is_open())
	{
		std::cerr << "Error: Cannot open file for reading: " << path << "\n";
		return false;
	}

	IStreamWrapper isw(ifs);
	Document d;
	d.ParseStream(isw);

	// check if Json file has component array
	if (!(d.HasMember("components") && d["components"].IsArray()))
	{
		std::cerr << "Components are missing/wrong!" << std::endl;
		return false;
	}

	Entity entity = g_coordinator->CreateEntity();
	const auto &components = d["components"]; // Get the components array

	// run thru all the components in the component array
	for (rapidjson::SizeType i = 0; i < components.Size(); ++i)
	{
		const auto &component = components[i];
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

		if (component.HasMember("LayerTag") && component["LayerTag"].IsObject())
		{
			LayerTag layerTag;
			layerTag.Deserialize(component["LayerTag"]);
			g_coordinator->AddComponent<LayerTag>(entity, layerTag);
		}
		if (component.HasMember("audiosource") && component["audiosource"].IsObject())
		{
			AudioSource as;
			as.Deserialize(component["audiosource"]);
			g_coordinator->AddComponent<AudioSource>(entity, as);
		}
		if (component.HasMember("TextComponent") && component["TextComponent"].IsObject())
		{
			TextComponent txt_component;
			txt_component.Deserialize(component["TextComponent"]);
			g_coordinator->AddComponent<TextComponent>(entity, txt_component);
		}

		if (component.HasMember("Parent") && component["Parent"].IsObject())
		{
			Parent p;
			p.Deserialize(component["Parent"]);
			g_coordinator->AddComponent<Parent>(entity, p);
		}

		if (component.HasMember("Children") && component["Children"].IsObject())
		{
			Children c;
			c.Deserialize(component["Children"]);
			g_coordinator->AddComponent<Children>(entity, c);
		}
	}
	// add entity to list of entity in the current scene
	auto sceneManager = g_coordinator->GetSystem<SceneManager>();
	sceneManager->IncrementEntityArray(entity);
	
	
	if (!g_coordinator->HasComponent<LayerTag>(entity))
	{
		LayerTag defaultLayer;
		defaultLayer.mask = LAYER_WORLD;
		defaultLayer.sortingLayer = 1;    // World
		defaultLayer.orderInLayer = 0;
		defaultLayer.z = 0.0f;
		g_coordinator->AddComponent(entity, defaultLayer);
	}
	

	
	// Add them to C# script container AND ONLY IF IT NOT ITERATING THRU THE SCRIPTS 
	if (attachScripts && g_coordinator->HasComponent<LogicComponent>(entity))
	{
		auto addScriptFunc = Engine->GetFunctionPtr<bool(*)(int, const char*)>(
			"ScriptAPI",
			"ScriptAPI.EngineInterface",
			"AddScriptViaName"
		);

		auto& logic = g_coordinator->GetComponent<LogicComponent>(entity);

		for (const auto& scriptName : logic.ScriptName)
		{
			if (!scriptName.empty())
			{
				addScriptFunc(entity, scriptName.c_str());
			}
		}
	}
	
	
	return entity;
}

// ONLY FOR ImGui
Entity Factory::DefaultObj()
{
	auto *g_coordinator = Coordinator::GetInstance();
	Entity entity = g_coordinator->CreateEntity();
	Framework::Transform t;
	t.Scale = {200.f, 200.f};
	t.Rotation = {0, 0};
	t.Pos = {0, 0};

	Framework::Renderable r;

	Framework::Animation a;

	// Add default Name component so entity can be renamed in hierarchy
	Name n;
	n.name = "GameObject"; // Default name

	// Default LayerTag with sortingLayer
	LayerTag layer;
	layer.mask = LAYER_WORLD;
	layer.sortingLayer = 1;    // 0=Background, 1=World, 2=UI
	layer.orderInLayer = 0;
	layer.z = 0.0f;	

	g_coordinator->AddComponent(entity, t);
	g_coordinator->AddComponent(entity, r);
	g_coordinator->AddComponent(entity, a);
	g_coordinator->AddComponent(entity, n);
	g_coordinator->AddComponent(entity, layer);

	// add entity to list of entity in the current scene
	auto sceneManager = g_coordinator->GetSystem<SceneManager>();
	sceneManager->IncrementEntityArray(entity);
	return entity;
}

// Used in ImGui and for when smth dies. Making it a function so that ImGui can just grab factory related stuff
void Factory::DeleteEntity(Entity entity)
{
	auto *g_coordinator = Coordinator::GetInstance();
	auto sceneManager = g_coordinator->GetSystem<SceneManager>();

	// Remove entities script before being delted
	auto clearEntityScriptFunc = Engine->GetFunctionPtr<void(*)(int)>(
		"ScriptAPI",
		"ScriptAPI.EngineInterface",
		"ClearEntityScript"
	);
	clearEntityScriptFunc(entity);


	sceneManager->DecrementEntityArray(entity); // KILL THE ENTITY FROM SCENE ENTITY CONTAINER >:D

	g_coordinator->DestroyEntity(entity);
}

// check if entity has what components, then just serialize to Json file
bool Factory::SaveEntityToJson(Entity entity, std::filesystem::path const &path)
{
	auto *g_coordinator = Coordinator::GetInstance();
	using namespace rapidjson;

	Document d;
	d.SetObject();
	auto &alloc = d.GetAllocator();

	// we want our components to be an array
	Value componentsArray(kArrayType);

	if (g_coordinator->HasComponent<NamingComponent>(entity))
	{
		Value ComponentObj(rapidjson::kObjectType);
		Value TypeVal;

		auto &objtype = g_coordinator->GetComponent<NamingComponent>(entity);
		objtype.Serialize(TypeVal, alloc);

		ComponentObj.AddMember("Typing", TypeVal, alloc);
		componentsArray.PushBack(ComponentObj, alloc);
	}

	// check if entity have these compoennts, if have then serialize to json file
	if (g_coordinator->HasComponent<Framework::Transform>(entity))
	{
		Value ComponentObj(rapidjson::kObjectType);
		Value TransformVal;

		auto &transform = g_coordinator->GetComponent<Framework::Transform>(entity);
		transform.Serialize(TransformVal, alloc);

		ComponentObj.AddMember("transform", TransformVal, alloc); // so now it will be "transform":{}
		componentsArray.PushBack(ComponentObj, alloc);			  // push the entire transform data into component array
	}

	// check if entity have these compoennts, if have then serialize to json file
	if (g_coordinator->HasComponent<Framework::Renderable>(entity))
	{
		Value ComponentObj(rapidjson::kObjectType);
		Value RenderVal;

		auto &render = g_coordinator->GetComponent<Framework::Renderable>(entity);
		render.Serialize(RenderVal, alloc);

		ComponentObj.AddMember("render", RenderVal, alloc);
		componentsArray.PushBack(ComponentObj, alloc);
	}

	if (g_coordinator->HasComponent<Framework::Animation>(entity))
	{
		Value ComponentObj(rapidjson::kObjectType);
		Value AnimationVal;

		auto &animation = g_coordinator->GetComponent<Framework::Animation>(entity);
		animation.Serialize(AnimationVal, alloc);

		ComponentObj.AddMember("animation", AnimationVal, alloc);
		componentsArray.PushBack(ComponentObj, alloc);
	}

	if (g_coordinator->HasComponent<LogicComponent>(entity))
	{
		Value ComponentObj(rapidjson::kObjectType);
		Value ScriptVal;

		auto &script = g_coordinator->GetComponent<LogicComponent>(entity);
		script.Serialize(ScriptVal, alloc);

		ComponentObj.AddMember("script", ScriptVal, alloc);
		componentsArray.PushBack(ComponentObj, alloc);
	}

	if (g_coordinator->HasComponent<Name>(entity))
	{
		Value ComponentObj(rapidjson::kObjectType);
		Value NameVal;

		auto &name = g_coordinator->GetComponent<Name>(entity);
		name.Serialize(NameVal, alloc);

		ComponentObj.AddMember("name", NameVal, alloc);
		componentsArray.PushBack(ComponentObj, alloc);
	}

	if (g_coordinator->HasComponent<Health>(entity))
	{
		Value ComponentObj(rapidjson::kObjectType);
		Value HealthVal;

		auto &health = g_coordinator->GetComponent<Health>(entity);
		health.Serialize(HealthVal, alloc);

		ComponentObj.AddMember("Health", HealthVal, alloc);
		componentsArray.PushBack(ComponentObj, alloc);
	}

	if (g_coordinator->HasComponent<AABB>(entity))
	{
		Value ComponentObj(rapidjson::kObjectType);
		Value AABBVal;

		auto &aabb = g_coordinator->GetComponent<AABB>(entity);
		aabb.Serialize(AABBVal, alloc);

		ComponentObj.AddMember("AABB", AABBVal, alloc);
		componentsArray.PushBack(ComponentObj, alloc);
	}

	if (g_coordinator->HasComponent<LayerTag>(entity))
	{
		Value ComponentObj(rapidjson::kObjectType);
		Value LayerTagVal;

		auto &layerTag = g_coordinator->GetComponent<LayerTag>(entity);
		layerTag.Serialize(LayerTagVal, alloc);

		ComponentObj.AddMember("LayerTag", LayerTagVal, alloc);
		componentsArray.PushBack(ComponentObj, alloc);
	}

	if (g_coordinator->HasComponent<AudioSource>(entity))
	{
		Value ComponentObj(rapidjson::kObjectType);
		Value AudioSourceVal;

		auto &audioSrc = g_coordinator->GetComponent<AudioSource>(entity);
		audioSrc.Serialize(AudioSourceVal, alloc);

		ComponentObj.AddMember("audiosource", AudioSourceVal, alloc);
		componentsArray.PushBack(ComponentObj, alloc);
	}

	if (g_coordinator->HasComponent<TextComponent>(entity))
	{
		Value ComponentObj(rapidjson::kObjectType);
		Value TextVal;

		auto &textComp = g_coordinator->GetComponent<TextComponent>(entity);
		textComp.Serialize(TextVal, alloc);

		ComponentObj.AddMember("TextComponent", TextVal, alloc);
		componentsArray.PushBack(ComponentObj, alloc);
	}

	if (g_coordinator->HasComponent<Parent>(entity))
	{
		Value ComponentObj(rapidjson::kObjectType);
		Value ParentVal;

		auto& parent = g_coordinator->GetComponent<Parent>(entity);
		parent.Serialize(ParentVal, alloc);

		ComponentObj.AddMember("Parent", ParentVal, alloc);
		componentsArray.PushBack(ComponentObj, alloc);
	}

	if (g_coordinator->HasComponent<Children>(entity))
	{
		Value ComponentObj(rapidjson::kObjectType);
		Value ChildrenVal;

		auto& children = g_coordinator->GetComponent<Children>(entity);
		children.Serialize(ChildrenVal, alloc);

		ComponentObj.AddMember("Children", ChildrenVal, alloc);
		componentsArray.PushBack(ComponentObj, alloc);
	}

	// ADD AS ONE BIG ARRAY WITH ALL THE COMPONENTS AVAILABLE ABOVE! >:]
	d.AddMember("components", componentsArray, alloc);

	StringBuffer buffer;
	PrettyWriter<StringBuffer> writer(buffer); // make it nice
	d.Accept(writer);
	namespace fs = std::filesystem;
	fs::create_directories(path.parent_path()); // make sure u save in Json and not build

	std::ofstream ofs{path};

	if (!ofs.is_open())
	{
		std::cerr << "Error: Cannot open file for writing: " << path << "\n";
		return false;
	}

	ofs << buffer.GetString();
	if (!ofs.good())
	{
		std::cerr << "Error: Failed to write JSON to file: " << path << "\n";
		return false;
	}
	return true;
}
