/******************************************************************************/
/**
 * @file        Assets.h
 * @project     Pulse Protocol
 * @author      Reginald Lew Yee Ren (primary)- 50%
 * @author      Ban Kai Wei Benjamin (secondary) - 50%
 * @brief       Core asset data structures used by the ECS system for collision entity
 * 
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/

#pragma once

//#include <pch/pch.h>

// -----------------------------------------------------------------------------
// NPC Structure
// -----------------------------------------------------------------------------
// Represents a non-playable character (hero or monster) in the ECS system.
// Includes positional data, dimensions, health, and sword attack state.
// -----------------------------------------------------------------------------
struct NPC
{
    // Vector2 pos;          // World position of the NPC (center point)
    float width;  // Width of NPC hitbox
    float height; // Height of NPC hitbox

    // Sword parameters
    float swordWidth;    // Width of the sword hitbox
    float swordHeight;   // Height of the sword hitbox
    float swingProgress; // Current swing progress (0.0 - 1.0 normalized)
    bool isSwinging;     // True if NPC is currently swinging its sword

    // Health and status
    int hp;       // Health points (e.g., Hero = 10 HP, Monster = 1 HP)
    bool isAlive; // True if NPC is alive, false if defeated

    void Serialize(rapidjson::Value &out, rapidjson::Document::AllocatorType &alloc) const
    {
        out.SetObject();
        out.AddMember("width", width, alloc);
        out.AddMember("height", height, alloc);
        out.AddMember("swordWidth", swordWidth, alloc);
        out.AddMember("swordHeight", swordHeight, alloc);
        out.AddMember("swingProgress", swingProgress, alloc);
        out.AddMember("isSwinging", isSwinging, alloc);
        out.AddMember("hp", hp, alloc);
        out.AddMember("isAlive", isAlive, alloc);
    }
    void Deserialize(const rapidjson::Value &in)
    {
        if (!in.IsObject())
            throw std::runtime_error("Invalid JSON for NPC");
        if (in.HasMember("width"))
            width = in["width"].GetFloat();
        if (in.HasMember("height"))
            height = in["height"].GetFloat();
        if (in.HasMember("swordWidth"))
            swordWidth = in["swordWidth"].GetFloat();
        if (in.HasMember("swordHeight"))
            swordHeight = in["swordHeight"].GetFloat();
        if (in.HasMember("swingProgress"))
            swingProgress = in["swingProgress"].GetFloat();
        if (in.HasMember("isSwinging"))
            isSwinging = in["isSwinging"].GetBool();
        if (in.HasMember("hp"))
            hp = in["hp"].GetInt();
        if (in.HasMember("isAlive"))
            isAlive = in["isAlive"].GetBool();
    }
};

// -----------------------------------------------------------------------------
// AABB Structure
// -----------------------------------------------------------------------------
// Axis-Aligned Bounding Box used for rectangular collision detection.
// -----------------------------------------------------------------------------
struct AABB
{
    Vector2 min;    ///< Local-space bottom-left
    Vector2 max;    ///< Local-space top-right

    void Serialize(rapidjson::Value& out, rapidjson::Document::AllocatorType& alloc) const
    {
        using namespace rapidjson;
        out.SetObject();

        Value minVal;
        Value maxVal;
        min.Serialize(minVal, alloc);
        max.Serialize(maxVal, alloc);

        out.AddMember("min", minVal, alloc);
        out.AddMember("max", maxVal, alloc);
    }

    void Deserialize(const rapidjson::Value& in)
    {
        if (!in.IsObject())
            throw std::runtime_error("Invalid JSON for AABB");

        if (in.HasMember("min"))
            min.Deserialize(in["min"]);

        if (in.HasMember("max"))
            max.Deserialize(in["max"]);
    }

    bool operator==(AABB const& other) const
    {
        return min == other.min && max == other.max;
    }

    bool operator!=(AABB const& other) const
    {
        return !(*this == other);
    }
};
