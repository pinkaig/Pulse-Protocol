/******************************************************************************/
/**
 * @file        Health.h
 * @project     Pulse Protocol
 * @author		Reginald Lew Yee Ren (primary) - 90%
 * @author      Ban Kai Wei Benjamin (secondary) - 10%
 * @brief		Health component used to track an entity's vitality, damage output,
 *              and alive state. This component contains no graphical or collision
 *              data — it is purely for gameplay logic.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once
#include <pch/pch_temp.h>

 //for .NET
#include "CoreEngine/Core/ImportExport.h" // for DLL_API
#pragma warning(push)
#pragma warning(disable: 4251)

 /**
  * @struct Health
  * @brief  Defines hit points, damage potential, and alive state of an entity.
  */
struct DLL_API Health
{
    int hp;        ///< Current hit points of the entity
    int damage;    ///< Amount of damage this entity can deal to others
    bool isAlive;  ///< Flag indicating whether this entity is still active

    /**
     * @brief Default constructor initializing health, damage, and alive state.
     * @param hpVal    Initial hit points (default 10)
     * @param dmgVal   Damage dealt to other entities (default 1)
     * @param alive    Initial alive state (default true)
     */
    Health(int hpVal = 10, int dmgVal = 1, bool alive = true)
        : hp(hpVal), damage(dmgVal), isAlive(alive) {}

    void Serialize(rapidjson::Value& out, rapidjson::Document::AllocatorType& alloc) const
    {
        using namespace rapidjson;
        out.SetObject();

        out.AddMember("hp", hp, alloc);
        out.AddMember("damage", damage, alloc);
        out.AddMember("isAlive", isAlive, alloc);
    }

    void Deserialize(const rapidjson::Value& in)
    {
        if (!in.IsObject()) throw std::runtime_error("Invalid JSON for LogicComponent");
        if (in.HasMember("hp")) hp = in["hp"].GetFloat();
        if (in.HasMember("damage")) damage = in["damage"].GetFloat();
        if (in.HasMember("isAlive")) isAlive = in["isAlive"].GetBool();

    }

    bool operator==(Health const& other) const {
        return
            hp == other.hp &&
            damage == other.damage &&
            isAlive == other.isAlive;
    }

    bool operator!=(Health const& other) const {
        return !(*this == other);
    }
};
