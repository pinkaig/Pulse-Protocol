/******************************************************************************/
/**
* @file        Collision.cpp
* @project     Pulse Protocol
* @author      Reginald - 100%
* @brief	  
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#pragma once
#define generic generic_workaround

#include "Collision.h"
#include "CoreEngine/Core/ScriptingBridge.h"
#include "../CoreEngine/ECS/Coordinator.h"

// Native engine collision system + types (AABB, OBB, CollisionManager)
#include "../Physics/collision.h"

#undef generic

namespace ScriptAPI
{
    CollisionComponent::CollisionComponent(unsigned int ID) : entityID(ID) {}

    // --- local helpers ---
    static bool TryGetWorldAABB(unsigned int id, AABB& outWorld)
    {
        auto g = Coordinator::GetInstance();

        if (!g->HasComponent<Framework::Transform>(id) ||
            !g->HasComponent<AABB>(id))
            return false;

        auto& t = g->GetComponent<Framework::Transform>(id);
        auto& base = g->GetComponent<AABB>(id);

        CollisionManager cm;
        outWorld = cm.BuildAABBFromTransform(t, base);
        return true;
    }

    static bool TryGetWorldOBB(unsigned int id, OBB& outWorld)
    {
        auto g = Coordinator::GetInstance();

        if (!g->HasComponent<Framework::Transform>(id) ||
            !g->HasComponent<AABB>(id))
            return false;

        auto& t = g->GetComponent<Framework::Transform>(id);
        auto& base = g->GetComponent<AABB>(id);

        CollisionManager cm;
        outWorld = cm.BuildOBBFromTransform(t, base);
        return true;
    }

    // --- wrapper methods ---
    bool CollisionComponent::CheckAABB(int otherEntity)
    {
        AABB a{}, b{};
        if (!TryGetWorldAABB(entityID, a) ||
            !TryGetWorldAABB(static_cast<unsigned int>(otherEntity), b))
            return false;

        auto g = Coordinator::GetInstance();
        auto cmSystem = g->GetSystem<CollisionManager>();
        if (cmSystem && !cmSystem->BroadPhaseAllows(entityID, static_cast<unsigned int>(otherEntity)))
            return false;

        CollisionManager cm;
        return cm.CheckAABBCollision(a, b);
    }

    bool CollisionComponent::CheckEdgeContact(int otherEntity, float threshold)
    {
        AABB a{}, b{};
        if (!TryGetWorldAABB(entityID, a) ||
            !TryGetWorldAABB(static_cast<unsigned int>(otherEntity), b))
            return false;

        auto g = Coordinator::GetInstance();
        auto cmSystem = g->GetSystem<CollisionManager>();
        if (cmSystem && !cmSystem->BroadPhaseAllows(entityID, static_cast<unsigned int>(otherEntity)))
            return false;

        CollisionManager cm;
        return cm.CheckEdgeContactCollision(a, b, threshold);
    }

    bool CollisionComponent::CheckDistance(float selfRadius, int otherEntity, float otherRadius)
    {
        auto g = Coordinator::GetInstance();
        unsigned int otherID = static_cast<unsigned int>(otherEntity);

        if (!g->HasComponent<Framework::Transform>(entityID) ||
            !g->HasComponent<Framework::Transform>(otherID))
            return false;

        auto& ta = g->GetComponent<Framework::Transform>(entityID);
        auto& tb = g->GetComponent<Framework::Transform>(otherID);

        CollisionManager cm;
        return cm.CheckDistanceCollision(ta.Pos, selfRadius, tb.Pos, otherRadius);
    }
}