/******************************************************************************/
/**
 * @file        SnapShot.cpp
 * @project     Pulse Protocol
 * @author      Ban Kai Wei Benjamin
 * @brief       Implementation of entity snapshot / restore helpers.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/
#include "SnapShot.h"
#include "CoreEngine/Factory/Factory.h"
#include "HierarchyHelpers.h"
//#include "EditorTypes.h"

namespace PulseEditor
{

    //// =============================================================================
    //// CreateFromSnapshot
    //// adds the component if the flag says it should have it, recreates an entity
    //// =============================================================================  
    Entity CreateFromSnapshot(EntitySnapshot const& snap)
    {
        // Create new entity and apply snapshot data
        // Return new entity id

        auto* g = Coordinator::GetInstance();
        //Entity e = f.DefaultObj();// create a new entity default obj
        Entity e = g->CreateEntity();

        if (snap.hasName) {
            g->AddComponent<Name>(e, snap.name_snap);
        }
            
        if (snap.hasTransform)
            g->AddComponent<Framework::Transform>(e, snap.transform_snap);

        if (snap.hasRenderable)
            g->AddComponent<Framework::Renderable>(e, snap.renderable_snap);

        if (snap.hasAnimation)
            g->AddComponent<Framework::Animation>(e, snap.animation_snap);

        if (snap.hasLogicComponent)
            g->AddComponent<LogicComponent>(e, snap.logiccomponent_snap);

        if (snap.hasAudioSource)
            g->AddComponent<AudioSource>(e, snap.audiosource_snap);

        if (snap.hasAABB)
            g->AddComponent<AABB>(e, snap.aabb_snap);

        if (snap.hasHealth)
            g->AddComponent<Health>(e, snap.health_snap);

        if (snap.hasTextComponent)
            g->AddComponent<TextComponent>(e, snap.text_snap);

        if (snap.hasLayerTag)
            g->AddComponent<LayerTag>(e, snap.layertag_snap);

        if (snap.hasPrefabInstance)
            g->AddComponent<PrefabInstance>(e, snap.prefabinstance_snap);

        return e; // placeholder
    }
    //// =============================================================================
    //// MakeEntitySnapshot
    //// Captures all components that currently exist on entity e.
    //// And create a snapshot of the entity's component and child parent status
    //// =============================================================================  
    EntitySnapshot MakeEntitySnapshot(Entity e)
    {
        EntitySnapshot snap{};
            snap.originalEntity = e;

            auto* g = Coordinator::GetInstance();
            if (!g || e == INVALID_ENTITY)
                return snap;

            if (g->HasComponent<Framework::Transform>(e))
            {
                snap.hasTransform = true;
                snap.transform_snap = g->GetComponent<Framework::Transform>(e);
            }
            if (g->HasComponent<Framework::Renderable>(e))
            {
                snap.hasRenderable = true;
                snap.renderable_snap = g->GetComponent<Framework::Renderable>(e);
            }
            if (g->HasComponent<Framework::Animation>(e))
            {
                snap.hasAnimation = true;
                snap.animation_snap = g->GetComponent<Framework::Animation>(e);
            }
            if (g->HasComponent<AABB>(e))
            {
                snap.hasAABB = true;
                snap.aabb_snap = g->GetComponent<AABB>(e);
            }
            if (g->HasComponent<Health>(e))
            {
                snap.hasHealth = true;
                snap.health_snap = g->GetComponent<Health>(e);
            }
            if (g->HasComponent<Name>(e))
            {
                snap.hasName = true;
                snap.name_snap = g->GetComponent<Name>(e);
            }
            if (g->HasComponent<LogicComponent>(e))
            {
                snap.hasLogicComponent = true;
                snap.logiccomponent_snap = g->GetComponent<LogicComponent>(e);
            }
            if (g->HasComponent<LayerTag>(e))
            {
                snap.hasLayerTag = true;
                snap.layertag_snap = g->GetComponent<LayerTag>(e);
            }
            if (g->HasComponent<AudioSource>(e))
            {
                snap.hasAudioSource = true;
                snap.audiosource_snap = g->GetComponent<AudioSource>(e);
            }
            if (g->HasComponent<TextComponent>(e))
            {
                snap.hasTextComponent = true;
                snap.text_snap = g->GetComponent<TextComponent>(e);
            }
            if (g->HasComponent<PrefabInstance>(e))
            {
                snap.hasPrefabInstance = true;
                snap.prefabinstance_snap = g->GetComponent<PrefabInstance>(e);
            }
            if (g->HasComponent<Parent>(e))
            {
                snap.hasParent = true;
                snap.parent_snap = g->GetComponent<Parent>(e);
            }
            if (g->HasComponent<Children>(e))
            {
                snap.hasChildren = true;
                snap.children_snap = g->GetComponent<Children>(e);
            }

        return snap;
    }

} // namespace PulseEditor