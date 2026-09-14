/******************************************************************************/
/**
 * @file        UndoHelper.h
 * @project     Pulse Protocol
 * @author      Ban Kai Wei Benjamin
 * @brief       Functions that implement undo logic using undomanager,
 *              snapshot, hierarchyhelpers 
 *               and editortypes with engine's factory functions
 * 
 *              The stable handle exists so undo/redo can keep track of the
 *              same "conceptual" game object even if ECS ID changes over time
 *              Need it to prevent phantom entity bug
 *              conceptual objecct -> same logical object in editor
 *              live entity -> current ECS id it has in the Entity container
 * 
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once
#include <vector>
#include <algorithm>
#include "UndoManager.h"
#include <memory>
#include "CoreEngine/Factory/Factory.h" 
#include "SnapShot.h"
#include "EditorTypes.h"
#include "HierarchyHelpers.h"
#include <iostream>
#include <unordered_set>
#include "LayerManagerPanel.h"
//using Entity = std::uint32_t; lazy put header
//std::uint32_t = Entity  


namespace PulseEditor
{

    struct DeletedNode // for tree
    {
        Entity oldEntity = INVALID_ENTITY;
        EntitySnapshot snapshot{};
        int displayIndex = -1;
        std::shared_ptr<EntityHandle> handle;
        std::shared_ptr<EntityHandle> parentHandle;
    };

    //not needed for now
    //inline std::shared_ptr<EntityHandle> EnsureHandle(Entity e)
    //{
    //    if (e == INVALID_ENTITY) return nullptr;

    //    auto it = handleOf.find(e);
    //    if (it != handleOf.end() && it->second)
    //        return it->second;

    //    auto h = std::make_shared<EntityHandle>();
    //    h->id = e;
    //    handleOf[e] = h;
    //    return h;
    //}

    // Helper: remove entity from display order if present
    //find entity e and remove it from the vector of entities
    inline void RemoveFromDisplayOrder(std::vector<std::uint32_t>& order, std::uint32_t e)
    {
        auto it = std::find(order.begin(), order.end(), e);
        if (it != order.end())
            order.erase(it);
    }
    //
    // Helper: insert entity back into display order at index (clamped)
    /* index needed for order of when entity ID is added to
    * order = [10, 42, 77, 99] //entity IDs
          0   1   2   3   // indices
    */
    inline void InsertIntoDisplayOrder(std::vector<std::uint32_t>& order, std::uint32_t e, int index)
    {
        if (index < 0) index = 0;
        if (index > (int)order.size()) index = (int)order.size();
        order.insert(order.begin() + index, e);
    }

    // Helper: find index of entity in display order
    // -1 means not found (INVALID ENTITY), 
    inline int FindIndexInDisplayOrder(std::vector<std::uint32_t> const& order, std::uint32_t e)
    {
        auto it = std::find(order.begin(), order.end(), e);
        if (it == order.end()) return INVALID_ENTITY;
        return static_cast<int>(it - order.begin());
    }

 
    //inline void RebindHandle(std::unordered_map<Entity, std::shared_ptr<EntityHandle>>& hof,
    //    std::shared_ptr<EntityHandle> const& h,
    //    Entity oldId,
    //    Entity newId)
    //{
    //    if (!h) return;

    //    if (oldId != INVALID_ENTITY)
    //        hof.erase(oldId);

    //    if (newId != INVALID_ENTITY)
    //        hof[newId] = h;
    //}


/**
 * @brief Creates a default game object and records the action for undo/redo.
 *
 * Spawns a default entity immediately, adds it to the display order and
 * handle map, then pushes an undo/redo action so the creation can be
 * reversed or recreated later.
 *
 * @return Stable handle to the "conceptual" object for selection/tracking.
 */
    inline std::shared_ptr<EntityHandle>
        PushCreateDefault(UndoManager& undo, Factory& factory, std::vector<Entity>& displayOrder
        , std::unordered_map<Entity, std::shared_ptr<EntityHandle>>& hof)
    {
        // Shared stable handle for this object
        auto h = std::make_shared<EntityHandle>();
        h->id = INVALID_ENTITY;

        {
            Entity created = factory.DefaultObj();
            h->id = created;
            hof[created] = h;
            // Put into display list (optional)
            displayOrder.push_back(created);
        }

        // Build undo/redo lambdas that refer ONLY to stable data:
        // - factory reference
        // - displayOrder reference
        // - shared handle h (stable)
        UndoManager::Action a;

        a.undo = [h, &factory, &displayOrder, &hof]()
            {
                // Undo create = delete the currently live entity
                if (h->id == INVALID_ENTITY) return;

                Entity old = h->id;

                // Delete from ECS
                factory.DeleteEntity(old);
                
                // Remove from UI
                RemoveFromDisplayOrder(displayOrder, old);

                // unbind map
                hof.erase(old);
                // Mark as not alive
                h->id = INVALID_ENTITY;
            };

        a.redo = [h, &factory, &displayOrder, &hof]()
            {
                // Redo create = create a NEW entity (new id), then update the handle
                if (h->id != INVALID_ENTITY)
                {
                    // Already alive somehow; avoid double-create
                    return;
                }
                //create the object again
                Entity createdAGN = factory.DefaultObj();
                h->id = createdAGN;
                hof[createdAGN] = h;

                // Add back into UI list
                displayOrder.push_back(createdAGN);
            };

        undo.Push(std::move(a));
        return h; // return handle so caller can keep selection tracking
    }
    /******************************************************************************/
/// @brief Records a delete-entity action into the undo system.
///
///     Deletes the entity currently referenced by the stable handle immediately,
///     then pushes an undo/redo action pair into the UndoManager so the deletion
///     can be reversed or re-applied later
///     The stable handle is required because undo restore may create a new
///     ECS entity ID. The handle preserves the logical identity of the object
///     1. Validate the stable handle.
///     2. Capture entity snapshot and hierarchy index.
///     3. Delete the live entity immediately.
///     4. Push an action:
///        - undo => recreate from snapshot and restore hierarchy position
///        - redo => delete the recreated live entity again
/*****************************************************************************/   
    inline void PushDeleteEntity(UndoManager& undo,
            Factory& factory,
            std::vector<Entity>& displayOrder,
            std::shared_ptr<EntityHandle> h
        , std::unordered_map<Entity, std::shared_ptr<EntityHandle>>& hof)
    {
        if (!h || h->id == INVALID_ENTITY)
            return; // nothing to delete

        // Capture snapshot BEFORE deleting
        EntitySnapshot snap = MakeEntitySnapshot(h->id);

        // Capture UI position BEFORE removing
        int index = FindIndexInDisplayOrder(displayOrder, h->id);

        {
            Entity old = h->id;
            RemoveFromDisplayOrder(displayOrder, old);
            factory.DeleteEntity(old);
            hof.erase(old);
            h->id = INVALID_ENTITY; //assign to invalid
        }

        UndoManager::Action a;
        //the mutable keyword in a lambda expression allows the body of the lambda 
        // to modify variables that were captured by value
        a.undo = [h, snap, index, &factory, &displayOrder, &hof]() mutable
            {
                // Undo delete = recreate entity from snapshot
                if (h->id != INVALID_ENTITY)
                {
                    // Already alive; avoid double-spawn
                    return;
                }

                std::cout << "[CreateFromSnapshot] UNDOING..\n";

                Entity recreated = CreateFromSnapshot(snap);
                if (recreated == INVALID_ENTITY) {
                    std::cout << "[CreateFromSnapshot] recreated is invalid entitiy..\n";
                    return;
                }
              

                h->id = recreated;
                hof[recreated] = h;

                // Restore UI order at original index
                InsertIntoDisplayOrder(displayOrder, recreated, index);

                //if (index < 0 || index >(int)displayOrder.size())
                //    displayOrder.push_back(recreated);
                //else
                //    displayOrder.insert(displayOrder.begin() + index, recreated);
         
            };

        a.redo = [h, &factory, &displayOrder, &hof]()
            {
                // Redo delete = delete the currently live entity
                if (h->id == INVALID_ENTITY)
                    return;
                Entity old = h->id;
                RemoveFromDisplayOrder(displayOrder, old);
                factory.DeleteEntity(old);
                hof.erase(old);
                h->id = INVALID_ENTITY;
            };

        undo.Push(std::move(a));
    }
    /**
     * @brief Deletes a root entity and its entire subtree with undo/redo support.
     *
     * Captures snapshots, stable handles, hierarchy links, display order, and
     * selection state before deleting the subtree. Undo recreates the subtree and
     * restores parent-child relationships, UI order, and selection. Redo deletes
     * the subtree again.
     * 
     * Needs this for Parent-Child delete undo/redo
     * 
     *  Undo restores:
     *    - all deleted entities
     *    - parent-child hierarchy links
     *    - display order positions
     *    - stable handles
     *    - selection of the restored root
     *
     *    Redo deletes the restored subtree again and clears selection.
     * 
     */
    inline void PushDeleteSubtree(
        UndoManager& undo,
        Factory& factory,
        std::vector<Entity>& displayOrder,
        std::unordered_map<Entity, std::shared_ptr<EntityHandle>>& hof,
        Entity root,
        Entity& se,
        std::shared_ptr<EntityHandle>& sh)
    {
        if (root == INVALID_ENTITY) //early return
            return;

        std::vector<Entity> entitiesToDelete;
        HierarchyHelpers::CollectSubtreeEntities(root, entitiesToDelete);
        std::unordered_set<Entity> deletedSet(entitiesToDelete.begin(), entitiesToDelete.end());
        if (entitiesToDelete.empty())
            return;

        // Shared container holding all deleted subtree nodes.
        // Kept alive for both undo and redo lambdas.
        auto deleted = std::make_shared<std::vector<DeletedNode>>();
        deleted->reserve(entitiesToDelete.size());

        // Capture all info BEFORE deletion
        for (Entity e : entitiesToDelete)
        {
            //create node and snapshots and get disply index
            DeletedNode node;
            node.oldEntity = e;
            node.snapshot = MakeEntitySnapshot(e);
            node.displayIndex = FindIndexInDisplayOrder(displayOrder, e);

            auto itHandle = hof.find(e);
            if (itHandle != hof.end())
            {
                node.handle = itHandle->second;
            }
            else
            {
                node.handle = std::make_shared<EntityHandle>();
                node.handle->id = e;
                hof[e] = node.handle;
            }
            //e.g. delete child then delete parent then undo
            //Need this for child-parent
            if (node.snapshot.hasParent)
            {
                //Entity oldParent = node.snapshot.parent_snap.parent;
                //auto itParentHandle = hof.find(oldParent);
                //if (itParentHandle != hof.end())
                //{
                //    node.parentHandle = itParentHandle->second;
                //}

                //need this to fix child D, parent D then undo undo
                Entity oldParent = node.snapshot.parent_snap.parent;
                if (oldParent != INVALID_ENTITY)
                {
                    auto itParentHandle = hof.find(oldParent);
                    if (itParentHandle != hof.end())
                    {
                        node.parentHandle = itParentHandle->second;
                    }
                    else //
                    {
                        // Create a stable handle for the parent,
                        // even if the parent is not being deleted in this action.
                        // prevents bug, cDpD, undo x2
                        node.parentHandle = std::make_shared<EntityHandle>();
                        node.parentHandle->id = oldParent;
                        hof[oldParent] = node.parentHandle;
                    }
                }
            }

            deleted->push_back(node);
        }

        // Delete now
        for (Entity e : entitiesToDelete)
        {
            auto* g = Coordinator::GetInstance();
            if (g && g->HasComponent<Parent>(e))
            {
                Entity parent = g->GetComponent<Parent>(e).parent;

                // only detach from parent if parent is NOT also being deleted
                if (parent != INVALID_ENTITY && deletedSet.find(parent) == deletedSet.end())
                {
                    //use hierarchy helper set parent 
                    HierarchyHelpers::SetParent(e, HierarchyHelpers::INVALID);
                }
            }

            RemoveFromDisplayOrder(displayOrder, e);
            hof.erase(e);
            //delete the object
            factory.DeleteEntity(e);
        }

        for (auto& node : *deleted)
        {
            if (node.handle)
                node.handle->id = INVALID_ENTITY;
        }

        se = INVALID_ENTITY;
        //reset the shared container before executing
        // undo/redo action 
        sh.reset(); 

        UndoManager::Action a;

        a.undo = [deleted, &displayOrder, &hof, &se, &sh]() mutable
            {
                auto* g = Coordinator::GetInstance();
                if (!g) return;

                std::unordered_map<Entity, Entity> oldToNew;

                // Pass 1: recreate all entities
                for (auto& node : *deleted)
                {
                    Entity recreated = CreateFromSnapshot(node.snapshot);
                    if (recreated == INVALID_ENTITY) {
                        continue;
                    }//early return
                       

                    oldToNew[node.oldEntity] = recreated;
                    //asign the handles to recreated entity
                    if (node.handle) 
                    {
                        node.handle->id = recreated;
                        hof[recreated] = node.handle;
                    }
                }

                // Pass 2: clear recreated hierarchy data
                for (auto& node : *deleted)
                {
                    auto itNew = oldToNew.find(node.oldEntity);
                    if (itNew == oldToNew.end())
                        continue;

                    Entity newEntity = itNew->second;

                    //remove the parent and child component

                    if (g->HasComponent<Parent>(newEntity))
                        g->RemoveComponent<Parent>(newEntity);

                    if (g->HasComponent<Children>(newEntity))
                        g->RemoveComponent<Children>(newEntity);
                }

                // Pass 3: rebuild hierarchy using saved parent links
                for (auto& node : *deleted)
                {
                    auto itNewChild = oldToNew.find(node.oldEntity);
                    if (itNewChild == oldToNew.end())
                        continue;
                    Entity newChild = itNewChild->second;
                    if (node.snapshot.hasParent)
                    {
                        Entity oldParent = node.snapshot.parent_snap.parent;
                        Entity parentToUse = INVALID_ENTITY;

                        // Case 1: parent was deleted in the SAME undo action
                        auto itNewParent = oldToNew.find(oldParent);
                        if (itNewParent != oldToNew.end())
                        {
                            parentToUse = itNewParent->second;
                        }
                        // Case 2: parent was deleted/recreated in a DIFFERENT undo action
                        else if (node.parentHandle && node.parentHandle->id != INVALID_ENTITY)
                        {
                            parentToUse = node.parentHandle->id;
                        }
                        // Case 3: parent was never deleted and still uses same raw ID
                        // check if parent(old) still exists
                        else if (oldParent != INVALID_ENTITY)
                        {
                            //assume exist if contain this components
                            bool liveParentExists =
                                g->HasComponent<Parent>(oldParent) ||
                                g->HasComponent<Children>(oldParent) ||
                                g->HasComponent<Name>(oldParent) ||
                                g->HasComponent<LayerTag>(oldParent);

                            if (liveParentExists)
                                parentToUse = oldParent;
                        }

                        HierarchyHelpers::SetParent(newChild, parentToUse);
                    }
                    else
                    {
                        HierarchyHelpers::SetParent(newChild, INVALID_ENTITY);
                    }
                }

                // Pass 4: restore display order
                for (auto& node : *deleted)
                {
                    if (!node.handle || node.handle->id == INVALID_ENTITY)
                        continue;

                    InsertIntoDisplayOrder(displayOrder, node.handle->id, node.displayIndex);
                }

                // Pass 5: restore selection to root
                if (!deleted->empty())
                {
                    auto& rootNode = deleted->back(); // root is last
                    sh = rootNode.handle;
                    se = sh ? sh->id : INVALID_ENTITY;
                }
            };

        a.redo = [deleted, &displayOrder, &hof, &se, &sh, &factory]() mutable
            {
                // deleted vector order is children first, parent last
                // deleting in this same order
                for (auto& node : *deleted)
                {
                    // Skip nodes that are not currently live.
                    // in handle and marked as invalid
                    if (!node.handle || node.handle->id == INVALID_ENTITY)
                        continue;

                    Entity e = node.handle->id;
                    // Remove from editor UI, unbind handle map, and delete from ECS.
                    RemoveFromDisplayOrder(displayOrder, e);
                    hof.erase(e);
                    factory.DeleteEntity(e);
                    node.handle->id = INVALID_ENTITY;
                }
                // Clear current selection after subtree deletion.
                se = INVALID_ENTITY;
                sh.reset();
            };

        undo.Push(std::move(a));
    }

    /*
    Components
    Push Edits, could be template functions for majority
    but easier debug and safer to execute this way

    NOTE: Layer and Logic Components vastly differs from 
    typical components

    Regular components
     * Records a transform component edit for undo/redo.
     * Stores the previous and updated <COMPONENT_TYPE> states for the entity referenced
     * by the stable handle. Undo restores the old <COMPONENT_TYPE>, while redo reapplies
     * the new <COMPONENT_TYPE>.

     LayerTag component
    - Records a LayerTag edit for undo/redo.
     * Restores the old layer on undo and reapplies the new layer on redo.

    LayerRegistry edit
    - Records a global layer registry edit for undo/redo.
     * Restores the previous registry snapshot on undo and the updated snapshot on redo.
     * Also refreshes the graphics layer filter after each restore.

    Logic component
    - Records a LogicComponent edit for undo/redo.
     * Restores the old script list on undo and reapplies the new script list on redo.
     * Also syncs the runtime script bindings by removing old scripts first,
     * then re-adding the restored scripts after the component state is assigned.

    */

    inline void PushTransformEdit(UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        Framework::Transform const& before,
        Framework::Transform const& after)
    {
        // Record a transform change for undo/redo.

        // Debug: show the stable handle and current live entity id.
        //std::cout << "[UndoH] PushTransformEdit called\n";
        //std::cout << "[UndoH] h ptr = " << h.get()
        //    << " id = " << (h ? h->id : INVALID_ENTITY)
        //    << " INVALID_ENTITY = " << INVALID_ENTITY
        //    << "\n";

        if (!h || h->id == INVALID_ENTITY) return;
        //std::cout << "[UndoH] PushTransformEdit no early return\n";
        UndoManager::Action a;

        a.undo = [h, before]()
            {// Restore the previous transform state.
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<Framework::Transform>(h->id)) return;

                c->GetComponent<Framework::Transform>(h->id) = before;
                std::cout << "[UndoH] undo transform for entity id " << h->id 
                    << "[UndoH] transform values are " << before << "\n";
            };

        a.redo = [h, after]()
            {// Reapply the edited transform state.
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<Framework::Transform>(h->id)) return;

                c->GetComponent<Framework::Transform>(h->id) = after;
                //std::cout << "[UndoH] redo transform for entity id " << h->id
                 //   << "[UndoH] transform values are " << after << "\n";
            };
        // Store the action in undo history.
        undo.Push(std::move(a));
    }

    inline void PushRenderableEdit(UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        Framework::Renderable const& before,
        Framework::Renderable const& after)
    {
        if (!h || h->id == INVALID_ENTITY) return;

        UndoManager::Action a;

        a.undo = [h, before]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<Framework::Renderable>(h->id)) return;

                c->GetComponent<Framework::Renderable>(h->id) = before;
            };

        a.redo = [h, after]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<Framework::Renderable>(h->id)) return;

                c->GetComponent<Framework::Renderable>(h->id) = after;
            };

        undo.Push(std::move(a));
    }

    inline void PushNameEdit(UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        Name const& before,
        Name const& after)
    {


        std::cout << "[UndoH] PushNameEdit called\n";
        if (!h || h->id == INVALID_ENTITY) return;
        std::cout << "[UndoH] PushNameEdit called after early return check\n";
        UndoManager::Action a;

        a.undo = [h, before]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                std::cout << "[UndoH] PushNameEdit undo \n";
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<Name>(h->id)) return;

                c->GetComponent<Name>(h->id) = before;
            };

        a.redo = [h, after]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                std::cout << "[UndoH] PushNameEdit redo \n";
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<Name>(h->id)) return;

                c->GetComponent<Name>(h->id) = after;
            };

        undo.Push(std::move(a));
    }

    inline void PushAnimationEdit(UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        Framework::Animation const& before,
        Framework::Animation const& after)
    {
        std::cout << "[UndoH] PushAnimationEdit called\n";
        if (!h || h->id == INVALID_ENTITY) return;
        std::cout << "[UndoH] PushAnimationEdit called after early return check\n";

        UndoManager::Action a;

        a.undo = [h, before]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                std::cout << "[UndoH] PushAnimationEdit undo\n";

                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<Framework::Animation>(h->id)) return;

                c->GetComponent<Framework::Animation>(h->id) = before;
            };

        a.redo = [h, after]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                std::cout << "[UndoH] PushAnimationEdit redo\n";

                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<Framework::Animation>(h->id)) return;

                c->GetComponent<Framework::Animation>(h->id) = after;
            };

        undo.Push(std::move(a));
    }

    inline void PushAABBEdit(
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        AABB const& before,
        AABB const& after)
    {
        if (!h || h->id == INVALID_ENTITY) return;

        UndoManager::Action a;

        a.undo = [h, before]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<AABB>(h->id)) return;

                c->GetComponent<AABB>(h->id) = before;
            };

        a.redo = [h, after]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<AABB>(h->id)) return;

                c->GetComponent<AABB>(h->id) = after;
            };

        undo.Push(std::move(a));
    }

    inline void PushHealthEdit(
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        Health const& before,
        Health const& after)
    {
        if (!h || h->id == INVALID_ENTITY) return;

        UndoManager::Action a;

        a.undo = [h, before]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<Health>(h->id)) return;

                c->GetComponent<Health>(h->id) = before;
            };

        a.redo = [h, after]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<Health>(h->id)) return;

                c->GetComponent<Health>(h->id) = after;
            };

        undo.Push(std::move(a));
    }

    inline void PushTextEdit(UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        TextComponent const& before,
        TextComponent const& after)
    {
        if (!h || h->id == INVALID_ENTITY) return;

        UndoManager::Action a;

        a.undo = [h, before]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<TextComponent>(h->id)) return;

                c->GetComponent<TextComponent>(h->id) = before;
            };

        a.redo = [h, after]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<TextComponent>(h->id)) return;

                c->GetComponent<TextComponent>(h->id) = after;
            };

        undo.Push(std::move(a));
    }

    inline void PushAudioEdit(UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        AudioSource const& before,
        AudioSource const& after)
    {
        if (!h || h->id == INVALID_ENTITY) return;

        UndoManager::Action a;

        a.undo = [h, before]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<AudioSource>(h->id)) return;

                c->GetComponent<AudioSource>(h->id) = before;
            };

        a.redo = [h, after]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<AudioSource>(h->id)) return;

                c->GetComponent<AudioSource>(h->id) = after;
            };

        undo.Push(std::move(a));
    }

    inline void PushLayerEdit(UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        LayerTag const& before,
        LayerTag const& after)
    {
        if (!h || h->id == INVALID_ENTITY) return;

        UndoManager::Action a;

        a.undo = [h, before]()
            {
                if (!h || h->id == INVALID_ENTITY) return;

                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<LayerTag>(h->id)) return;

                c->GetComponent<LayerTag>(h->id) = before;
            };

        a.redo = [h, after]()
            {
                if (!h || h->id == INVALID_ENTITY) return;

                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<LayerTag>(h->id)) return;

                c->GetComponent<LayerTag>(h->id) = after;
            };

        undo.Push(std::move(a));
    }

    inline void PushLayerRegistryEdit(
        UndoManager& undo,
        LayerRegistrySnapshot const& before,
        LayerRegistrySnapshot const& after)
    {
        UndoManager::Action a;

        a.undo = [before]()
            {
                LayerRegistry::Get().RestoreSnapshot(before);
                UpdateGraphicsLayerFilter();
            };

        a.redo = [after]()
            {
                LayerRegistry::Get().RestoreSnapshot(after);
                UpdateGraphicsLayerFilter();
            };

        undo.Push(std::move(a));
    }

    inline void PushPrefabInstanceEdit(
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        PrefabInstance const& before,
        PrefabInstance const& after)
    {
        if (!h || h->id == INVALID_ENTITY) return;

        UndoManager::Action a;

        a.undo = [h, before]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<PrefabInstance>(h->id)) return;

                c->GetComponent<PrefabInstance>(h->id) = before;
            };

        a.redo = [h, after]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<PrefabInstance>(h->id)) return;

                c->GetComponent<PrefabInstance>(h->id) = after;
            };

        undo.Push(std::move(a));
    }

    inline void PushLogicEdit(
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        LogicComponent const& before,
        LogicComponent const& after)
    {
        if (!h || h->id == INVALID_ENTITY) return;

        UndoManager::Action a;

        a.undo = [h, before]()
            {
                if (!h || h->id == INVALID_ENTITY) return;

                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<LogicComponent>(h->id)) return;

                auto& logic = c->GetComponent<LogicComponent>(h->id);


                std::cout << "[UndoHelper] LogicComponent before sync engine undo\n";

                // sync engine/runtime first if needed
                for (auto const& s : logic.ScriptName)
                {
                    auto removeScriptFunc = Engine->GetFunctionPtr<void(*)(int, const char*)>(
                        "ScriptAPI",
                        "ScriptAPI.EngineInterface",
                        "RemoveScriptFromEntity"
                    );
                    if (removeScriptFunc)
                        removeScriptFunc(h->id, s.c_str());
                }

                logic = before;

                for (auto const& s : logic.ScriptName)
                {
                    auto addScriptFunc = Engine->GetFunctionPtr<bool(*)(int, const char*)>(
                        "ScriptAPI",
                        "ScriptAPI.EngineInterface",
                        "AddScriptViaName"
                    );
                    if (addScriptFunc)
                        addScriptFunc(h->id, s.c_str());
                }
            };

        a.redo = [h, after]()
            {
                if (!h || h->id == INVALID_ENTITY) return;

                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<LogicComponent>(h->id)) return;

                auto& logic = c->GetComponent<LogicComponent>(h->id);

                std::cout << "[UndoHelper] LogicComponent before sync engine redo\n";

                for (auto const& s : logic.ScriptName)
                {
                    auto removeScriptFunc = Engine->GetFunctionPtr<void(*)(int, const char*)>(
                        "ScriptAPI",
                        "ScriptAPI.EngineInterface",
                        "RemoveScriptFromEntity"
                    );
                    if (removeScriptFunc)
                        removeScriptFunc(h->id, s.c_str());
                }

                logic = after;

                for (auto const& s : logic.ScriptName)
                {
                    auto addScriptFunc = Engine->GetFunctionPtr<bool(*)(int, const char*)>(
                        "ScriptAPI",
                        "ScriptAPI.EngineInterface",
                        "AddScriptViaName"
                    );
                    if (addScriptFunc)
                        addScriptFunc(h->id, s.c_str());
                }
            };

        undo.Push(std::move(a));
    }

    /*
    * Remove Component Feature
      undo/redo



      Regular remove component
      - Records component removal for undo/redo.
      * Undo restores the removed <COMPONENT_TYPE> with its saved value.
      * Redo removes the <COMPONENT_TYPE> again from the entity
        referenced by the stable handle.
        
      Layer Component CANNOT BE REMOVED

      Logic Component needs to re-add runtime scripts
      or remove runtime scripts too depending on undo/redo

    */

    inline void PushRemoveTransform(
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        Framework::Transform const& removedValue)
    {
        if (!h || h->id == INVALID_ENTITY) return;

        UndoManager::Action a;

        a.undo = [h, removedValue]()
            {
                if (!h || h->id == INVALID_ENTITY) return;

                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (c->HasComponent<Framework::Transform>(h->id)) return;

                c->AddComponent<Framework::Transform>(h->id, removedValue);
            };

        a.redo = [h]()
            {
                if (!h || h->id == INVALID_ENTITY) return;

                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<Framework::Transform>(h->id)) return;

                c->RemoveComponent<Framework::Transform>(h->id);
            };

        undo.Push(std::move(a));
    }

    inline void PushRemoveRenderable(
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        Framework::Renderable const& removedValue)
    {
        if (!h || h->id == INVALID_ENTITY) return;

        UndoManager::Action a;

        a.undo = [h, removedValue]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (c->HasComponent<Framework::Renderable>(h->id)) return;

                c->AddComponent<Framework::Renderable>(h->id, removedValue);
            };

        a.redo = [h]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<Framework::Renderable>(h->id)) return;

                c->RemoveComponent<Framework::Renderable>(h->id);
            };

        undo.Push(std::move(a));
    }

    inline void PushRemoveAnimation(
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        Framework::Animation const& removedValue)
    {
        if (!h || h->id == INVALID_ENTITY) return;

        UndoManager::Action a;

        a.undo = [h, removedValue]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (c->HasComponent<Framework::Animation>(h->id)) return;

                c->AddComponent<Framework::Animation>(h->id, removedValue);
            };

        a.redo = [h]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<Framework::Animation>(h->id)) return;

                c->RemoveComponent<Framework::Animation>(h->id);
            };

        undo.Push(std::move(a));
    }

    inline void PushRemoveName(
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        Name const& removedValue)
    {
        if (!h || h->id == INVALID_ENTITY) return;

        UndoManager::Action a;

        a.undo = [h, removedValue]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (c->HasComponent<Name>(h->id)) return;

                c->AddComponent<Name>(h->id, removedValue);
            };

        a.redo = [h]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<Name>(h->id)) return;

                c->RemoveComponent<Name>(h->id);
            };

        undo.Push(std::move(a));
    }

    inline void PushRemoveAABB(
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        AABB const& removedValue)
    {
        if (!h || h->id == INVALID_ENTITY) return;

        UndoManager::Action a;

        a.undo = [h, removedValue]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (c->HasComponent<AABB>(h->id)) return;

                c->AddComponent<AABB>(h->id, removedValue);
            };

        a.redo = [h]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<AABB>(h->id)) return;

                c->RemoveComponent<AABB>(h->id);
            };

        undo.Push(std::move(a));
    }

    inline void PushRemoveHealth(
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        Health const& removedValue)
    {
        if (!h || h->id == INVALID_ENTITY) return;

        UndoManager::Action a;

        a.undo = [h, removedValue]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (c->HasComponent<Health>(h->id)) return;

                c->AddComponent<Health>(h->id, removedValue);
            };

        a.redo = [h]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<Health>(h->id)) return;

                c->RemoveComponent<Health>(h->id);
            };

        undo.Push(std::move(a));
    }

    inline void PushRemoveText(
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        TextComponent const& removedValue)
    {
        if (!h || h->id == INVALID_ENTITY) return;

        UndoManager::Action a;

        a.undo = [h, removedValue]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (c->HasComponent<TextComponent>(h->id)) return;

                c->AddComponent<TextComponent>(h->id, removedValue);
            };

        a.redo = [h]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<TextComponent>(h->id)) return;

                c->RemoveComponent<TextComponent>(h->id);
            };

        undo.Push(std::move(a));
    }

    inline void PushRemoveAudioSource(
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        AudioSource const& removedValue)
    {
        if (!h || h->id == INVALID_ENTITY) return;

        UndoManager::Action a;

        a.undo = [h, removedValue]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (c->HasComponent<AudioSource>(h->id)) return;

                c->AddComponent<AudioSource>(h->id, removedValue);
            };

        a.redo = [h]()
            {
                if (!h || h->id == INVALID_ENTITY) return;
                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<AudioSource>(h->id)) return;

                c->RemoveComponent<AudioSource>(h->id);
            };

        undo.Push(std::move(a));
    }
    //Layer Tag cannot be Removed
    //inline void PushRemoveLayerTag(
    //    UndoManager& undo,
    //    std::shared_ptr<EntityHandle> h,
    //    LayerTag const& removedValue)
    //{
    //    if (!h || h->id == INVALID_ENTITY) return;

    //    UndoManager::Action a;

    //    a.undo = [h, removedValue]()
    //        {
    //            if (!h || h->id == INVALID_ENTITY) return;
    //            auto* c = Coordinator::GetInstance();
    //            if (!c) return;
    //            if (c->HasComponent<LayerTag>(h->id)) return;

    //            c->AddComponent<LayerTag>(h->id, removedValue);
    //        };

    //    a.redo = [h]()
    //        {
    //            if (!h || h->id == INVALID_ENTITY) return;
    //            auto* c = Coordinator::GetInstance();
    //            if (!c) return;
    //            if (!c->HasComponent<LayerTag>(h->id)) return;

    //            c->RemoveComponent<LayerTag>(h->id);
    //        };

    //    undo.Push(std::move(a));
    //}

    inline void PushRemoveLogic(
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        LogicComponent const& removedValue)
    {
        if (!h || h->id == INVALID_ENTITY) return;

        UndoManager::Action a;

        a.undo = [h, removedValue]()
            {
                if (!h || h->id == INVALID_ENTITY) return;

                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (c->HasComponent<LogicComponent>(h->id)) return;

                // Restore ECS component first
                c->AddComponent<LogicComponent>(h->id, removedValue);

                // Restore runtime script bindings
                auto addScriptFunc = Engine->GetFunctionPtr<bool(*)(int, const char*)>(
                    "ScriptAPI",
                    "ScriptAPI.EngineInterface",
                    "AddScriptViaName"
                );

                if (addScriptFunc)
                {
                    for (auto const& scriptName : removedValue.ScriptName)
                    {
                        addScriptFunc(h->id, scriptName.c_str());
                    }
                }
            };

        a.redo = [h]()
            {
                if (!h || h->id == INVALID_ENTITY) return;

                auto* c = Coordinator::GetInstance();
                if (!c) return;
                if (!c->HasComponent<LogicComponent>(h->id)) return;

                auto& logic = c->GetComponent<LogicComponent>(h->id);

                // Remove runtime script bindings first
                auto removeScriptFunc = Engine->GetFunctionPtr<void(*)(int, const char*)>(
                    "ScriptAPI",
                    "ScriptAPI.EngineInterface",
                    "RemoveScriptFromEntity"
                );

                if (removeScriptFunc)
                {
                    for (auto const& scriptName : logic.ScriptName)
                    {
                        removeScriptFunc(h->id, scriptName.c_str());
                    }
                }

                // Then remove ECS component
                c->RemoveComponent<LogicComponent>(h->id);
            };

        undo.Push(std::move(a));
    }

}

