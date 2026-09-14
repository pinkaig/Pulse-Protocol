/******************************************************************************/
/**
 * @file        PrefabManager.h
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En
 * @brief       Declares the PrefabManager class for managing reusable entity templates.
 *              - Added CanSaveAsPrefab() - checks if entity can be saved
 *              - Added GetBlockReason() - returns reason why entity cannot be saved
 *              - Blocks '_' prefix entities (engine-managed), allows game text
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#pragma once
#include "pch/pch_temp.h"
#include "CoreEngine/ECS/Types.h"
#include "../PulseEngine/ObjectAllocator/ObjectAllocator.h"
//for .NET
#include "CoreEngine/Core/ImportExport.h" // for DLL_API
// stupid warning just saying to make sure we are using same compilier type and runtime MDd. WHICH WE ARE SO WHY WARNING STILL??????????
#pragma warning(push)
#pragma warning(disable: 4251)

class Factory;

struct PrefabData {
    std::string prefabName;
    std::string filePath;
};

class DLL_API PrefabManager
{
public:
    PrefabManager();   // Constructor
    ~PrefabManager();  // Destructor

    // Check if entity can be saved as prefab (returns false for '_' prefix entities)
    bool CanSaveAsPrefab(Entity entity) const;

    // Get reason why entity cannot be saved (empty string if it can be saved)
    std::string GetBlockReason(Entity entity) const;

    // save entity as prefab (returns false if entity has '_' prefix name)
    bool SaveEntityAsPrefab(Entity entity, std::string const& prefabName);

    // load prefab and create entity
    std::optional<Entity> InstantiatePrefab(std::string const& prefabName);

    // delete prefab from disk
    bool DeletePrefab(std::string const& prefabName);

    // get all available prefabs for the editor
    std::vector<PrefabData> GetAvailablePrefabs();

    // get prefab path by name
    std::string GetPrefabPath(std::string const& prefabName);

    // updates all entities in CURRENT SCENE that were instantiated from this prefab
    void UpdateAllInstancesOfPrefab(std::string const& prefabName);

    // updates all entities in ALL SCENE FILES (on disk) that were instantiated from this prefab
    void UpdatePrefabInAllScenes(std::string const& prefabName);

private:
    std::filesystem::path assetRoot;
    std::string prefabDirectory;

    // updates a single entity from a prefab file (private helper)
    bool UpdateEntityFromPrefab(Entity entity, std::string const& prefabPath);

    // using Factory internally
   // Factory* factory;

    ObjectAllocator factoryOA;
    std::unique_ptr<Factory, OADeleter> factory;
};

#pragma warning(pop)