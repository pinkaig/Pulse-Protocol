/******************************************************************************/
/**
 * @file        PrefabInstance.h
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En (primary) - 90%
 * @author      Ban Kai Wei Benjamin (secondary) - 10%
 * @brief       Component that tracks which prefab an entity was instantiated from.
 *              Enables the prefab system to update all instances when a prefab
 *              is modified in the editor.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#pragma once
#include "pch/pch_temp.h"

/**
 * @brief Component that links an entity to its source prefab
 *
 * When an entity is instantiated from a prefab, this component is attached
 * to track which prefab it came from. This enables:
 * - Updating all instances when the prefab is edited
 * - Unlinking instances to make them independent
 * - Identifying which entities are prefab instances
 */
struct PrefabInstance {
    std::string prefabName;  // Name of the prefab this entity was created from (without .json)
    bool isLinked = true;    // If true, this instance will update when the prefab changes; else this instance is independent from the prefab

    //comaprison
    bool operator==(PrefabInstance const& other) const
    {
        return prefabName == other.prefabName &&
            isLinked == other.isLinked;
    }

    bool operator!=(PrefabInstance const& other) const
    {
        return !(*this == other);
    }

    // Serialization methods for saving/loading
    void Serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& alloc) const
    {
        obj.SetObject();

        rapidjson::Value nameVal;
        nameVal.SetString(prefabName.c_str(), static_cast<rapidjson::SizeType>(prefabName.length()), alloc);
        obj.AddMember("prefabName", nameVal, alloc);

        obj.AddMember("isLinked", isLinked, alloc);
    }

    void Deserialize(const rapidjson::Value& obj)
    {
        if (obj.HasMember("prefabName") && obj["prefabName"].IsString()) {
            prefabName = obj["prefabName"].GetString();
        }

        if (obj.HasMember("isLinked") && obj["isLinked"].IsBool()) {
            isLinked = obj["isLinked"].GetBool();
        }
    }
};