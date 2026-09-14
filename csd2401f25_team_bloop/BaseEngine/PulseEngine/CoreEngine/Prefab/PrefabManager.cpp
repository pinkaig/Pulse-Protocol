/******************************************************************************/
/**
 * @file        PrefabManager.cpp
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En (Primary) - 90%
 * @author      Reginald Lew Yee Ren (Secondary) - 10%
 * @brief       Implements the PrefabManager system for entity template management.
 *              - Blocks saving entities with '_' prefix (engine-managed)
 *              - Game text (TextComponent without '_' prefix) CAN be saved
 *              - Added TextComponent and AudioSource to UpdateEntityFromPrefab
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#include "PrefabManager.h"
#include "PrefabInstance.h"
#include "CoreEngine/Factory/Factory.h"
#include "Components/DisplayName.h"

// Component headers for UpdateEntityFromPrefab
#include "Graphics/Transform.h"
#include "Graphics/Renderable.h"
#include "Graphics/Animation.h"
#include "Graphics/Text.h"          
#include "GameLogic/GameLogic.h"
#include "Components/Health.h"
#include "Components/AABB.h"
// Note: AudioSource should already be available via pch or other includes

// RapidJSON for scene file updates
#include <rapidjson/ostreamwrapper.h>

PrefabManager::PrefabManager() 
	: factoryOA(sizeof(Factory),
	OAConfig{
	  false, 4, 2, false, 0, 0, alignof(std::max_align_t)
	})
{
	//factory = new Factory();
	factory = oa_make_unique<Factory>(factoryOA);

	assetRoot = CoreEngine::GetAssetRoot();
	prefabDirectory = (assetRoot / "Prefabs").string() + "/";

	// optional debug:
	std::cout << "[PrefabManager] assetRoot = " << assetRoot << "\n";
	std::cout << "[PrefabManager] prefabDirectory = " << prefabDirectory << "\n";
}

PrefabManager::~PrefabManager()
{
	//dont need dlt anymore
	//delete factory;
}

bool PrefabManager::SaveEntityAsPrefab(Entity entity, std::string const& prefabName)
{
	auto* coord = Coordinator::GetInstance();

	// =========================================================================
	// Block entities with '_' prefix (engine-managed like _FPS_Display)
	// Game text WITHOUT '_' prefix CAN be saved as prefab
	// =========================================================================
	if (coord->HasComponent<Name>(entity)) {
		auto& name = coord->GetComponent<Name>(entity).name;
		if (!name.empty() && name[0] == '_') {
			std::cerr << "[PrefabManager] Cannot save engine-managed entity as prefab: "
				<< name << std::endl;
			return false;
		}
	}

	namespace fs = std::filesystem;

	fs::path prefabPath = prefabDirectory + prefabName + ".json";

	//// DEBUG: Print the absolute path
	//std::cout << "[PrefabManager] Saving prefab to: " << fs::absolute(prefabPath) << std::endl;

	// if directory does not exist, create it
	fs::create_directories(fs::path(prefabDirectory));

	// use Factory's existing save single entity func
	bool success = factory->SaveEntityToJson(entity, prefabPath);

	if (success) {
		// AUTO-LINK: If this entity doesn't have PrefabInstance component, add it
		if (!coord->HasComponent<PrefabInstance>(entity)) {
			PrefabInstance prefabInstance;
			prefabInstance.prefabName = prefabName;
			prefabInstance.isLinked = true;
			coord->AddComponent<PrefabInstance>(entity, prefabInstance);

			std::cout << "[PrefabManager] Auto-linked entity " << entity
				<< " to prefab '" << prefabName << "'" << std::endl;
		}
	}

	return success;
}

bool PrefabManager::CanSaveAsPrefab(Entity entity) const
{
	auto* coord = Coordinator::GetInstance();

	// Cannot save if entity name starts with '_' (engine-managed)
	if (coord->HasComponent<Name>(entity)) {
		auto& name = coord->GetComponent<Name>(entity).name;
		if (!name.empty() && name[0] == '_') {
			return false;
		}
	}

	return true;
}

std::string PrefabManager::GetBlockReason(Entity entity) const
{
	auto* coord = Coordinator::GetInstance();

	if (coord->HasComponent<Name>(entity)) {
		auto& name = coord->GetComponent<Name>(entity).name;
		if (!name.empty() && name[0] == '_') {
			return "Engine-managed entities (names starting with '_') cannot be saved as prefabs.";
		}
	}

	return "";
}

std::optional<Entity> PrefabManager::InstantiatePrefab(std::string const& prefabName)
{
	// strip .json extension if present
	std::string cleanName = prefabName;
	if (cleanName.ends_with(".json")) {
		cleanName = cleanName.substr(0, cleanName.length() - 5);
	}

	std::string prefabPath = GetPrefabPath(cleanName);

	// if the prefab path is wrong/does not exist
	if (!std::filesystem::exists(prefabPath)) {
		std::cerr << "[PrefabManager] Prefab not found: " << prefabName << std::endl;
		return std::nullopt;
	}

	auto result = factory->CreateEntityFromJson(prefabPath);

	if (result.has_value()) {
		// set entity name without the .json extension
		Entity entity = result.value();
		auto* coord = Coordinator::GetInstance();

		// track that this entity came from this prefab
		PrefabInstance prefabInstance;
		prefabInstance.prefabName = cleanName;
		prefabInstance.isLinked = true;
		coord->AddComponent<PrefabInstance>(entity, prefabInstance);

		// if entity has name component alr then just set it to that name in the JSON file
		if (coord->HasComponent<Name>(entity)) {
			coord->GetComponent<Name>(entity).name = cleanName;
		}
		else {
			// else add Name component if it doesn't exist
			Name nm;
			nm.name = cleanName;
			coord->AddComponent<Name>(entity, nm);
		}
	}
	else {
		std::cerr << "[PrefabManager] Failed to create entity from: " << prefabPath << std::endl;
	}

	return result;
}

bool PrefabManager::DeletePrefab(std::string const& prefabName) {
	namespace fs = std::filesystem;

	// strip .json extension if present
	std::string cleanName = prefabName;
	if (cleanName.ends_with(".json")) {
		cleanName = cleanName.substr(0, cleanName.length() - 5);
	}

	fs::path prefabPath = GetPrefabPath(cleanName);

	// check if file exists
	if (!fs::exists(prefabPath)) {
		std::cerr << "[PrefabManager] Cannot delete - prefab not found: " << cleanName << std::endl;
		return false;
	}

	try {
		fs::remove(prefabPath);
		std::cout << "[PrefabManager] Successfully deleted prefab: " << cleanName << std::endl;
		return true;
	}
	catch (const fs::filesystem_error& e) {
		std::cerr << "[PrefabManager] Failed to delete prefab: " << e.what() << std::endl;
		return false;
	}
}


std::vector<PrefabData> PrefabManager::GetAvailablePrefabs()
{
	std::vector<PrefabData> prefabs;

	// automatically create a directory for prefab, if it does not exist
	if (!std::filesystem::exists(prefabDirectory)) {
		std::filesystem::create_directories(prefabDirectory);
		return prefabs;
	}

	// this loops through every file/folder in the directory
	for (auto const& entry : std::filesystem::directory_iterator(prefabDirectory)) {
		if (entry.path().extension() == ".json") {

			// create a struct to hold the data of the prefab
			PrefabData data;

			// get filename without the .json extension (etc. "EnemyPrefab.json" -> "EnemyPrefab")
			data.prefabName = entry.path().stem().string();

			// this gets the full file path
			data.filePath = entry.path().string();

			prefabs.push_back(data);
		}
	}

	return prefabs;
}

// this is just to get the path to the specific prefab
std::string PrefabManager::GetPrefabPath(std::string const& prefabName)
{
	return prefabDirectory + prefabName + ".json";
}

void PrefabManager::UpdateAllInstancesOfPrefab(std::string const& prefabName)
{
	auto* coord = Coordinator::GetInstance();

	// Collect all entities that have PrefabInstance component and match this prefab
	std::vector<Entity> prefabInstances;
	for (Entity entity = 0; entity < MaxEntity; ++entity) {
		if (coord->HasComponent<PrefabInstance>(entity)) {
			auto& prefabInstance = coord->GetComponent<PrefabInstance>(entity);

			// If this entity was created from this prefab and is still linked
			if (prefabInstance.prefabName == prefabName && prefabInstance.isLinked) {
				prefabInstances.push_back(entity);
			}
		}
	}

	// Now update all collected instances
	int updateCount = 0;
	for (Entity entity : prefabInstances) {
		std::string prefabPath = GetPrefabPath(prefabName);

		// Update this entity's components from the prefab file
		if (UpdateEntityFromPrefab(entity, prefabPath)) {
			updateCount++;
			std::cout << "[PrefabManager] Updated entity " << entity
				<< " from prefab: " << prefabName << std::endl;
		}
		else {
			std::cerr << "[PrefabManager] Failed to update entity " << entity << std::endl;
		}
	}

	std::cout << "[PrefabManager] Updated " << updateCount << " instances of prefab: "
		<< prefabName << std::endl;
}

bool PrefabManager::UpdateEntityFromPrefab(Entity entity, std::string const& prefabPath)
{
	auto* coord = Coordinator::GetInstance();

	std::ifstream ifs(prefabPath);
	if (!ifs.is_open()) {
		std::cerr << "[PrefabManager] Cannot open prefab file: " << prefabPath << "\n";
		return false;
	}

	rapidjson::IStreamWrapper isw(ifs);
	rapidjson::Document d;
	d.ParseStream(isw);

	if (!(d.HasMember("components") && d["components"].IsArray())) {
		std::cerr << "[PrefabManager] Invalid prefab format!" << std::endl;
		return false;
	}

	const auto& components = d["components"];

	// =========================================================================
	// Remove old components EXCEPT Transform, PrefabInstance, Name, LayerTag
	// =========================================================================

	if (coord->HasComponent<Framework::Renderable>(entity))
		coord->RemoveComponent<Framework::Renderable>(entity);
	if (coord->HasComponent<Framework::Animation>(entity))
		coord->RemoveComponent<Framework::Animation>(entity);
	if (coord->HasComponent<LogicComponent>(entity))
		coord->RemoveComponent<LogicComponent>(entity);
	if (coord->HasComponent<Health>(entity))
		coord->RemoveComponent<Health>(entity);
	if (coord->HasComponent<AABB>(entity))
		coord->RemoveComponent<AABB>(entity);
	if (coord->HasComponent<TextComponent>(entity))
		coord->RemoveComponent<TextComponent>(entity);
	if (coord->HasComponent<AudioSource>(entity))
		coord->RemoveComponent<AudioSource>(entity);

	// =========================================================================
	// Add updated components from prefab EXCEPT Transform
	// =========================================================================
	for (rapidjson::SizeType i = 0; i < components.Size(); ++i)
	{
		const auto& component = components[i];

		if (component.HasMember("render") && component["render"].IsObject()) {
			Framework::Renderable r;
			r.Deserialize(component["render"]);
			coord->AddComponent<Framework::Renderable>(entity, r);
		}

		if (component.HasMember("animation") && component["animation"].IsObject()) {
			Framework::Animation a;
			a.Deserialize(component["animation"]);
			coord->AddComponent<Framework::Animation>(entity, a);
		}

		if (component.HasMember("script") && component["script"].IsObject()) {
			LogicComponent l;
			l.Deserialize(component["script"]);
			coord->AddComponent<LogicComponent>(entity, l);
		}

		if (component.HasMember("Health") && component["Health"].IsObject()) {
			Health h;
			h.Deserialize(component["Health"]);
			coord->AddComponent<Health>(entity, h);
		}

		if (component.HasMember("AABB") && component["AABB"].IsObject()) {
			AABB aabb;
			aabb.Deserialize(component["AABB"]);
			coord->AddComponent<AABB>(entity, aabb);
		}

		// NEW: TextComponent
		if (component.HasMember("TextComponent") && component["TextComponent"].IsObject()) {
			TextComponent txt;
			txt.Deserialize(component["TextComponent"]);
			coord->AddComponent<TextComponent>(entity, txt);
		}

		// NEW: AudioSource
		if (component.HasMember("audiosource") && component["audiosource"].IsObject()) {
			AudioSource audio;
			audio.Deserialize(component["audiosource"]);
			coord->AddComponent<AudioSource>(entity, audio);
		}
	}

	return true;
}

void PrefabManager::UpdatePrefabInAllScenes(std::string const& prefabName)
{
    namespace fs = std::filesystem;

    // Scene directory - matches your SceneManager path
    //std::string sceneDirectory = "../../PulseEngine/JSON/";
	std::filesystem::path sceneDirectory = assetRoot.parent_path() / "JSON";

    if (!fs::exists(sceneDirectory)) {
        std::cerr << "[PrefabManager] Scene directory not found: " << sceneDirectory << std::endl;
        return;
    }

    // Load the prefab data first
    std::string prefabPath = GetPrefabPath(prefabName);
    std::ifstream prefabFile(prefabPath);
    if (!prefabFile.is_open()) {
        std::cerr << "[PrefabManager] Cannot open prefab file: " << prefabPath << std::endl;
        return;
    }

    rapidjson::IStreamWrapper prefabIsw(prefabFile);
    rapidjson::Document prefabDoc;
    prefabDoc.ParseStream(prefabIsw);
    prefabFile.close();

    if (!prefabDoc.HasMember("components") || !prefabDoc["components"].IsArray()) {
        std::cerr << "[PrefabManager] Invalid prefab format!" << std::endl;
        return;
    }

    int scenesUpdated = 0;
    int entitiesUpdated = 0;

    // Get current scene name to skip it (already updated in memory)
    auto* coord = Coordinator::GetInstance();
    auto sceneManager = coord->GetSystem<SceneManager>();
    std::string currentSceneName = sceneManager ? sceneManager->GrabCurrentScene() : "";

    // Iterate through all JSON files in scene directory
    for (auto const& entry : fs::directory_iterator(sceneDirectory)) {
        if (entry.path().extension() != ".json") {
            continue;
        }

        std::string scenePath = entry.path().string();
        std::string fileName = entry.path().stem().string();

        // Skip the current scene (already updated in memory)
        if (fileName == currentSceneName) {
            continue;
        }

        // Skip prefab files (they're in a different folder, but just in case)
        if (scenePath.find("Prefabs") != std::string::npos) {
            continue;
        }

        // Load scene JSON
        std::ifstream sceneFile(scenePath);
        if (!sceneFile.is_open()) {
            continue;
        }

        rapidjson::IStreamWrapper sceneIsw(sceneFile);
        rapidjson::Document sceneDoc;
        sceneDoc.ParseStream(sceneIsw);
        sceneFile.close();

        // Check if this is a scene file (has "scene" and "entities" members)
        if (!sceneDoc.HasMember("scene") || !sceneDoc.HasMember("entities") || !sceneDoc["entities"].IsArray()) {
            continue;
        }

        bool sceneModified = false;
        auto& entities = sceneDoc["entities"];

        // Check each entity in the scene
        for (rapidjson::SizeType i = 0; i < entities.Size(); ++i) {
            auto& entity = entities[i];

            if (!entity.HasMember("components") || !entity["components"].IsArray()) {
                continue;
            }

            auto& components = entity["components"];

            // Check if this entity has a PrefabInstance component linking to our prefab
            bool isLinkedToThisPrefab = false;

            for (rapidjson::SizeType j = 0; j < components.Size(); ++j) {
                auto& comp = components[j];

                // Note: Your SceneManager saves it as "prefabInstance" (lowercase p)
                if (comp.HasMember("prefabInstance") && comp["prefabInstance"].IsObject()) {
                    auto& prefabInst = comp["prefabInstance"];

                    if (prefabInst.HasMember("prefabName") &&
                        prefabInst["prefabName"].IsString() &&
                        std::string(prefabInst["prefabName"].GetString()) == prefabName &&
                        prefabInst.HasMember("isLinked") &&
                        prefabInst["isLinked"].IsBool() &&
                        prefabInst["isLinked"].GetBool())
                    {
                        isLinkedToThisPrefab = true;
                        break;
                    }
                }
            }

            if (!isLinkedToThisPrefab) {
                continue;
            }

            // =========================================================
            // Save components we want to PRESERVE (per-instance data)
            // =========================================================
            rapidjson::Value savedTransform(rapidjson::kObjectType);
            rapidjson::Value savedPrefabInstance(rapidjson::kObjectType);
            rapidjson::Value savedName(rapidjson::kObjectType);
            rapidjson::Value savedLayerTag(rapidjson::kObjectType);

            bool hasTransform = false;
            bool hasPrefabInstance = false;
            bool hasName = false;
            bool hasLayerTag = false;

            for (rapidjson::SizeType j = 0; j < components.Size(); ++j) {
                const auto& comp = components[j];

                if (comp.HasMember("transform") && !hasTransform) {
                    savedTransform.CopyFrom(comp["transform"], sceneDoc.GetAllocator());
                    hasTransform = true;
                }
                if (comp.HasMember("prefabInstance") && !hasPrefabInstance) {
                    savedPrefabInstance.CopyFrom(comp["prefabInstance"], sceneDoc.GetAllocator());
                    hasPrefabInstance = true;
                }
                if (comp.HasMember("name") && !hasName) {
                    savedName.CopyFrom(comp["name"], sceneDoc.GetAllocator());
                    hasName = true;
                }
                if (comp.HasMember("layerTag") && !hasLayerTag) {
                    savedLayerTag.CopyFrom(comp["layerTag"], sceneDoc.GetAllocator());
                    hasLayerTag = true;
                }
            }

            // =========================================================
            // Clear and rebuild components from prefab
            // =========================================================
            components.Clear();

            // Copy all components from prefab
            const auto& prefabComponents = prefabDoc["components"];
            for (rapidjson::SizeType j = 0; j < prefabComponents.Size(); ++j) {
                rapidjson::Value newComp(rapidjson::kObjectType);
                newComp.CopyFrom(prefabComponents[j], sceneDoc.GetAllocator());

                // Replace transform with saved instance transform
                if (newComp.HasMember("transform") && hasTransform) {
                    newComp.RemoveMember("transform");
                    rapidjson::Value transformCopy;
                    transformCopy.CopyFrom(savedTransform, sceneDoc.GetAllocator());
                    newComp.AddMember("transform", transformCopy, sceneDoc.GetAllocator());
                }

                // Replace name with saved instance name
                if (newComp.HasMember("name") && hasName) {
                    newComp.RemoveMember("name");
                    rapidjson::Value nameCopy;
                    nameCopy.CopyFrom(savedName, sceneDoc.GetAllocator());
                    newComp.AddMember("name", nameCopy, sceneDoc.GetAllocator());
                }

                components.PushBack(newComp, sceneDoc.GetAllocator());
            }

            // Re-add preserved components if they weren't in the prefab
            if (hasTransform) {
                // Check if transform was already added
                bool foundTransform = false;
                for (rapidjson::SizeType j = 0; j < components.Size(); ++j) {
                    if (components[j].HasMember("transform")) {
                        foundTransform = true;
                        break;
                    }
                }
                if (!foundTransform) {
                    rapidjson::Value transformComp(rapidjson::kObjectType);
                    rapidjson::Value transformCopy;
                    transformCopy.CopyFrom(savedTransform, sceneDoc.GetAllocator());
                    transformComp.AddMember("transform", transformCopy, sceneDoc.GetAllocator());
                    components.PushBack(transformComp, sceneDoc.GetAllocator());
                }
            }

            if (hasName) {
                // Check if name was already added
                bool foundName = false;
                for (rapidjson::SizeType j = 0; j < components.Size(); ++j) {
                    if (components[j].HasMember("name")) {
                        foundName = true;
                        break;
                    }
                }
                if (!foundName) {
                    rapidjson::Value nameComp(rapidjson::kObjectType);
                    rapidjson::Value nameCopy;
                    nameCopy.CopyFrom(savedName, sceneDoc.GetAllocator());
                    nameComp.AddMember("name", nameCopy, sceneDoc.GetAllocator());
                    components.PushBack(nameComp, sceneDoc.GetAllocator());
                }
            }

            if (hasLayerTag) {
                rapidjson::Value layerComp(rapidjson::kObjectType);
                rapidjson::Value layerCopy;
                layerCopy.CopyFrom(savedLayerTag, sceneDoc.GetAllocator());
                layerComp.AddMember("layerTag", layerCopy, sceneDoc.GetAllocator());
                components.PushBack(layerComp, sceneDoc.GetAllocator());
            }

            // Always re-add PrefabInstance component
            if (hasPrefabInstance) {
                rapidjson::Value prefabInstComp(rapidjson::kObjectType);
                rapidjson::Value prefabCopy;
                prefabCopy.CopyFrom(savedPrefabInstance, sceneDoc.GetAllocator());
                prefabInstComp.AddMember("prefabInstance", prefabCopy, sceneDoc.GetAllocator());
                components.PushBack(prefabInstComp, sceneDoc.GetAllocator());
            }

            sceneModified = true;
            entitiesUpdated++;
        }

        // Save modified scene back to disk
        if (sceneModified) {
            std::ofstream outFile(scenePath);
            if (outFile.is_open()) {
                rapidjson::OStreamWrapper osw(outFile);
                rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
                sceneDoc.Accept(writer);
                outFile.close();

                scenesUpdated++;
                std::cout << "[PrefabManager] Updated scene file: " << fileName << std::endl;
            }
        }
    }

    std::cout << "[PrefabManager] Updated " << entitiesUpdated << " entities across "
        << scenesUpdated << " scene files for prefab: " << prefabName << std::endl;
}