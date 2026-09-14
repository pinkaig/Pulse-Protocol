/******************************************************************************/
/**
 * @file        SnapShot.h
 * @project     Pulse Protocol
 * @author      Ban Kai Wei Benjamin
 * @brief       Saves an Entity and its full component state before deletion,
 *              so it can be restored on undo. Parent/child relationships are
 *              captured by value and remapped on restore so entity IDs stay
 *              opaque (no CreateEntityWithID dependency).
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/
#pragma once
#include "Editor.h"

#include "Graphics/Transform.h"
#include "Graphics/Renderable.h"
#include "Graphics/Animation.h"

#include "CoreEngine/Assets.h"                // NPC
#include "Components/AABB.h"                  // AABB
#include "Components/Health.h"                // Health
#include "Components/DisplayName.h"           // Name
#include "Graphics/Layer.h"                   // LayerTag
#include "Components/EntityType.h"            // NamingComponent
#include "GameLogic/GameLogic.h"              // LogicComponent
#include "Audio/AudioSource.h"                // AudioSource
#include "Graphics/Text.h"                    // TextComponent
#include "CoreEngine/Prefab/PrefabInstance.h" // PrefabInstance
#include "Components/ParentChild.h"           // Parent, Children

namespace PulseEditor
{
    // =========================================================================
    // EntitySnapshot
    // A plain value-copy of every component an entity might own.
    // Parent/Children store the OLD entity IDs at capture time.
    // Use the remap table in SceneSnapshot to translate them after restore.
    // =========================================================================
    struct EntitySnapshot
    {
        Entity originalEntity = INVALID_ENTITY;

        // --- presence flags ---
        bool hasName           = false;
        bool hasTransform      = false;
        bool hasRenderable     = false;
        bool hasAnimation      = false;
        bool hasLogicComponent = false;
        bool hasAudioSource = false;
        bool hasAABB           = false;
        bool hasHealth         = false;
        bool hasTextComponent  = false;

        bool hasLayerTag = false;
        bool hasPrefabInstance = false;
        bool hasParent         = false;
        bool hasChildren       = false;

        // --- component data ---
        Name                  name_snap{};
        Framework::Transform  transform_snap{};
        Framework::Renderable renderable_snap{};
        Framework::Animation  animation_snap{};
        LogicComponent        logiccomponent_snap{};
        AudioSource           audiosource_snap{};
        AABB                  aabb_snap{};
        Health                health_snap{};
        TextComponent         text_snap{};
 
        //Edotor stuff
        LayerTag              layertag_snap{};
        PrefabInstance        prefabinstance_snap{};
        Parent                parent_snap{};    // stores OLD parent entity id
        Children              children_snap{};  // stores OLD children entity ids
    };

    Entity CreateFromSnapshot(EntitySnapshot const& snap);
    EntitySnapshot MakeEntitySnapshot(Entity e);

} // namespace PulseEditor
