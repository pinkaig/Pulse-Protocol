/******************************************************************************/
/**
 * @file        HierarchyHelpers.h
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En (primary) - 90%
 * @author      Ban Kai Wei Benjamin (secondary) - 10%
 * @brief       Helper functions for managing entity parent-child relationships.
 *              Provides utilities for setting parents, getting children,
 *              and checking hierarchy structure.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#pragma once
#include "pch/pch_temp.h"
#include "CoreEngine/ECS/Coordinator.h"
#include "CoreEngine/ECS/Types.h"
#include "Components/ParentChild.h"
#include "Graphics/Transform.h"

namespace HierarchyHelpers
{
    // Invalid entity constant
    constexpr Entity INVALID = static_cast<Entity>(-1);

    // =========================================================================
    // SET PARENT
    // Sets the parent of a child entity, updating both Parent and Children components
    // =========================================================================
    inline void SetParent(Entity child, Entity newParent)
    {
        auto* g = Coordinator::GetInstance();
        if (!g) return;

        // Don't parent to self
        if (child == newParent) return;

        // ─────────────────────────────────────────────────────────────────────
        // STEP 1: Remove from OLD parent's children list
        // ─────────────────────────────────────────────────────────────────────
        if (g->HasComponent<Parent>(child))
        {
            Entity oldParent = g->GetComponent<Parent>(child).parent;
            if (oldParent != INVALID && g->HasComponent<Children>(oldParent))
            {
                auto& oldChildren = g->GetComponent<Children>(oldParent).children;
                oldChildren.erase(
                    std::remove(oldChildren.begin(), oldChildren.end(), child),
                    oldChildren.end()
                );

                // Clean up empty Children component
                if (oldChildren.empty())
                {
                    g->RemoveComponent<Children>(oldParent);
                }
            }
        }

        // ─────────────────────────────────────────────────────────────────────
        // STEP 2: Set NEW parent
        // ─────────────────────────────────────────────────────────────────────
        if (newParent == INVALID)
        {
            // Unparent - remove Parent component
            if (g->HasComponent<Parent>(child))
            {
                g->RemoveComponent<Parent>(child);
            }
            std::cout << "[Hierarchy] Entity " << child << " unparented" << std::endl;
        }
        else
        {
            // Add or update Parent component on child
            if (!g->HasComponent<Parent>(child))
            {
                g->AddComponent<Parent>(child, Parent{});
            }
            g->GetComponent<Parent>(child).parent = newParent;

            // Add child to new parent's Children list
            if (!g->HasComponent<Children>(newParent))
            {
                g->AddComponent<Children>(newParent, Children{});
            }

            auto& newChildren = g->GetComponent<Children>(newParent).children;
            if (std::find(newChildren.begin(), newChildren.end(), child) == newChildren.end())
            {
                newChildren.push_back(child);
            }

            std::cout << "[Hierarchy] Entity " << child << " parented to " << newParent << std::endl;
        }
    }

    // =========================================================================
    // HAS PARENT
    // Check if entity has a parent
    // =========================================================================
    inline bool HasParent(Entity entity)
    {
        auto* g = Coordinator::GetInstance();
        if (!g) return false;

        if (!g->HasComponent<Parent>(entity)) return false;
        return g->GetComponent<Parent>(entity).parent != INVALID;
    }

    // =========================================================================
    // GET PARENT
    // Returns parent entity or INVALID if none
    // =========================================================================
    inline Entity GetParent(Entity entity)
    {
        auto* g = Coordinator::GetInstance();
        if (!g) return INVALID;

        if (!g->HasComponent<Parent>(entity))
            return INVALID;

        return g->GetComponent<Parent>(entity).parent;
    }

    // =========================================================================
    // GET CHILDREN
    // Returns list of child entities (empty if none)
    // =========================================================================
    inline std::vector<Entity> GetChildren(Entity entity)
    {
        auto* g = Coordinator::GetInstance();
        if (!g) return {};

        if (!g->HasComponent<Children>(entity))
            return {};

        return g->GetComponent<Children>(entity).children;
    }

    // =========================================================================
    // HAS CHILDREN
    // Check if entity has any children
    // =========================================================================
    inline bool HasChildren(Entity entity)
    {
        auto* g = Coordinator::GetInstance();
        if (!g) return false;

        if (!g->HasComponent<Children>(entity)) return false;
        return !g->GetComponent<Children>(entity).children.empty();
    }

    // =========================================================================
    // IS ROOT
    // Check if entity is a root (no parent)
    // =========================================================================
    inline bool IsRoot(Entity entity)
    {
        return !HasParent(entity);
    }

    // =========================================================================
    // IS DESCENDANT OF
    // Check if 'entity' is a descendant of 'potentialAncestor'
    // Used to prevent circular parenting
    // =========================================================================
    inline bool IsDescendantOf(Entity entity, Entity potentialAncestor)
    {
        auto* g = Coordinator::GetInstance();
        if (!g) return false;

        Entity current = entity;
        int maxDepth = 100;  // Prevent infinite loops

        while (current != INVALID && maxDepth-- > 0)
        {
            if (current == potentialAncestor)
                return true;

            current = GetParent(current);
        }

        return false;
    }

    // =========================================================================
    // CAN SET PARENT
    // Validate if parenting operation is allowed (prevents circular refs)
    // =========================================================================
    inline bool CanSetParent(Entity child, Entity newParent)
    {
        if (child == newParent) return false;
        if (newParent == INVALID) return true;  // Unparenting is always allowed

        // Check if newParent is a descendant of child (would create cycle)
        return !IsDescendantOf(newParent, child);
    }

    // =========================================================================
    // MOVE ENTITY AND CHILDREN
    // Move an entity and all its children by a delta
    // =========================================================================
    inline void MoveEntityAndChildren(Entity entity, float deltaX, float deltaY)
    {
        auto* g = Coordinator::GetInstance();
        if (!g || !g->HasComponent<Framework::Transform>(entity)) return;

        // Move this entity
        auto& tr = g->GetComponent<Framework::Transform>(entity);
        tr.Pos.x += deltaX;
        tr.Pos.y += deltaY;

        // Move children recursively
        if (g->HasComponent<Children>(entity))
        {
            for (Entity child : g->GetComponent<Children>(entity).children)
            {
                MoveEntityAndChildren(child, deltaX, deltaY);
            }
        }
    }

    // =========================================================================
    // UNPARENT ALL CHILDREN
    // Remove all children from a parent (but don't delete them)
    // =========================================================================
    inline void UnparentAllChildren(Entity parent)
    {
        auto children = GetChildren(parent);
        for (Entity child : children)
        {
            SetParent(child, INVALID);
        }
    }

    // =========================================================================
    // DELETE ENTITY AND CHILDREN
    // Recursively delete an entity and all its children
    // Call this before Factory::DeleteEntity to clean up hierarchy
    // =========================================================================
    inline void PrepareDeleteWithChildren(Entity entity, std::vector<Entity>& outEntitiesToDelete)
    {
        auto* g = Coordinator::GetInstance();
        if (!g) return;

        // First, collect all children recursively
        if (g->HasComponent<Children>(entity))
        {
            for (Entity child : g->GetComponent<Children>(entity).children)
            {
                PrepareDeleteWithChildren(child, outEntitiesToDelete);
            }
        }

        // Add this entity to delete list
        outEntitiesToDelete.push_back(entity);

        // Remove from parent's children list
        if (g->HasComponent<Parent>(entity))
        {
            Entity parent = g->GetComponent<Parent>(entity).parent;
            if (parent != INVALID && g->HasComponent<Children>(parent))
            {
                auto& siblings = g->GetComponent<Children>(parent).children;
                siblings.erase(
                    std::remove(siblings.begin(), siblings.end(), entity),
                    siblings.end()
                );
            }
        }
    }

    // =========================================================================
    // GET HIERARCHY DEPTH
    // Returns how deep an entity is in the hierarchy (0 = root)
    // =========================================================================
    inline int GetHierarchyDepth(Entity entity)
    {
        int depth = 0;
        Entity current = entity;
        int maxDepth = 100;

        while (HasParent(current) && maxDepth-- > 0)
        {
            current = GetParent(current);
            depth++;
        }

        return depth;
    }

    // =========================================================================
    // Collec tSubtree Entities
    // Collect all the children and parent recursively
    // parents will be added to the back of container
    // =========================================================================
    inline void CollectSubtreeEntities(Entity entity, std::vector<Entity>& outEntities)
    {
        auto* g = Coordinator::GetInstance();
        if (!g) return; //early return

        if (g->HasComponent<Children>(entity))
        {
            for (Entity child : g->GetComponent<Children>(entity).children)
            {
                CollectSubtreeEntities(child, outEntities);
            }
        }

        // children first, parent last
        outEntities.push_back(entity);
    }
}