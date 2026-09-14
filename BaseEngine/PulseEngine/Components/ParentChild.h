/******************************************************************************/
/**
 * @file        ParentChild.h
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En
 * @brief       Parent-Child hierarchy components for entity grouping.
 *              Allows entities to be organized in a tree structure where
 *              moving a parent also moves all children.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#pragma once
#include "pch/pch_temp.h"
#include "CoreEngine/ECS/Types.h"

// =============================================================================
// PARENT COMPONENT
// Stores which entity is this entity's parent
// =============================================================================
struct Parent
{
    Entity parent = static_cast<Entity>(-1);  // INVALID_ENTITY means no parent

    void Serialize(rapidjson::Value& out, rapidjson::Document::AllocatorType& alloc) const
    {
        out.SetObject();
        out.AddMember("parent", static_cast<int>(parent), alloc);
    }

    void Deserialize(const rapidjson::Value& in)
    {
        if (!in.IsObject()) return;
        if (in.HasMember("parent") && in["parent"].IsInt())
            parent = static_cast<Entity>(in["parent"].GetInt());
    }
};

// =============================================================================
// CHILDREN COMPONENT
// Stores list of child entities
// =============================================================================
struct Children
{
    std::vector<Entity> children;

    void Serialize(rapidjson::Value& out, rapidjson::Document::AllocatorType& alloc) const
    {
        using namespace rapidjson;
        out.SetObject();

        Value childArray(kArrayType);
        for (Entity child : children)
        {
            childArray.PushBack(static_cast<int>(child), alloc);
        }
        out.AddMember("children", childArray, alloc);
    }

    void Deserialize(const rapidjson::Value& in)
    {
        if (!in.IsObject()) return;
        children.clear();

        if (in.HasMember("children") && in["children"].IsArray())
        {
            const auto& arr = in["children"];
            for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
            {
                if (arr[i].IsInt())
                    children.push_back(static_cast<Entity>(arr[i].GetInt()));
            }
        }
    }
};