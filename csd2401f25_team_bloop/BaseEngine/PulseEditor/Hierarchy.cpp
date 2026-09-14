/******************************************************************************/
/**
 * @file        Hierarchy.cpp
 * @project     Pulse Protocol
 * @author      Leu Jun Yong (primary) - 60%
 * @author      Chloe Lau Rey En (secondary) - 15%
 * @author      Ban Kai Wei Benjamin (secondary) - 15%
 * @author      Reginald Lew Yee Ren (secondary) - 10%
 * @brief       Scene hierarchy panel with entity list, drag-drop reordering, and inline renaming.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

// -----------------------------------------------------------------------------
// Hierarchy.cpp  -  Scene hierarchy panel (entity list + toolbar)
// -----------------------------------------------------------------------------
#include "pch/pch_temp.h"
#include "Hierarchy.h"
#include "HierarchyHelpers.h"
#include "Editor.h"   // selectedEntity, INVALID_ENTITY, entityDisplayOrder, Game/ECS helpers
#include "TopBarPanel.h"
#include <unordered_map>
#include "CoreEngine/Prefab/PrefabInstance.h"
#include "Components/ParentChild.h"

#include "UndoHelper.h"
#include "EditorTypes.h"
// ImGui
#include "imgui.h"
//
#include "SnapShot.h"

// Everything else (Factory, Name, GUIButton, Renderable, Transform, Animation)
// is already included via Editor.h

namespace PulseEditor
{

    // -------------------------------------------------------------------------
    // CONFIGURATION CONSTANTS (Change these to customize behavior)
    // -------------------------------------------------------------------------

    namespace HierarchyConfig
    {
        // Default name for new entities
        constexpr const char* DEFAULT_ENTITY_NAME = "GameObject";

        // Default transform scale for new entities
        constexpr float DEFAULT_SCALE_X = 200.0f;
        constexpr float DEFAULT_SCALE_Y = 200.0f;

        // Maximum attempts to find a unique name before giving up
        constexpr int MAX_NAME_ATTEMPTS = 10000;

        // Fallback name if all else fails
        constexpr const char* FALLBACK_ENTITY_NAME = "GameObject (New)";
    }

    // -------------------------------------------------------------------------
    // RENAME STATE (Inline renaming)
    // -------------------------------------------------------------------------

    struct RenameState
    {
        Entity renamingEntity = INVALID_ENTITY;
        char renameBuffer[256] = "";
        bool justStartedRenaming = false;
        Name beforeEdit{};
        // [ADDED] True when the user tried to confirm a name already used by
        // another entity. Keeps the input open and shows a red warning.
        bool showDuplicateWarning = false;
    };

    static RenameState s_renameState;

    // Returns true if any other alive entity already uses this name.
    static bool NameAlreadyExists(const std::string& name, Entity excludeEntity)
    {
        auto* g = Coordinator::GetInstance();
        if (!g) return false;
        for (Entity e : g->GetAllEntities())
        {
            if (e == excludeEntity) continue;
            if (!g->IsAlive(e)) continue;
            if (g->HasComponent<Name>(e) && g->GetComponent<Name>(e).name == name)
                return true;
        }
        return false;
    }

    // -------------------------------------------------------------------------
    // HELPER FUNCTIONS
    // -------------------------------------------------------------------------

    // Helper function to generate unique GameObject names
    static std::string GenerateUniqueGameObjectName()
    {
        using namespace HierarchyConfig;

        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator)
            return DEFAULT_ENTITY_NAME;

        std::set<std::string> existingNames;

        // Collect all existing names
        auto allEntities = g_coordinator->GetAllEntities();
        for (auto entity : allEntities)
        {
            if (g_coordinator->HasComponent<Name>(entity))
            {
                existingNames.insert(g_coordinator->GetComponent<Name>(entity).name);
            }
        }

        // Try default name first
        if (existingNames.find(DEFAULT_ENTITY_NAME) == existingNames.end())
            return DEFAULT_ENTITY_NAME;

        // Then try numbered variants
        for (int i = 1; i < MAX_NAME_ATTEMPTS; ++i)
        {
            std::string candidate = std::string(DEFAULT_ENTITY_NAME) + " (" + std::to_string(i) + ")";
            if (existingNames.find(candidate) == existingNames.end())
                return candidate;
        }

        // Fallback if we exhausted all attempts (shouldn't happen)
        return FALLBACK_ENTITY_NAME;
    }

    // Helper function to create entity with auto-generated name
    Entity CreateEntityWithName(const std::string& baseName = "")
    {
        using namespace HierarchyConfig;

        Factory factory;
        Entity newEntity = factory.DefaultObj();

        if (newEntity == INVALID_ENTITY)
            return INVALID_ENTITY;

        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator)
            return newEntity;

        // Set default transform scale
        if (g_coordinator->HasComponent<Framework::Transform>(newEntity))
        {
            auto& t = g_coordinator->GetComponent<Framework::Transform>(newEntity);
            t.Scale = { DEFAULT_SCALE_X, DEFAULT_SCALE_Y };
        }

        // Auto-generate or use provided name
        std::string entityName = baseName.empty() ? GenerateUniqueGameObjectName() : baseName;

        // Add or update Name component
        if (g_coordinator->HasComponent<Name>(newEntity))
        {
            g_coordinator->GetComponent<Name>(newEntity).name = entityName;
        }
        else
        {
            Name nameComp;
            nameComp.name = entityName;
            g_coordinator->AddComponent<Name>(newEntity, nameComp);
        }

        return newEntity;
    }

    // Helper to start renaming an entity
    static void StartRenaming(Entity entity)
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator || entity == INVALID_ENTITY)
            return;

        // Auto-add Name component if missing
        if (!g_coordinator->HasComponent<Name>(entity))
        {
            Name newName;
            newName.name = "Entity " + std::to_string(static_cast<int>(entity));
            g_coordinator->AddComponent<Name>(entity, newName);
        }


        if (g_coordinator->HasComponent<Name>(entity))
        {
            s_renameState.renamingEntity = entity;
            //Undo name
            s_renameState.beforeEdit = g_coordinator->GetComponent<Name>(entity);
            std::cout << "[Hierarchy RENAME] before : " << s_renameState.beforeEdit.name << "\n";
            std::string currentName = g_coordinator->GetComponent<Name>(entity).name;
            std::strncpy(s_renameState.renameBuffer, currentName.c_str(), sizeof(s_renameState.renameBuffer) - 1);
            s_renameState.renameBuffer[sizeof(s_renameState.renameBuffer) - 1] = '\0';
            s_renameState.justStartedRenaming = true;
        }
    }

    // Helper to finish renaming
    static void FinishRenaming(bool save)
    {
        if (save && s_renameState.renamingEntity != INVALID_ENTITY)
        {
            auto* g_coordinator = Coordinator::GetInstance();
            if (g_coordinator && g_coordinator->HasComponent<Name>(s_renameState.renamingEntity))
            {
                std::string newName = s_renameState.renameBuffer;
                if (!newName.empty())
                {
                    // Block save if the name already belongs to another entity.
                    // Keep the input open with a red warning until they fix it.
                    if (NameAlreadyExists(newName, s_renameState.renamingEntity))
                    {
                        s_renameState.showDuplicateWarning = true;
                        s_renameState.justStartedRenaming = true; // re-focus input
                        return; // do NOT close or save
                    }

                    s_renameState.showDuplicateWarning = false;

                    //Undo name
                    Name after;
                    after.name = newName;
                    std::cout << "[Hierarchy RENAME] after : " << after.name << "\n";
                    if (s_renameState.beforeEdit != after)
                    {
                        Entity e = s_renameState.renamingEntity;

                        auto it = handleOf.find(e);
                        //std::shared_ptr<EntityHandle> h =
                        //    (it != handleOf.end()) ? it->second : nullptr;

                        g_coordinator->GetComponent<Name>(e) = after;
                        //auto h = EnsureHandle(e);
                        PushNameEdit(mUndo, selectedHandle, s_renameState.beforeEdit, after);
                    }
                    else
                    {
                        g_coordinator->GetComponent<Name>(s_renameState.renamingEntity) = after;
                    }
                }
            }
        }

        s_renameState.renamingEntity = INVALID_ENTITY;
        s_renameState.renameBuffer[0] = '\0';
        s_renameState.justStartedRenaming = false;
        s_renameState.showDuplicateWarning = false;
    }

    // =====================================================
    // Prefab parent/child numbering (EDITOR-ONLY UI)
    //  - First instance of a prefab in entityDisplayOrder => [P]
    //  - Next ones => [C1], [C2], ...
    // =====================================================
    static std::unordered_map<Entity, int> s_prefabChildNumber; // 0 = parent, 1.. = child index

    static void RebuildPrefabChildNumbers(const std::vector<Entity>& displayOrder)
    {
        s_prefabChildNumber.clear();

        auto* coord = Coordinator::GetInstance();
        if (!coord) return;

        std::unordered_map<std::string, Entity> parentOfPrefab; // prefabName -> entity
        std::unordered_map<std::string, int> childCount;        // prefabName -> count

        for (Entity e : displayOrder)
        {
            if (!coord->HasComponent<PrefabInstance>(e))
                continue;

            const auto& inst = coord->GetComponent<PrefabInstance>(e);
            if (inst.prefabName.empty())
                continue;

            auto it = parentOfPrefab.find(inst.prefabName);
            if (it == parentOfPrefab.end())
            {
                // First instance encountered becomes the parent
                parentOfPrefab[inst.prefabName] = e;
                s_prefabChildNumber[e] = 0; // parent
            }
            else
            {
                // Subsequent ones are children, numbered from 1
                int next = ++childCount[inst.prefabName];
                s_prefabChildNumber[e] = next; // C1, C2...
            }
        }
    }

    // -------------------------------------------------------------------------
    // RECURSIVE TREE NODE DRAWING
    // -------------------------------------------------------------------------
    static void DrawEntityNode(Entity entity, int& index, int depth, bool& anySelectableClicked, int& draggedIndex)
    {
        auto* g_coordinator = Coordinator::GetInstance();
        if (!g_coordinator) return;
        if (!g_coordinator->IsAlive(entity)) return;

        // Get display name
        std::string display;
        if (g_coordinator->HasComponent<Name>(entity))
        {
            display = g_coordinator->GetComponent<Name>(entity).name;
        }
        else
        {
            display = "Entity " + std::to_string(static_cast<int>(entity));
        }

        // Check for prefab instance
        bool isPrefabInstance = false;
        bool isPrefabLinked = false;
        if (g_coordinator->HasComponent<PrefabInstance>(entity))
        {
            isPrefabInstance = true;
            isPrefabLinked = g_coordinator->GetComponent<PrefabInstance>(entity).isLinked;
        }

        if (isPrefabInstance)
        {
            int num = 0;
            auto it = s_prefabChildNumber.find(entity);
            if (it != s_prefabChildNumber.end())
                num = it->second;

            std::string tag;
            if (num == 0) tag = "[P]";
            else tag = "[C" + std::to_string(num) + "]";

            // Optional: preserve your linked/unlinked visual
            if (!isPrefabLinked)
                tag.insert(tag.size() - 1, "-"); // [P] -> [P-], [C1] -> [C1-]

            display = tag + " " + display;
        }

        // Check if has children
        bool hasChildren = HierarchyHelpers::HasChildren(entity);
        std::vector<Entity> children = HierarchyHelpers::GetChildren(entity);

        bool isSelected = (selectedEntity == entity);
        std::string uniqueLabel = display + "##" + std::to_string(static_cast<int>(entity));

        // Check if currently renaming this entity
        bool isRenaming = (s_renameState.renamingEntity == entity);

        // Indent based on depth
        float indentWidth = depth * 20.0f;
        if (indentWidth > 0)
            ImGui::Indent(indentWidth);

        // ═══════════════════════════════════════════════════════════════════════
        // INLINE RENAMING
        // ═══════════════════════════════════════════════════════════════════════
        if (isRenaming)
        {
            // Live duplicate check — tint the input red while typing a taken name
            std::string currentBuffer = s_renameState.renameBuffer;
            bool isDuplicate = !currentBuffer.empty()
                && NameAlreadyExists(currentBuffer, s_renameState.renamingEntity);

            if (isDuplicate)
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.55f, 0.1f, 0.1f, 1.0f));

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

            if (s_renameState.justStartedRenaming)
            {
                ImGui::SetKeyboardFocusHere();
                s_renameState.justStartedRenaming = false;
            }

            ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue |
                ImGuiInputTextFlags_AutoSelectAll;

            if (ImGui::InputText(("##rename" + std::to_string(entity)).c_str(),
                s_renameState.renameBuffer,
                sizeof(s_renameState.renameBuffer), flags))
            {
                FinishRenaming(true);
            }

            if (isDuplicate)
                ImGui::PopStyleColor();

            // Red warning text below the input when name is taken
            if (isDuplicate)
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Name already exists!");

            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                FinishRenaming(false);
            }

            if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0))
            {
                FinishRenaming(true);
            }
        }
        else
        {
            // ═══════════════════════════════════════════════════════════════════════
            // TREE NODE DISPLAY
            // ═══════════════════════════════════════════════════════════════════════

            // Tree node flags
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_SpanAvailWidth |
                ImGuiTreeNodeFlags_DefaultOpen;

            if (!hasChildren)
                flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

            if (isSelected)
                flags |= ImGuiTreeNodeFlags_Selected;

            // Color for prefab instances
            if (isPrefabInstance && isPrefabLinked)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
            else if (isPrefabInstance)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.7f, 1.0f));

            // Draw tree node
            bool nodeOpen = ImGui::TreeNodeEx(
                (void*)(intptr_t)entity,
                flags,
                "%s",
                display.c_str()
            );

            if (isPrefabInstance)
                ImGui::PopStyleColor();

            // Handle selection
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            {
                selectedEntity = entity;
                //aaa
                auto it = handleOf.find(entity);
                selectedHandle = (it != handleOf.end()) ? it->second : nullptr;
                anySelectableClicked = true;
            }

            // ═══════════════════════════════════════════════════════════════════════
            // DRAG SOURCE - For reparenting
            // ═══════════════════════════════════════════════════════════════════════
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                draggedIndex = index;

                struct HierarchyDragData {
                    int index;
                    Entity entity;
                };

                static HierarchyDragData s_dragData;
                s_dragData.index = index;
                s_dragData.entity = entity;

                ImGui::SetDragDropPayload("HIERARCHY_ENTITY_REORDER", &s_dragData, sizeof(HierarchyDragData));
                ImGui::Text("%s", display.c_str());
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Drop on entity to parent");
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Drop on empty = unparent");
                ImGui::EndDragDropSource();
            }

            // ═══════════════════════════════════════════════════════════════════════
            // DROP TARGET - For making children
            // ═══════════════════════════════════════════════════════════════════════
            if (ImGui::BeginDragDropTarget())
            {
                // Visual feedback
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 p_min = ImGui::GetItemRectMin();
                ImVec2 p_max = ImGui::GetItemRectMax();
                draw_list->AddRectFilled(p_min, p_max, IM_COL32(0, 150, 255, 50));

                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY_REORDER"))
                {
                    struct HierarchyDragData {
                        int index;
                        Entity entity;
                    };

                    const HierarchyDragData* dragData = (const HierarchyDragData*)payload->Data;
                    Entity droppedEntity = dragData->entity;

                    // Don't parent to self or to own descendants
                    if (droppedEntity != entity && HierarchyHelpers::CanSetParent(droppedEntity, entity))
                    {
                        HierarchyHelpers::SetParent(droppedEntity, entity);
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // Double-click to rename
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                StartRenaming(entity);
            }

            // ═══════════════════════════════════════════════════════════════════════
            // RIGHT-CLICK CONTEXT MENU
            // ═══════════════════════════════════════════════════════════════════════
            if (ImGui::BeginPopupContextItem())
            {
                if (HierarchyHelpers::HasParent(entity))
                {
                    if (ImGui::MenuItem("Unparent"))
                    {
                        HierarchyHelpers::SetParent(entity, HierarchyHelpers::INVALID);
                    }
                }

                if (ImGui::MenuItem("Create Child"))
                {
                    Entity newChild = CreateEntityWithName();
                    if (newChild != INVALID_ENTITY)
                    {
                        HierarchyHelpers::SetParent(newChild, entity);
                        entityDisplayOrder.push_back(newChild);
                        selectedEntity = newChild;
                    }
                }

                ImGui::EndPopup();
            }

            // ═══════════════════════════════════════════════════════════════════════
            // DRAW CHILDREN RECURSIVELY
            // ═══════════════════════════════════════════════════════════════════════
            if (hasChildren && nodeOpen)
            {
                for (Entity child : children)
                {
                    // Unindent before recursion, child will add its own indent
                    if (indentWidth > 0)
                        ImGui::Unindent(indentWidth);

                    index++;
                    DrawEntityNode(child, index, depth + 1, anySelectableClicked, draggedIndex);

                    // Re-indent after child is done
                    if (indentWidth > 0)
                        ImGui::Indent(indentWidth);
                }
                ImGui::TreePop();
            }
        }

        if (indentWidth > 0)
            ImGui::Unindent(indentWidth);

        index++;
    }

    // -------------------------------------------------------------------------
    // MAIN HIERARCHY WINDOW
    // -------------------------------------------------------------------------

    void DrawHierarchyWindow(ImGuiID dockID)
    {

        if (dockID)
            ImGui::SetNextWindowDockID(dockID, ImGuiCond_FirstUseEver);

        if (!ImGui::Begin("Hierarchy"))
        {
            ImGui::End();
            return;
        }

        auto* g_coordinator = Coordinator::GetInstance();


        if (!g_coordinator)
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Coordinator not found!");
            ImGui::End();
            return;
        }

        // STICK WITH ONLY EMPTY ENTITY FROM NOW ON

        //static std::vector<EntityPrefab> entityPrefabs = {
        //    { "Empty Entity",  nullptr,                                   true, 200.0f, 200.0f },
        //    { "Hero",          "../../PulseEngine/JSON/HeroPrefab.json",     false, 0.0f,  0.0f   },
        //    { "Monster",       "../../PulseEngine/JSON/MonsterPrefab.json",  false, 0.0f,  0.0f   },
        //    { "Background",    nullptr,                                   true, 800.0f, 600.0f }
        //};

        //// which entity type to create (index into prefabs array)
        //static int selectedType = 0;

        //// Build display names for combo box
        //std::vector<const char*> displayNames;
        //displayNames.reserve(entityPrefabs.size());
        //for (const auto& prefab : entityPrefabs)
        //    displayNames.push_back(prefab.displayName);

        // ============================================
        // TOOLBAR - CREATE & DELETE & REORDER
        // ============================================

        // Disable all editing when playing
        bool isPlaying = IsPlaying();

        // Track scene changes during play mode
        static std::string lastSceneDuringPlay = "";

        if (isPlaying)
        {
            // Get current scene from SceneManager
            auto sceneManager = g_coordinator->GetSystem<SceneManager>();
            std::string currentScene = sceneManager ? sceneManager->GrabCurrentScene() : "";

            // If scene changed while playing, deselect
            if (!lastSceneDuringPlay.empty() && currentScene != lastSceneDuringPlay)
            {
                selectedEntity = INVALID_ENTITY;
                // Also cancel any renaming in progress
                if (s_renameState.renamingEntity != INVALID_ENTITY)
                {
                    FinishRenaming(false);
                }
            }

            lastSceneDuringPlay = currentScene;
        }
        else
        {
            // Reset tracking when not playing
            lastSceneDuringPlay = "";
        }

        // -----------------------------------------------------------------------------
        // [+] CREATE EMPTY ENTITY BUTTON
        // -----------------------------------------------------------------------------
        if (ImGui::Button("+"))
        {
            //Entity prevSelected = selectedEntity;

            //Factory factory;
            selectedHandle = PushCreateDefault(mUndo, Editorfactory, entityDisplayOrder, handleOf);
            selectedEntity = selectedHandle ? selectedHandle->id : INVALID_ENTITY;
            // Entity newEntity = factory.DefaultObj();
             //if (newEntity != INVALID_ENTITY)
             //{
             //    //entityDisplayOrder.push_back(newEntity);
             //    //selectedEntity = newEntity;

             //    selectedHandle = PushCreateDefault(mUndo, factory, entityDisplayOrder);
             //}
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Create Empty GameObject");

        ImGui::SameLine();

        // Buttons that require selection
        ImGui::BeginDisabled(selectedEntity == INVALID_ENTITY);


        // -----------------------------------------------------------------------------
        // [X] DELETE SELECTED ENTITY BUTTON
        // -----------------------------------------------------------------------------
        if (ImGui::Button("X"))
        {
            if (selectedHandle && selectedHandle->id != INVALID_ENTITY)
            {
                // Entity toDelete = selectedEntity;

                // // Collect the whole subtree (root + all descendants)
                // std::vector<Entity> entitiesToDelete;
                // HierarchyHelpers::PrepareDeleteWithChildren(toDelete, entitiesToDelete);

                //// Factory factory;
                // PushDeleteEntity(mUndo, Editorfactory, entityDisplayOrder, selectedHandle, handleOf);
                // selectedHandle.reset();
                // selectedEntity = INVALID_ENTITY;

                PushDeleteSubtree(mUndo, Editorfactory, entityDisplayOrder, handleOf,
                    selectedEntity, selectedEntity, selectedHandle);
            }
        }

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Delete Selected GameObject");

        ImGui::EndDisabled();

        ImGui::Separator();

        // ============================================
        // ENTITY LIST WITH DRAG-DROP REORDERING & PREFAB CREATION
        // ============================================

        ImGui::BeginChild("HierarchyList",
            ImVec2(-FLT_MIN, -FLT_MIN),
            true,
            ImGuiWindowFlags_AlwaysUseWindowPadding);

        if (ImGui::IsWindowHovered())
            ImGui::SetWindowFocus();

        bool anySelectableClicked = false;

        //aaa
        // Sync display order with actual entities
        //auto allEntities = g_coordinator->GetAllEntities();
        //std::set<Entity> entitySet(allEntities.begin(), allEntities.end());
        auto allEntities = g_coordinator->GetAllEntities();

        std::set<Entity> entitySet;
        for (Entity e : allEntities)
        {
            if (g_coordinator->IsAlive(e))          // <-- use your alive check function
                entitySet.insert(e);
        }

        // Remove deleted entities from display order
        entityDisplayOrder.erase(
            std::remove_if(entityDisplayOrder.begin(), entityDisplayOrder.end(), [&entitySet](Entity e) { return entitySet.find(e) == entitySet.end(); }), entityDisplayOrder.end()
        );

        // Add any new entities not in display order
        for (auto entity : allEntities)
        {
            //aaa
            if (!g_coordinator->IsAlive(entity))    // <-- add this line
                continue;

            if (std::find(entityDisplayOrder.begin(), entityDisplayOrder.end(), entity) == entityDisplayOrder.end())
            {
                entityDisplayOrder.push_back(entity);
            }
        }

        // NEW: rebuild prefab parent/child numbering for UI tags
        RebuildPrefabChildNumbers(entityDisplayOrder);

        // Display entities as tree view
        static int draggedIndex = -1;
        int index = 0;

        // Only draw root entities (children drawn recursively)
        for (Entity entity : entityDisplayOrder)
        {
            if (HierarchyHelpers::IsRoot(entity))
            {
                DrawEntityNode(entity, index, 0, anySelectableClicked, draggedIndex);
            }
        }

        // Drop zone for unparenting (empty space)
        ImVec2 availSpace = ImGui::GetContentRegionAvail();
        if (availSpace.y > 20.0f)
        {
            ImGui::InvisibleButton("##unparent_dropzone", ImVec2(-1, availSpace.y));

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY_REORDER"))
                {
                    struct HierarchyDragData {
                        int index;
                        Entity entity;
                    };

                    const HierarchyDragData* dragData = (const HierarchyDragData*)payload->Data;
                    HierarchyHelpers::SetParent(dragData->entity, HierarchyHelpers::INVALID);
                }
                ImGui::EndDragDropTarget();
            }
        }

        // Deselect if clicked on empty space
        if (ImGui::IsWindowHovered() &&
            ImGui::IsMouseClicked(0) &&
            !anySelectableClicked)
        {
            selectedEntity = INVALID_ENTITY;
            //aaa
            selectedHandle.reset();
            // Also cancel renaming if clicking empty space
            if (s_renameState.renamingEntity != INVALID_ENTITY)
            {
                FinishRenaming(true);
            }
        }

        ImGui::EndChild();

        ImGui::End();
    }


}