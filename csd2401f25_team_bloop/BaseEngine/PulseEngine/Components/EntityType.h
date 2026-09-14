/******************************************************************************/
/**
 * @file        EntityType.h
 * @project     Pulse Protocol
 * @author		Chia Wei Xuan Rachael
 * @brief		Entity categorization component for tagging entities with semantic types
 *              (e.g., "player", "enemy", "button"). Provides runtime type identification
 *              for game logic, debugging, and editor display. Fully serializable for
 *              prefab and scene persistence.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/
#pragma once
#include "pch/pch_temp.h"

//Serialize
#pragma warning(push, 0)
#include <rapidjson/document.h>     // for Document, Value, AllocatorType
#include <rapidjson/prettywriter.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h> // for StringBuffer
#pragma warning(pop)

struct NamingComponent
{
    std::string Type;    // so like player, enenmy, button

    void Serialize(rapidjson::Value& out, rapidjson::Document::AllocatorType& alloc) const
    {
        //using namespace rapidjson;
        out.SetObject();
        out.AddMember("Type", rapidjson::Value(Type.c_str(), alloc), alloc);
    }

    void Deserialize(const rapidjson::Value& in)
    {
        if (!in.IsObject()) throw std::runtime_error("Invalid JSON for LogicComponent");
        if (in.HasMember("Type")) Type = in["Type"].GetString();
    }
};