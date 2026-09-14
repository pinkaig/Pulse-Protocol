/******************************************************************************/
/**
* @file        Collision.h
* @project     Pulse Protocol
* @author      Reginald - 100%
* @brief       C++/CLI ScriptAPI wrapper for collision utilities.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#pragma once

namespace ScriptAPI
{
    // so scripts can do: GetCollision().CheckAABB(otherEntity);
    public value struct CollisionComponent
    {
    public:
        // AABB vs AABB using Transform + AABB on both entities
        bool CheckAABB(int otherEntity);

        // Edge contact check with tolerance threshold
        bool CheckEdgeContact(int otherEntity, float threshold);

        // Distance check using both Transform positions
        bool CheckDistance(float selfRadius, int otherEntity, float otherRadius);

    internal:
        CollisionComponent(unsigned int ID);

    private:
        unsigned int entityID;
    };
}