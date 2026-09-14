/******************************************************************************/
/**
 * @file        Inspector.cpp
 * @project     Pulse Protocol
 * @author      Leu Jun Yong (primary) - 50%
 *              Chloe Lau Rey En (secondary) - 15%
 *              Ban Kai Wei Benjamin (secondary) - 15%
 *              Goh Pin Kai (secondary) - 5%
 *              Reginald Lew Yee Ren (secondary) - 15%
 * 
 * @brief       Entity inspector panel with component editors and undo support.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/

#include "pch/pch_temp.h"

// editor files
#include "Inspector.h"
#include "Editor.h"                 // selectedEntity, INVALID_ENTITY, PrefabManager, etc.
#include "TopBarPanel.h"
#include "SnapShot.h"

// engine files
#include "Graphics/glapp.h"         // GLApp::shdrpgms
#include "Graphics/Layer.h"         // GLApp::shdrpgms
#include "Graphics/LayerRegistry.h" // for dynamic layer names
#include "Graphics/Text.h"
#include "CoreEngine/Core/CoreEngine.h"      // for FilePathToGame
#include "CoreEngine/Asset/AssetsManager.h"
#include "CoreEngine/Prefab/PrefabInstance.h"
#include "EditorTypes.h"
#include "UndoHelper.h"

//#include "Graphics/Renderable.h"

namespace PulseEditor
{
    // Definition of singleton undomanager for editor;
    // UNDO Manager
    static bool isEditing_Transform = false;
    static Framework::Transform beforeEdit_Transform;

    static bool isEditing_Renderable = false;
    static Framework::Renderable beforeEdit_Renderable{};

    static Framework::Animation beforeEdit_Animation{};
    static bool isEditing_Animation = false;

    static Name beforeEdit_Name{};
    static bool isEditing_Name = false;

    static AABB beforeEdit_AABB;
    static bool isEditing_AABB = false;

    static Health beforeEdit_Health;
    static bool isEditing_Health = false;

    static bool isEditing_Text = false;
    static TextComponent beforeEdit_Text{};

    static bool isEditing_AudioSource = false;
    static AudioSource beforeEdit_AudioSource{};

    static bool isEditing_LayerTag = false;
    static LayerTag beforeEdit_LayerTag{};

    static bool isEditing_PrefabInstance = false;
    static PrefabInstance beforeEdit_PrefabInstance{};

    static bool isEditing_Logic = false;
    static LogicComponent beforeEdit_Logic{};


    // -------------------------------------------------------------------------
    // Function to handle Transform undo
    // -------------------------------------------------------------------------
    //static void HandleTransformUndo(
    //    Framework::Transform &currentTransform,
    //    UndoManager &undo,
    //    Entity e,
    //    Coordinator *g)
    //{
    //    // If the current ImGui widget became active (user clicked or started dragging)
    //    if (ImGui::IsItemActivated())
    //    {
    //        // "snapshot" the "before" state
    //        // "save the transform data before editting"
    //        beforeEdit = currentTransform;
    //        isEditing = true;
    //    }

    //    // If editing has ended (mouse released or finalizing text input)
    //    if (ImGui::IsItemDeactivatedAfterEdit() && isEditing)
    //    {
    //        if (beforeEdit != currentTransform) // if something actually changed
    //        {
    //            Framework::Transform after = currentTransform;
    //            PushTransformEdit(mUndo, selectedHandle, beforeEdit, after);
    //        }

    //        isEditing = false; // reset edit session
    //    }
    //}

    static void HandleTransformUndo(
        Framework::Transform& currentTransform,
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h)
    {
        // Started dragging / editing THIS widget
        if (ImGui::IsItemActivated())
        {
            beforeEdit_Transform = currentTransform;
            isEditing_Transform = true;
        }

        // Finished editing THIS widget
        if (ImGui::IsItemDeactivatedAfterEdit() && isEditing_Transform)
        {
            std::cout << "done editing widget transform\n";
            Framework::Transform after = currentTransform;
            std::cout << "[INSPECTOR] AFTER EDITTING values" << after << "\n";
            if (beforeEdit_Transform != after)
            {
                std::cout << "BEFORE != AFTER !!!!! pushing transform edit\n";
                PushTransformEdit(undo, h, beforeEdit_Transform, after);
            }

            isEditing_Transform = false;
        }
    }

    static void HandleRenderableUndo(
        Framework::Renderable& currentRenderable,
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h) {
        if (ImGui::IsItemActivated())
        {
            beforeEdit_Renderable = currentRenderable;
            isEditing_Renderable = true;
        }

        // Finished editing THIS widget
        if (ImGui::IsItemDeactivatedAfterEdit() && isEditing_Renderable)
        {
            Framework::Renderable after = currentRenderable;
            if (beforeEdit_Renderable != after)
            {
                PushRenderableEdit(undo, h, beforeEdit_Renderable, after);
            }
            isEditing_Renderable = false;
        }
    }

    static void HandleNameUndo(
        Name& currentName,
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h)
    {
        if (ImGui::IsItemActivated())
        {
            std::cout << "[Inspector] HandleNameUndo called\n";
            beforeEdit_Name = currentName;
            isEditing_Name = true;
        }

        if (ImGui::IsItemDeactivatedAfterEdit() && isEditing_Name)
        {
            Name after = currentName;
            if (beforeEdit_Name != after)
            {
                PushNameEdit(undo, h, beforeEdit_Name, after);
            }

            isEditing_Name = false;
        }
    }

    static void HandleAnimationUndo(
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h,
        Framework::Animation const& before,
        Framework::Animation const& after)
    {
        if (before != after)
        {
            PushAnimationEdit(undo, h, before, after);
        }
    }

    static void HandleAABBUndo(
        AABB& currentAABB,
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h)
    {
        if (ImGui::IsItemActivated())
        {
            beforeEdit_AABB = currentAABB;
            isEditing_AABB = true;
        }

        if (ImGui::IsItemDeactivatedAfterEdit() && isEditing_AABB)
        {
            AABB after = currentAABB;
            if (beforeEdit_AABB != after)
            {
                PushAABBEdit(undo, h, beforeEdit_AABB, after);
            }
            isEditing_AABB = false;
        }
    }

    static void HandleHealthUndo(
        Health& currentHealth,
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h)
    {
        if (ImGui::IsItemActivated())
        {
            beforeEdit_Health = currentHealth;
            isEditing_Health = true;
        }

        if (ImGui::IsItemDeactivatedAfterEdit() && isEditing_Health)
        {
            Health after = currentHealth;
            if (beforeEdit_Health != after)
            {
                PushHealthEdit(undo, h, beforeEdit_Health, after);
            }
            isEditing_Health = false;
        }
    }

    static void HandleTextUndo(
        TextComponent& currentText,
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h)
    {
        if (ImGui::IsItemActivated())
        {
            beforeEdit_Text = currentText;
            isEditing_Text = true;
        }

        if (ImGui::IsItemDeactivatedAfterEdit() && isEditing_Text)
        {
            TextComponent after = currentText;
            if (beforeEdit_Text != after)
            {
                PushTextEdit(undo, h, beforeEdit_Text, after);
            }

            isEditing_Text = false;
        }
    }

    static void HandleAudioUndo(
        AudioSource& currentAudio,
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h)
    {
        if (ImGui::IsItemActivated())
        {
            beforeEdit_AudioSource = currentAudio;
            isEditing_AudioSource = true;
        }

        if (ImGui::IsItemDeactivatedAfterEdit() && isEditing_AudioSource)
        {
            AudioSource after = currentAudio;
            if (beforeEdit_AudioSource != after)
            {
                PushAudioEdit(undo, h, beforeEdit_AudioSource, after);
            }

            isEditing_AudioSource = false;
        }
    }

    static void HandleLayerUndo(
        LayerTag& currentLayer,
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h)
    {
        if (ImGui::IsItemActivated())
        {
            beforeEdit_LayerTag = currentLayer;
            isEditing_LayerTag = true;
        }

        if (ImGui::IsItemDeactivatedAfterEdit() && isEditing_LayerTag)
        {
            LayerTag after = currentLayer;
            if (beforeEdit_LayerTag != after)
            {
                PushLayerEdit(undo, h, beforeEdit_LayerTag, after);
            }

            isEditing_LayerTag = false;
        }
    }

    static void HandlePrefabInstanceUndo(
        PrefabInstance& currentPrefabInstance,
        UndoManager& undo,
        std::shared_ptr<EntityHandle> h)
    {
        if (ImGui::IsItemActivated())
        {
            beforeEdit_PrefabInstance = currentPrefabInstance;
            isEditing_PrefabInstance = true;
        }

        if (ImGui::IsItemDeactivatedAfterEdit() && isEditing_PrefabInstance)
        {
            PrefabInstance after = currentPrefabInstance;
            if (beforeEdit_PrefabInstance != after)
            {
                PushPrefabInstanceEdit(undo, h, beforeEdit_PrefabInstance, after);
            }

            isEditing_PrefabInstance = false;
        }
    }
    // Don't have HandleLogicComponentUndo 
    // as PushLogicEdit in UndoHelper.h is different from the rest.
    //

    // -------------------------------------------------------------------------
    // Helper: add a "add component" button at the bottom, more intuitive
    // compared to right-clicking
    // -------------------------------------------------------------------------
    static void DrawAddComponentMenu(Coordinator* g, Entity entity)
    {
        // This will hold all the "add component" menu items
        auto* coord = g;

        // Transform
        if (!coord->HasComponent<Framework::Transform>(entity))
        {
            if (ImGui::MenuItem("Transform"))
            {
                Framework::Transform t{};
                t.Pos = { 0.0f, 0.0f };
                t.Scale = { 1.0f, 1.0f };
                t.Rotation.x = 0.0f;
                t.Rotation.y = 0.0f;
                t.isVisible = true;
                t.autoHide = false;
                coord->AddComponent<Framework::Transform>(entity, t);
            }
        }

        // Renderable
        if (!coord->HasComponent<Framework::Renderable>(entity))
        {
            if (ImGui::MenuItem("Renderable"))
            {
                Framework::Renderable r{};
                r.mdl_ref = 1.0f;           // Sprite quad model
                r.shd_ref = 2.0f;           // Sprite shader
                coord->AddComponent<Framework::Renderable>(entity, r);
            }
        }

        // Animation
        if (!coord->HasComponent<Framework::Animation>(entity))
        {
            if (ImGui::MenuItem("Animation"))
            {
                Framework::Animation a{};
                coord->AddComponent<Framework::Animation>(entity, a);
            }
        }

        // LogicComponent
        if (!coord->HasComponent<LogicComponent>(entity))
        {
            if (ImGui::MenuItem("LogicComponent"))
            {
                LogicComponent lc{};
                coord->AddComponent<LogicComponent>(entity, lc);
            }
        }

        // Health
        if (!coord->HasComponent<Health>(entity))
        {
            if (ImGui::MenuItem("Health"))
            {
                Health h{};  // Uses defaults: hp=10, damage=1, isAlive=true
                coord->AddComponent<Health>(entity, h);
            }
        }

        // AABB
        if (!coord->HasComponent<AABB>(entity))
        {
            if (ImGui::MenuItem("AABB"))
            {
                AABB a{};
                a.min = { -0.5f, -0.5f };
                a.max = { 0.5f, 0.5f };
                coord->AddComponent<AABB>(entity, a);
            }
        }

        // AudioSource
        if (!coord->HasComponent<AudioSource>(entity))
        {
            if (ImGui::MenuItem("AudioSource"))
            {
                AudioSource as{};
                coord->AddComponent<AudioSource>(entity, as);
            }
        }

        // TextComponent
        if (!coord->HasComponent<TextComponent>(entity))
        {
            if (ImGui::MenuItem("TextComponent"))
            {
                TextComponent tc{};
                tc.text = "New Text";
                tc.font_size = 24.0f;
                tc.color = { 1.0f, 1.0f, 1.0f };
                tc.visible = true;
                coord->AddComponent<TextComponent>(entity, tc);
            }
        }

        // Add more components as needed...
    }

    // -------------------------------------------------------------------------
    // Local helper: parse "<rows>x<cols>" from filename ("hero_4x3.png")
    // -------------------------------------------------------------------------
    static bool ParseGridFromFilename(const std::string &full, int &outRows, int &outCols)
    {
        outRows = 1;
        outCols = 1;

        if (full.empty())
            return false;

        // grab the filename portion
        size_t slash = full.find_last_of("/\\");
        std::string filename = (slash != std::string::npos) ? full.substr(slash + 1) : full;

        // strip extension
        size_t dot = filename.find_last_of('.');
        std::string stem = (dot != std::string::npos) ? filename.substr(0, dot) : filename;

        // find an 'x' with digits on both sides, scanning from the end
        for (size_t i = stem.size(); i-- > 0;)
        {
            if (stem[i] != 'x' && stem[i] != 'X')
                continue;

            // parse left number
            size_t l = i;
            while (l > 0 && isdigit((unsigned char)stem[l - 1]))
                --l;
            if (l == i)
                continue; // no digits on the left

            // parse right number
            size_t r = i + 1;
            while (r < stem.size() && isdigit((unsigned char)stem[r]))
                ++r;
            if (r == i + 1)
                continue; // no digits on the right

            try
            {
                int rows = std::stoi(stem.substr(l, i - l));
                int cols = std::stoi(stem.substr(i + 1, r - (i + 1)));
                if (rows > 0 && cols > 0)
                {
                    outRows = rows;
                    outCols = cols;
                    return true;
                }
            }
            catch (...)
            {
                // ignore and continue searching
            }
        }
        return false;
    }

    // -------------------------------------------------------------------------
    // Helper: Draw inline remove button for component headers
    // -------------------------------------------------------------------------
    static bool DrawInlineRemoveButton(const char *componentName)
    {
        ImGui::SameLine(ImGui::GetWindowWidth() - 60); // Changed from 35 to 60 for wider button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));

        char buttonId[64];
        snprintf(buttonId, sizeof(buttonId), "Remove##%s", componentName); // Changed from "X##Remove%s"

        bool clicked = ImGui::SmallButton(buttonId);

        ImGui::PopStyleColor(3);

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Remove %s", componentName);
        }

        return clicked;
    }

    // -------------------------------------------------------------------------
    // Main Inspector window
    // -------------------------------------------------------------------------
    void DrawInspectorWindow(ImGuiID rightDockID)
    {


        // Dock on the right
        if (rightDockID)
            ImGui::SetNextWindowDockID(rightDockID, ImGuiCond_FirstUseEver);

        if (!ImGui::Begin("Inspector"))
        {
            ImGui::End();
            return;
        }

        ImGui::TextUnformatted("Entity Inspector");
        ImGui::Separator();

        auto *g = Coordinator::GetInstance();
        if (!g)
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Coordinator not found!");
            ImGui::End();
            return;
        }

        static Entity lastSelected = INVALID_ENTITY;
        if (selectedEntity != lastSelected) {
            //isEditing = false;
            //Reset flags for undo
            isEditing_Transform = false;
            isEditing_Renderable = false;
            lastSelected = selectedEntity;
        }

        if (selectedEntity == INVALID_ENTITY)
        {
            ImGui::TextUnformatted("No entity selected.");
            ImGui::End();
            return;
        }

        if (!selectedHandle) {
            selectedHandle = std::make_shared<EntityHandle>();
        }
        selectedHandle->id = selectedEntity;

        ImGui::Text("Entity ID: %d", (int)selectedEntity);

        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();

        // Create LayerTag if it doesn't exist
        if (!g->HasComponent<LayerTag>(selectedEntity))
        {
            LayerTag tag{};
            tag.mask = LAYER_WORLD;
            tag.sortingLayer = 1;  // World
            tag.orderInLayer = 0;
            tag.z = 0.0f;
            g->AddComponent<LayerTag>(selectedEntity, tag);
        }

        auto &layer = g->GetComponent<LayerTag>(selectedEntity);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.9f, 1.0f, 1.0f)); // Light blue
        ImGui::TextUnformatted("Layer:");
        ImGui::PopStyleColor();

        ImGui::SameLine();

        // Layer dropdown - with dynamic names from LayerRegistry
        auto& registry = LayerRegistry::Get();
        const auto& allLayers = registry.GetAllLayers();

        static std::vector<std::string> s_layerNameStrings;
        static std::vector<const char*> s_layerNamePtrs;

        s_layerNameStrings.clear();
        s_layerNamePtrs.clear();

        for (const auto& layerInfo : allLayers)
        {
            s_layerNameStrings.push_back(layerInfo.name);
        }
        for (const auto& name : s_layerNameStrings)
        {
            s_layerNamePtrs.push_back(name.c_str());
        }

        int currentLayerIndex = registry.GetLayerIndex(layer.mask);
        if (currentLayerIndex < 0 || currentLayerIndex >= static_cast<int>(allLayers.size()))
        {
            currentLayerIndex = 1;
        }
        //UNDO FOR LAYER
        LayerTag beforeLayerDropdown = layer;

        ImGui::SetNextItemWidth(130.0f);
        if (ImGui::Combo("##LayerDropdown", &currentLayerIndex, s_layerNamePtrs.data(), static_cast<int>(s_layerNamePtrs.size())))
        {
            if (currentLayerIndex >= 0 && currentLayerIndex < static_cast<int>(allLayers.size()))
            {
                layer.mask = allLayers[currentLayerIndex].mask;

                // Auto-sync sorting layer when mask changes
                if (layer.mask == LAYER_BACKGROUND) layer.sortingLayer = 0;
                else if (layer.mask == LAYER_WORLD) layer.sortingLayer = 1;
                else if (layer.mask == LAYER_UI) layer.sortingLayer = 2;
                else if (layer.mask == LAYER_TEXT) layer.sortingLayer = 3;
            }
        }

        if (beforeLayerDropdown != layer)
        {
            PushLayerEdit(mUndo, selectedHandle, beforeLayerDropdown, layer);
        }

        // === SORTING LAYER (for render order) ===

        ImGui::Text("Sorting Layer:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(130.0f);
        LayerTag beforeSortingLayer = layer;
        const char* sortingLayerNames[] = { "Background", "World", "UI", "Text" };
        ImGui::Combo("##SortingLayer", &layer.sortingLayer, sortingLayerNames, IM_ARRAYSIZE(sortingLayerNames));
        if (beforeSortingLayer != layer)
        {
            PushLayerEdit(mUndo, selectedHandle, beforeSortingLayer, layer);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Sorting Layer controls render order:\nBackground (0) -> World (1) -> UI (2)\nHigher = renders on top");
        }

        // === ORDER IN LAYER (fine-grained within sorting layer) ===
        ImGui::Text("Order in Layer:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150.0f);
        ImGui::DragInt("##OrderInLayer", &layer.orderInLayer, 1.0f, -9999, 9999);
        HandleLayerUndo(layer, mUndo, selectedHandle);
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Order within the sorting layer.\nHigher number = drawn on top.\nWorks across ALL layers now!");
        }

        ImGui::Separator();

        // ---------------------------------------------------------------------
        // Prefab Instance Info
        // ---------------------------------------------------------------------
        if (g->HasComponent<PrefabInstance>(selectedEntity))
        {
            auto &prefabInstance = g->GetComponent<PrefabInstance>(selectedEntity);

            // Show prefab info in a colored header (like Unity)
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.7f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

            if (ImGui::CollapsingHeader("Prefab Instance", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PopStyleColor(2);

                ImGui::Indent();

                // Show prefab name
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Prefab:");
                ImGui::SameLine();
                ImGui::Text("%s", prefabInstance.prefabName.c_str());

                // Linked checkbox
                bool isLinked = prefabInstance.isLinked;
                if (ImGui::Checkbox("Linked to Prefab", &isLinked))
                {
                    prefabInstance.isLinked = isLinked;
                    if (!isLinked)
                    {
                        std::cout << "[Inspector] Unlinked entity " << selectedEntity
                                  << " from prefab '" << prefabInstance.prefabName << "'" << std::endl;
                    }
                    else
                    {
                        std::cout << "[Inspector] Linked entity " << selectedEntity
                                  << " to prefab '" << prefabInstance.prefabName << "'" << std::endl;
                    }
                    HandlePrefabInstanceUndo(prefabInstance, mUndo, selectedHandle);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Uncheck to make this instance independent from the prefab.\n"
                                      "When unchecked, this entity won't update when the prefab changes.");
                }

                ImGui::Spacing();

                // Apply to Prefab button
                if (ImGui::Button("Apply to Prefab", ImVec2(-1, 30)))
                {
                    PrefabManager prefabMgr;

                    // Save this entity's current state as the new prefab definition
                    if (prefabMgr.SaveEntityAsPrefab(selectedEntity, prefabInstance.prefabName))
                    {
                        std::cout << "[Inspector] Saved entity " << selectedEntity
                            << " as prefab '" << prefabInstance.prefabName << "'" << std::endl;

                        // Update all instances in CURRENT scene (in memory)
                        prefabMgr.UpdateAllInstancesOfPrefab(prefabInstance.prefabName);

                        // Update all instances in OTHER scene files (on disk)
                        prefabMgr.UpdatePrefabInAllScenes(prefabInstance.prefabName);

                        std::cout << "[Inspector] Updated all instances of prefab '"
                            << prefabInstance.prefabName << "' across all scenes" << std::endl;
                    }
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Save this entity's current properties as the new prefab definition.\n"
                                      "All other instances of this prefab will update to match.\n"
                                      "Transform (position/rotation/scale) is preserved per-instance.");
                }

                ImGui::Unindent();
            }
            else
            {
                ImGui::PopStyleColor(2);
            }

            ImGui::Separator();
        }





        // ---------------------------------------------------------------------
        // Name component
        // ---------------------------------------------------------------------
        Inspector_NameComponent(g);

        // ---------------------------------------------------------------------
        // Transform component
        // ---------------------------------------------------------------------
        Inspector_TransformComponent(g);

        // ---------------------------------------------------------------------
        // Renderable component
        // ---------------------------------------------------------------------
        Inspector_RenderableComponent(g);

        // ---------------------------------------------------------------------
        // Animation component
        // ---------------------------------------------------------------------
        Inspector_AnimationComponent(g);

        // ---------------------------------------------------------------------
        // Script component
        // ---------------------------------------------------------------------
        Inspector_LogicComponent(g);

        // ---------------------------------------------------------------------
        // Audio component
        // ---------------------------------------------------------------------
        Inspector_AudioComponent(g);

        // ---------------------------------------------------------------------
        // Text component
        // ---------------------------------------------------------------------
        Inspector_TextComponentUI(g);

        // ---------------------------------------------------------------------
        // Health component
        // ---------------------------------------------------------------------
        Inspector_HealthComponent(g);

        // ---------------------------------------------------------------------
        // AABB component
        // ---------------------------------------------------------------------
        Inspector_AABBComponent(g);

        // =====================================================================
        // "add component" button
        // =====================================================================
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Add New Component", ImVec2(-1, 30)))
        {
            ImGui::OpenPopup("AddComponentPopup");
        }

        if (ImGui::BeginPopup("AddComponentPopup"))
        {
            DrawAddComponentMenu(g, selectedEntity);
            ImGui::EndPopup();
        }

        ImGui::Spacing();

        ImGui::End();

    } // end of function

    // -------------------------------------------------------------------------
    // Inspector Components
    // -------------------------------------------------------------------------
    void Inspector_NameComponent(Coordinator *g)
    {
        //early return
        if (!g->HasComponent<Name>(selectedEntity)){ return; }
       
        auto &nm = g->GetComponent<Name>(selectedEntity);

        bool removeName = false;
        bool nameOpen = 
            ImGui::CollapsingHeader("Name", ImGuiTreeNodeFlags_DefaultOpen 
                | ImGuiTreeNodeFlags_AllowOverlap);
        removeName = DrawInlineRemoveButton("Name");

        if (removeName) // remove name component
        {
            //Framework::Transform beforeRemove = tr;
            //PushRemoveTransform(mUndo, selectedHandle, beforeRemove);
            //g->RemoveComponent<Framework::Transform>(selectedEntity);
            //save name first
            Name beforeRemove = nm;
            PushRemoveName(mUndo, selectedHandle, beforeRemove);
            g->RemoveComponent<Name>(selectedEntity);

        }
        else if (nameOpen)
        {
            ImGui::Indent();

            static Entity lastEntity = INVALID_ENTITY;
            static char nameBuf[256] = {};

            if (lastEntity != selectedEntity)
            {
                lastEntity = selectedEntity;
                std::strncpy(nameBuf, nm.name.c_str(), sizeof(nameBuf));
                nameBuf[sizeof(nameBuf) - 1] = '\0';
                std::cout << "name buffer " << nameBuf << "\n";
            }
            else if (!isEditing_Name)
            {
                std::strncpy(nameBuf, nm.name.c_str(), sizeof(nameBuf));
                nameBuf[sizeof(nameBuf) - 1] = '\0';
            }

            //copying nm.name into namebuf every frame
            //static char nameBuf[256];
            //std::strncpy(nameBuf, nm.name.c_str(), sizeof(nameBuf));
            //nameBuf[sizeof(nameBuf) - 1] = '\0';
            if (ImGui::InputText("Entity Name", nameBuf, IM_ARRAYSIZE(nameBuf))) {//edit string
                nm.name = nameBuf;
            }

            HandleNameUndo(nm, mUndo, selectedHandle);

            ImGui::Unindent();
        }

        ImGui::Separator();
    }// end of Name component


    void Inspector_TransformComponent(Coordinator* g)
    {
        if (g->HasComponent<Framework::Transform>(selectedEntity))
        {
            auto& tr = g->GetComponent<Framework::Transform>(selectedEntity);

            bool removeTransform = false;
            bool transformOpen = ImGui::CollapsingHeader(
                "Transform",
                ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap
            );
            removeTransform = DrawInlineRemoveButton("Transform");

            if (removeTransform)
            {
                Framework::Transform beforeRemove = tr;
                PushRemoveTransform(mUndo, selectedHandle, beforeRemove);
                g->RemoveComponent<Framework::Transform>(selectedEntity);
            }
            else if (transformOpen)
            {
                ImGui::Indent();

                // --- Position ---
                float pos[2] = { tr.Pos.x, tr.Pos.y };
                ImGui::DragFloat2("Position", pos, 0.5f);
                tr.Pos.x = pos[0];
                tr.Pos.y = pos[1];
                //HandleTransformUndo(tr, mUndo, selectedEntity, g);
                HandleTransformUndo(tr, mUndo, selectedHandle);


                // --- Scale + Keep Aspect Ratio (Option A: changing both updates ratio) ---
                static std::unordered_map<std::uint32_t, bool>  s_keepAspect;
                static std::unordered_map<std::uint32_t, float> s_aspectXY; // ratio = X / Y

                const std::uint32_t entKey = static_cast<std::uint32_t>(selectedEntity);

                // capture old values before editing
                float oldX = tr.Scale.x;
                float oldY = tr.Scale.y;

                bool& keepAspect = s_keepAspect[entKey];

                if (ImGui::Checkbox("Keep Aspect Ratio", &keepAspect))
                {
                    // when toggled ON, capture current ratio
                    if (keepAspect)
                    {
                        if (std::fabs(oldY) > 1e-6f) s_aspectXY[entKey] = oldX / oldY;
                        else                         s_aspectXY[entKey] = 1.0f;
                    }
                }

                float sc[2] = { tr.Scale.x, tr.Scale.y };
                ImGui::DragFloat2("Scale", sc, 0.5f);

                const float eps = 1e-6f;
                bool changedX = (std::fabs(sc[0] - oldX) > eps);
                bool changedY = (std::fabs(sc[1] - oldY) > eps);

                if (keepAspect)
                {
                    float ratio = (s_aspectXY.count(entKey) ? s_aspectXY[entKey] : 1.0f);
                    if (std::fabs(ratio) < eps) ratio = 1.0f;

                    if (changedX && changedY)
                    {
                        // user changed BOTH -> treat as new ratio and remember it
                        if (std::fabs(sc[1]) > eps) s_aspectXY[entKey] = sc[0] / sc[1];
                        else                        s_aspectXY[entKey] = 1.0f;
                        // do not force either axis this frame
                    }
                    else if (changedX)
                    {
                        // X changed -> update Y
                        sc[1] = sc[0] / ratio;
                    }
                    else if (changedY)
                    {
                        // Y changed -> update X
                        sc[0] = sc[1] * ratio;
                    }
                }

                tr.Scale.x = sc[0];
                tr.Scale.y = sc[1];
                //HandleTransformUndo(tr, mUndo, selectedEntity, g);
                HandleTransformUndo(tr, mUndo, selectedHandle);
                // --- Rotation (deg) ---
                ImGui::DragFloat("Rotation (deg)", &tr.Rotation.x, 1.0f, -360.f, 360.f);
                //HandleTransformUndo(tr, mUndo, selectedEntity, g);
                HandleTransformUndo(tr, mUndo, selectedHandle);
                // --- Rotation Speed ---
                ImGui::DragFloat("Rotation Speed", &tr.Rotation.y, 0.1f, -10.0f, 10.0f);
                //HandleTransformUndo(tr, mUndo, selectedEntity, g);
                HandleTransformUndo(tr, mUndo, selectedHandle);
                // --- Visible ---
                ImGui::Checkbox("Visible", &tr.isVisible);
               // HandleTransformUndo(tr, mUndo, selectedEntity, g);
                HandleTransformUndo(tr, mUndo, selectedHandle);
                ImGui::Unindent();
            }

            ImGui::Separator();
        }
    }


    void Inspector_RenderableComponent(Coordinator *g)
    {
        if (g->HasComponent<Framework::Renderable>(selectedEntity))
        {
            auto &r = g->GetComponent<Framework::Renderable>(selectedEntity);

            bool removeRenderable = false;
            bool renderableOpen = ImGui::CollapsingHeader("Renderable", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);
            removeRenderable = DrawInlineRemoveButton("Renderable");

            if (removeRenderable)
            {
                Framework::Renderable beforeRemove = r;
                PushRemoveRenderable(mUndo, selectedHandle, beforeRemove);
                g->RemoveComponent<Framework::Renderable>(selectedEntity);
            }
            else if (renderableOpen)
            {
                ImGui::Indent();

                // --- Model reference (background vs sprite) ----------------------
                {
                   // int modelIdx = static_cast<int>(r.mdl_ref);
                   // modelIdx = std::clamp(modelIdx, 0, 1);
                    int oldModelIdx = std::clamp(static_cast<int>(r.mdl_ref), 0, 1);
                    int newModelIdx = oldModelIdx;

                    const char *kModelNames[2] = {"Background Quad", "Sprite Quad"};

                    ImGui::Text("Model: %s", kModelNames[oldModelIdx]);
                    ImGui::SameLine();
                    if (ImGui::SmallButton(" - ##model"))
                        newModelIdx = std::max(0, oldModelIdx - 1);
                    ImGui::SameLine();
                    if (ImGui::SmallButton(" + ##model"))
                        newModelIdx = std::min(1, oldModelIdx + 1);

                    //==Undo Renderable
                    if (newModelIdx != oldModelIdx)
                    {
                        Framework::Renderable before = r;
                        r.mdl_ref = static_cast<float>(newModelIdx);
                        Framework::Renderable after = r;
                        PushRenderableEdit(mUndo, selectedHandle, before, after);
                    }
                }

                // --- Shader reference --------------------------------------------
                {
                    const int maxShaders = static_cast<int>(GLApp::shdrpgms.size());
                    //int shaderIdx = std::clamp(static_cast<int>(r.shd_ref), 0, std::max(0, maxShaders - 1));
                    int oldShaderIdx = std::clamp(static_cast<int>(r.shd_ref), 0, std::max(0, maxShaders - 1));
                    int newShaderIdx = oldShaderIdx;

                    auto ShaderName = [&](int i) -> const char *
                    {
                        switch (i)
                        {
                        case 0:
                            return "Test Shader";
                        case 1:
                            return "Background Shader";
                        case 2:
                            return "Sprite Shader";
                        case 3:
                            return "Font Shader";
                        default:
                            return "Shader (unnamed)";
                        }
                    };

                    ImGui::Text("Shader: %s", ShaderName(oldShaderIdx));
                    ImGui::SameLine();
                    if (ImGui::SmallButton(" - ##shader"))
                        newShaderIdx = std::max(0, oldShaderIdx - 1);
                    ImGui::SameLine();
                    if (ImGui::SmallButton(" + ##shader"))
                        newShaderIdx = std::min(std::max(0, maxShaders - 1), oldShaderIdx + 1);

                    if (newShaderIdx != oldShaderIdx)
                    {
                        Framework::Renderable before = r;
                        r.shd_ref = static_cast<float>(newShaderIdx);
                        Framework::Renderable after = r;
                        PushRenderableEdit(mUndo, selectedHandle, before, after);
                    }

                    //r.shd_ref = static_cast<float>(shaderIdx);
                }

                // --- Texture info ------------------------------------------------
                ImGui::Text("Texture ID: %u", r.textureID);

                // --- Sprite filename editing (name only) -------------------------
                {
                    const std::string &full = r.spriteName;
                    std::string dir, fileNoExt, ext;

                    if (!full.empty())
                    {
                        size_t slash = full.find_last_of("/\\");
                        std::string filename = (slash != std::string::npos) ? full.substr(slash + 1) : full;
                        size_t dot = filename.find_last_of('.');
                        if (dot != std::string::npos)
                        {
                            fileNoExt = filename.substr(0, dot);
                            ext = filename.substr(dot);
                        }
                        else
                            fileNoExt = filename;
                        if (slash != std::string::npos)
                            dir = full.substr(0, slash + 1);
                    }

                    if (!dir.empty())
                        ImGui::TextWrapped("Folder: %s", dir.c_str());

                    static char fileBuf[256];
                    std::strncpy(fileBuf, fileNoExt.c_str(), sizeof(fileBuf));
                    fileBuf[sizeof(fileBuf) - 1] = '\0';

                    if (ImGui::InputText("Sprite Name", fileBuf, IM_ARRAYSIZE(fileBuf)))
                    {
                        std::string newName = fileBuf;

                        // Strip extension if user typed one
                        size_t dot = newName.find_last_of('.');
                        if (dot != std::string::npos)
                            newName = newName.substr(0, dot);

                        if (ext.empty())
                            ext = ".png";

                        bool typedHasSlash = newName.find_first_of("/\\") != std::string::npos;
                        if (typedHasSlash)
                            r.spriteName = newName + ext;
                        else
                            r.spriteName = newName + ext;

                        r.needsTextureReload = true;
                    }

                    HandleRenderableUndo(r, mUndo, selectedHandle);
                }

                // --- Reload texture button + drag & drop target ------------------
                {
                    bool reloadClicked = ImGui::Button("Reload Texture");

                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_TEXTURE"))
                        {
                            const char* droppedPath = static_cast<const char*>(payload->Data);
                            if (droppedPath)
                            {
                                namespace fs = std::filesystem;
                                std::string assetID = fs::path(droppedPath).stem().string();
                                r.spriteName = assetID;
                            }
                            reloadClicked = true;
                        }
                        ImGui::EndDragDropTarget();
                    }

                    if (reloadClicked)
                    {
                        // ═══════════════════════════════════════════════════════════════
                        // UNDO: Snapshot BEFORE texture change
                        // ═══════════════════════════════════════════════════════════════
                        //Entity e = selectedEntity;
                        //EntitySnapshot before = MakeEntitySnapshot(e);
                        //mUndo.Push([e, before]() {
                        //    RestoreEntityFromSnapshot(e, before);
                        //});
                        Framework::Renderable before = r;
                        namespace fs = std::filesystem;
                        const std::string kTexturesRoot = (FilePathToGame / "Assets" / "Textures").string();

                        constexpr int SPRITE_SHADER_INDEX = 1;
                        constexpr int SPRITESHEET_SHADER_INDEX = 2;

                        {
                            std::string candidate = r.spriteName;
                            std::error_code ec;

                            if ((fs::path(candidate).has_parent_path() || fs::path(candidate).is_absolute()) &&
                                fs::exists(candidate, ec))
                            {
                            }
                            else
                            {
                                std::string base = fs::path(candidate).filename().string();
                                if (base.empty())
                                    base = candidate;

                                const fs::path roots[] = {
                                    fs::path(kTexturesRoot),
                                    FilePathToGame / "Assets"};

                                std::string resolved;
                                for (const fs::path &root : roots)
                                {
                                    fs::path rootAbs = fs::weakly_canonical(root, ec);
                                    if (ec || rootAbs.empty() || !fs::exists(rootAbs, ec))
                                        continue;

                                    for (fs::recursive_directory_iterator it(rootAbs, ec), end; it != end && !ec; ++it)
                                    {
                                        if (!it->is_regular_file(ec))
                                            continue;
                                        if (it->path().filename().string() == base)
                                        {
                                            resolved = it->path().lexically_normal().string();
                                            break;
                                        }
                                    }
                                    if (!resolved.empty())
                                        break;
                                }

                                if (!resolved.empty())
                                {
                                    std::string pathStr = resolved;
                                    std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

                                    size_t myEnginePos = pathStr.find(FilePathToGame.filename().string() + "/Assets");
                                    if (myEnginePos != std::string::npos)
                                    {
                                        r.spriteName = "../../" + pathStr.substr(myEnginePos);
                                    }
                                    else
                                    {
                                        r.spriteName = pathStr;
                                    }
                                }
                            }
                        }

                        // Evict old cached version so it's re-loaded from disk
                        mAssets.EvictTexture(r.spriteName);
                        r.textureID = mAssets.GetOrLoadTexture(r.spriteName);
                        r.loadedSpriteName = r.spriteName;
                        r.needsTextureReload = false;

                        int hintRows = 1, hintCols = 1;
                        bool isSheet = false;

                        {
                            int rr = 1, cc = 1;
                            std::string base = fs::path(r.spriteName).filename().string();
                            if (ParseGridFromFilename(base, rr, cc) && rr * cc > 1)
                            {
                                hintRows = rr;
                                hintCols = cc;
                                isSheet = true;
                            }
                        }

                        if (!isSheet)
                        {
                            std::string base = fs::path(r.spriteName).filename().string();
                            for (auto &c : base)
                                c = (char)tolower(c);
                            if (base.find("sheet") != std::string::npos ||
                                base.find("atlas") != std::string::npos)
                                isSheet = true;
                        }

                        r.shd_ref = static_cast<float>(isSheet ? SPRITESHEET_SHADER_INDEX : SPRITE_SHADER_INDEX);

                        Framework::Renderable after = r;
                        PushRenderableEdit(mUndo, selectedHandle, before, after);





                    } // end ofreload clicked 
                }

                ImGui::Unindent();
            }

            ImGui::Separator();
        }
    }
    void Inspector_AnimationComponent(Coordinator *g)
    {
        if (g->HasComponent<Framework::Animation>(selectedEntity))
        {
            auto &a = g->GetComponent<Framework::Animation>(selectedEntity);

            bool removeAnimation = false;
            bool animationOpen = ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);
            removeAnimation = DrawInlineRemoveButton("Animation");

            if (removeAnimation)
            {
                //Framework::Renderable beforeRemove = r;
                //PushRemoveRenderable(mUndo, selectedHandle, beforeRemove);
                //g->RemoveComponent<Framework::Renderable>(selectedEntity);
                Framework::Animation beforeRemove = a;
                PushRemoveAnimation(mUndo, selectedHandle, a);
                g->RemoveComponent<Framework::Animation>(selectedEntity);

            }
            else if (animationOpen)
            {
                ImGui::Indent();

                auto StepperInt = [&](const char* label, int* v, int minv, int maxv, int step = 1)
                    {
                        ImGui::PushID(label);
                        ImGui::SetNextItemWidth(120.0f);

                        // Disable ImGui built-in +/- buttons
                        bool changed = ImGui::InputInt("##value", v, 0, 0);

                        ImGui::SameLine();
                        if (ImGui::SmallButton("-"))
                        {
                            *v = std::max(minv, *v - step);
                            changed = true;
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("+"))
                        {
                            *v = std::min(maxv, *v + step);
                            changed = true;
                        }
                        ImGui::SameLine();
                        ImGui::TextUnformatted(label);

                        ImGui::PopID();
                        return changed;
                    };

                auto StepperFloat = [&](const char* label, float* v, float minv, float maxv, float step = 0.1f)
                    {
                        ImGui::PushID(label);
                        ImGui::SetNextItemWidth(120.0f);

                        // Disable ImGui built-in +/- buttons (step args are for arrows)
                        bool changed = ImGui::InputFloat("##value", v, 0.0f, 0.0f, "%.1f");

                        ImGui::SameLine();
                        if (ImGui::SmallButton("-"))
                        {
                            *v = std::max(minv, *v - step);
                            changed = true;
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("+"))
                        {
                            *v = std::min(maxv, *v + step);
                            changed = true;
                        }
                        ImGui::SameLine();
                        ImGui::TextUnformatted(label);

                        ImGui::PopID();
                        return changed;
                    };

                // Rows
                {
                    Framework::Animation before = a;

                    int rows = a.rows;
                    if (StepperInt("Rows", &rows, 1, 999))
                    {
                        a.rows = std::max(1, rows);
                        a.totalFrames = std::max(1, a.rows * a.columns);
                        a.currentFrame = std::min(a.currentFrame, a.totalFrames - 1);
                        a.elapsedTime = 0.0f;

                        HandleAnimationUndo(mUndo, selectedHandle, before, a);
                    }
                }

                // Columns
                {
                    Framework::Animation before = a;

                    int cols = a.columns;
                    if (StepperInt("Columns", &cols, 1, 999))
                    {
                        a.columns = std::max(1, cols);
                        a.totalFrames = std::max(1, a.rows * a.columns);
                        a.currentFrame = std::min(a.currentFrame, a.totalFrames - 1);
                        a.elapsedTime = 0.0f;

                        HandleAnimationUndo(mUndo, selectedHandle, before, a);
                    }
                }

                // Total Frames 
                {
                    Framework::Animation before = a;

                    const int maxFrames = std::max(1, a.rows * a.columns);
                    int frames = a.totalFrames;

                    if (StepperInt("Total Frames", &frames, 1, maxFrames))
                    {
                        a.totalFrames = std::clamp(frames, 1, maxFrames);
                        a.currentFrame = std::clamp(a.currentFrame, 0, a.totalFrames - 1);
                        a.elapsedTime = 0.0f;

                        HandleAnimationUndo(mUndo, selectedHandle, before, a);
                    }
                }

                // Current frame
                {
                    Framework::Animation before = a;

                    const int maxFrame = std::max(0, a.totalFrames - 1);
                    int cur = std::clamp(a.currentFrame, 0, maxFrame);
                    if (ImGui::SliderInt("Current Frame", &cur, 0, maxFrame))
                    {
                        a.currentFrame = cur;
                        a.elapsedTime = 0.0f;

                        HandleAnimationUndo(mUndo, selectedHandle, before, a);
                    }
                }

                // Animation speed
                {
                    Framework::Animation before = a;
                    float sp = a.animationSpeed;
                    if (StepperFloat("Anim Speed (fps)", &sp, 0.0f, 120.0f, 0.1f)) {
                        a.animationSpeed = std::max(0.0f, sp);
                        HandleAnimationUndo(mUndo, selectedHandle, before, a);
                    }
                }

                ImGui::Unindent();
            }

            ImGui::Separator();
        }
    }
    void Inspector_LogicComponent(Coordinator* g)
    {
        if (g->HasComponent<LogicComponent>(selectedEntity))
        {
            auto& script = g->GetComponent<LogicComponent>(selectedEntity);

            bool removeScript = false;
            bool scriptOpen = ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);
            removeScript = DrawInlineRemoveButton("Script");

            if (removeScript)
            {
                LogicComponent beforeRemove = script;
                PushRemoveLogic(mUndo, selectedHandle, beforeRemove);
                g->RemoveComponent<LogicComponent>(selectedEntity);
            }
            else if (scriptOpen)
            {
                ImGui::Indent();

                static std::vector<std::string> scriptOptions;
                static bool scriptsLoaded = false;

                if (!scriptsLoaded)
                {
                    auto getScriptsFunc = Engine->GetFunctionPtr<const char* (*)(void)>(
                        "ScriptAPI",
                        "ScriptAPI.EngineInterface",
                        "GetAvailableScripts"
                    );

                    if (getScriptsFunc)
                    {
                        const char* result = getScriptsFunc();
                        if (result != nullptr && strlen(result) > 0)
                        {
                            std::stringstream ss(result);
                            std::string item;
                            while (std::getline(ss, item, ','))
                            {
                                if (!item.empty())
                                {
                                    item.erase(0, item.find_first_not_of(" \t\r\n"));
                                    item.erase(item.find_last_not_of(" \t\r\n") + 1);
                                    scriptOptions.push_back(item);
                                }
                            }
                            scriptsLoaded = true;
                        }
                    }
                }

                std::vector<const char*> scriptNames;
                for (auto& s : scriptOptions)
                    scriptNames.push_back(s.c_str());
                int numScripts = (int)scriptNames.size();

                // =========================================================================
                // Display ALL scripts attached
                // =========================================================================
                ImGui::Text("Scripts Attached: %zu", script.ScriptName.size());
                ImGui::Spacing();

                for (size_t i = 0; i < script.ScriptName.size(); ++i)
                {
                    ImGui::PushID((int)i);
                    ImGui::Text("%zu.", i + 1);
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "%s", script.ScriptName[i].c_str());
                    ImGui::SameLine();

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));

                    if (ImGui::SmallButton("Remove##Script"))
                    {
                        LogicComponent before = script;

                        const char* scriptToRemove = script.ScriptName[i].c_str();
                        script.ScriptName.erase(script.ScriptName.begin() + i);

                        auto removeScriptFunc = Engine->GetFunctionPtr<void(*)(int, const char*)>(
                            "ScriptAPI",
                            "ScriptAPI.EngineInterface",
                            "RemoveScriptFromEntity"
                        );
                        if (removeScriptFunc)
                            removeScriptFunc(selectedEntity, scriptToRemove);

                        LogicComponent after = script;
                        if (before != after)
                        {
                            PushLogicEdit(mUndo, selectedHandle, before, after);
                        }

                        ImGui::PopStyleColor(3);
                        ImGui::PopID();
                        continue;
                    }

                    ImGui::PopStyleColor(3);
                    ImGui::PopID();
                    ImGui::Spacing();
                }

                // =========================================================================
                // Add New Script Section
                // =========================================================================
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::Text("Add Script:");
                ImGui::SameLine();

                static char searchBuf[128] = "";
                std::vector<const char*> filteredNames;
                for (int i = 0; i < numScripts; i++)
                {
                    if (strlen(searchBuf) == 0 ||
                        std::string(scriptNames[i]).find(searchBuf) != std::string::npos)
                        filteredNames.push_back(scriptNames[i]);
                }
                int numFiltered = (int)filteredNames.size();

                static int selectedScriptIndex = 0;
                ImGui::SetNextItemWidth(250.0f);

                if (numScripts > 0)
                {
                    if (selectedScriptIndex >= numFiltered) selectedScriptIndex = 0;

                    const char* previewLabel = (numFiltered > 0 && selectedScriptIndex < numFiltered)
                        ? filteredNames[selectedScriptIndex] : "Select Script...";

                    if (ImGui::BeginCombo("##ScriptDropdown", previewLabel))
                    {
                        if (ImGui::IsWindowAppearing())
                        {
                            ImGui::SetScrollHereY(0.0f);
                            ImGui::SetKeyboardFocusHere();
                        }
                        ImGui::SetNextItemWidth(-1);
                        ImGui::InputText("##DropdownSearch", searchBuf, sizeof(searchBuf));
                        ImGui::Separator();

                        for (int i = 0; i < numFiltered; i++)
                        {
                            bool isSelected = (selectedScriptIndex == i);
                            if (ImGui::Selectable(filteredNames[i], isSelected))
                                selectedScriptIndex = i;
                            if (isSelected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }

                    ImGui::SameLine();

                    if (ImGui::SmallButton("Add"))
                    {
                        if (numFiltered > 0)
                        {
                            const char* selectedScript = filteredNames[selectedScriptIndex];

                            bool alreadyExists = false;
                            for (const auto& s : script.ScriptName)
                            {
                                if (s == selectedScript)
                                {
                                    alreadyExists = true;
                                    break;
                                }
                            }

                            if (!alreadyExists)
                            {
                                LogicComponent before = script;

                                script.ScriptName.push_back(selectedScript);

                                auto addScriptFunc = Engine->GetFunctionPtr<bool(*)(int, const char*)>(
                                    "ScriptAPI",
                                    "ScriptAPI.EngineInterface",
                                    "AddScriptViaName"
                                );
                                if (addScriptFunc)
                                {
                                    bool success = addScriptFunc(selectedEntity, selectedScript);
                                    std::cout << "[Inspector] Added script: " << selectedScript
                                        << " (Success: " << success << ")" << std::endl;
                                }

                                LogicComponent after = script;
                                if (before != after)
                                {
                                    PushLogicEdit(mUndo, selectedHandle, before, after);
                                }

                                selectedScriptIndex = 0;
                                searchBuf[0] = '\0';
                            }
                            else
                            {
                                ImGui::OpenPopup("ScriptAlreadyExists");
                            }
                        }
                    }
                }
                else
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No scripts available!");
                }

                if (ImGui::BeginPopupModal("ScriptAlreadyExists", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "This script is already attached!");
                    ImGui::Spacing();
                    if (ImGui::Button("OK", ImVec2(120, 0)))
                        ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                }

                ImGui::Spacing();

                if (script.ScriptName.empty())
                    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "No scripts attached");
                else
                    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Active Scripts: %zu attached", script.ScriptName.size());

                ImGui::Unindent();
            }

            ImGui::Separator();
        }
    }

    void Inspector_AudioComponent(Coordinator* g)
    {
        if (g->HasComponent<AudioSource>(selectedEntity))
        {
            auto& audio = g->GetComponent<AudioSource>(selectedEntity);

            bool removeAudio = false;
            bool audioOpen = ImGui::CollapsingHeader("AudioSource", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);
            removeAudio = DrawInlineRemoveButton("AudioSource");

            if (removeAudio)
            {
                AudioSource beforeRemove = audio;
                PushRemoveAudioSource(mUndo, selectedHandle, beforeRemove);
                g->RemoveComponent<AudioSource>(selectedEntity);
            }
            else if (audioOpen)
            {
                ImGui::Indent();

                ImGui::Text("Audio File:");
                ImGui::SameLine();

                auto& audioResources = mAssets.getAudioRegContainer();
                std::string preview = audio.audio_id.empty() ? "<None>" : audio.audio_id;

                static char audioSearchFilter[128] = "";


                AudioSource before = audio;

                ImGui::SetNextItemWidth(300.0f);
                if (ImGui::BeginCombo("##AudioFile", preview.c_str()))
                {
                    if (ImGui::IsWindowAppearing())
                    {
                        ImGui::SetScrollHereY(0.0f);
                        ImGui::SetKeyboardFocusHere();
                    }

                    ImGui::SetNextItemWidth(-1);
                    ImGui::InputTextWithHint("##AudioSearch", "Search audio...", audioSearchFilter, IM_ARRAYSIZE(audioSearchFilter));
                    ImGui::Separator();

                    std::string filterLower = audioSearchFilter;
                    for (auto& c : filterLower) c = static_cast<char>(tolower(c));

                    if (filterLower.empty() || std::string("<none>").find(filterLower) != std::string::npos)
                    {
                        if (ImGui::Selectable("<None>", audio.audio_id.empty()))
                        {
                            audio.audio_id = "";
                            audioSearchFilter[0] = '\0';
                        }
                    }

                    bool hasBGM = false;
                    bool hasSFX = false;

                    for (auto const& [id, resource] : audioResources)
                    {
                        std::string labelLower = id;
                        for (auto& c : labelLower) c = static_cast<char>(tolower(c));
                        std::string typeLower = resource.audiotype;
                        for (auto& c : typeLower) c = static_cast<char>(tolower(c));

                        bool matchesFilter = filterLower.empty() ||
                            labelLower.find(filterLower) != std::string::npos ||
                            typeLower.find(filterLower) != std::string::npos;

                        if (matchesFilter)
                        {
                            if (resource.audiotype == "BGM") hasBGM = true;
                            else hasSFX = true;
                        }
                    }

                    if (hasBGM)
                    {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "== BGM ==");

                        for (auto const& [id, resource] : audioResources)
                        {
                            if (resource.audiotype != "BGM") continue;

                            std::string labelLower = id;
                            for (auto& c : labelLower) c = static_cast<char>(tolower(c));

                            bool matchesFilter = filterLower.empty() ||
                                labelLower.find(filterLower) != std::string::npos;

                            if (!matchesFilter) continue;

                            bool isSelected = (audio.audio_id == id);
                            std::string label = "  " + id;

                            if (ImGui::Selectable(label.c_str(), isSelected))
                            {
                                audio.audio_id = id;
                                audioSearchFilter[0] = '\0';
                                //std::cout << "[Inspector] Selected audio: " << id << "\n";
                            }

                            if (isSelected)
                                ImGui::SetItemDefaultFocus();
                        }
                    }

                    if (hasSFX)
                    {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "== SFX ==");

                        for (auto const& [id, resource] : audioResources)
                        {
                            if (resource.audiotype == "BGM") continue;

                            std::string labelLower = id;
                            for (auto& c : labelLower) c = static_cast<char>(tolower(c));
                            std::string typeLower = resource.audiotype;
                            for (auto& c : typeLower) c = static_cast<char>(tolower(c));

                            bool matchesFilter = filterLower.empty() ||
                                labelLower.find(filterLower) != std::string::npos ||
                                typeLower.find(filterLower) != std::string::npos;

                            if (!matchesFilter) continue;

                            bool isSelected = (audio.audio_id == id);
                            std::string label = "  " + id;

                            if (ImGui::Selectable(label.c_str(), isSelected))
                            {
                                audio.audio_id = id;
                                audioSearchFilter[0] = '\0';
                                //std::cout << "[Inspector] Selected audio: " << id << "\n";
                            }

                            if (isSelected)
                                ImGui::SetItemDefaultFocus();
                        }
                    }

                    ImGui::EndCombo();
                }

                ImGui::Spacing();

                if (!audio.audio_id.empty() && mAssets.HasAudio(audio.audio_id))
                {
                    std::string path = mAssets.GetAudioPath(audio.audio_id);
                    ImGui::TextWrapped("Path: %s", path.c_str());

                    if (auto* resource = mAssets.getRegistry().FindAudio(audio.audio_id))
                    {
                        ImGui::Text("Type: %s", resource->audiotype.c_str());
                    }
                }

                if (before != audio)
                {
                    PushAudioEdit(mUndo, selectedHandle, before, audio);
                }

                ImGui::Separator();

                ImGui::SetNextItemWidth(200.0f);
                ImGui::SliderFloat("Volume", &audio.volume, 0.0f, 1.0f, "%.2f"); //slide1
                HandleAudioUndo(audio, mUndo, selectedHandle);
                ImGui::Checkbox("Loop", &audio.loop); //checkbox1
                HandleAudioUndo(audio, mUndo, selectedHandle);
                ImGui::Checkbox("Play on Awake", &audio.playOnAwake); //checkbox2
                HandleAudioUndo(audio, mUndo, selectedHandle);

                ImGui::Spacing();

                if (!IsPlaying())
                {
                    ImGui::Text("Preview:");
                    ImGui::SameLine();

                    if (ImGui::SmallButton("Play"))
                    {
                        if (!audio.audio_id.empty())
                        {
                            auto audioManager = g->GetSystem<AudioManager>();
                            if (audioManager)
                            {
                                auto* resource = mAssets.getRegistry().FindAudio(audio.audio_id);
                                if (resource)
                                    audioManager->PlayFromAudioSource(audio);
                            }
                        }
                    }
                    ImGui::SameLine();

                    if (ImGui::SmallButton("Stop")) 
                    {
                        auto audioManager = g->GetSystem<AudioManager>();
                        if (audioManager)
                            audioManager->StopPlayingAllAudio();
                    }
                }
                else
                {
                    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Game is running");
                    if (audio.channel)
                    {
                        bool isChannelPlaying = false;
                        audio.channel->isPlaying(&isChannelPlaying);
                        ImGui::Text("Status: %s", isChannelPlaying ? "Playing" : "Stopped");
                    }
                }

                ImGui::Unindent();
            }

            ImGui::Separator();
        }
    }

    void Inspector_TextComponentUI(Coordinator *g)
    {
        if (!g->HasComponent<TextComponent>(selectedEntity))
            return;

        auto &txt = g->GetComponent<TextComponent>(selectedEntity);

        bool removeText = false;
        bool textOpen = ImGui::CollapsingHeader("TextComponent", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);
        removeText = DrawInlineRemoveButton("TextComponent");

        if (removeText)
        {                
            TextComponent beforeRemove = txt;
            PushRemoveText(mUndo, selectedHandle, beforeRemove);
            g->RemoveComponent<TextComponent>(selectedEntity);
        }
        else if (textOpen)
        {
            ImGui::Indent();

            // === Text Content ===
            ImGui::Text("Text:");
            ImGui::SameLine();

            static char textBuf[1024];
            std::strncpy(textBuf, txt.text.c_str(), sizeof(textBuf));
            textBuf[sizeof(textBuf) - 1] = '\0';

            ImGui::SetNextItemWidth(300.0f);
            if (ImGui::InputTextMultiline("##TextContent", textBuf, IM_ARRAYSIZE(textBuf),
                                          ImVec2(300.0f, 60.0f)))
            {
                txt.text = textBuf;
            }
            HandleTextUndo(txt, mUndo, selectedHandle);

            ImGui::Spacing();

            // === Font Selection ===
            ImGui::Text("Font:");
            ImGui::SameLine();

            auto &fontResources = mAssets.getRegistry().getFontContainer();
            std::string fontPreview = txt.font_id.empty() ? "<Default>" : txt.font_id;

            ImGui::SetNextItemWidth(250.0f);

            TextComponent beforeFont = txt;
            if (ImGui::BeginCombo("##FontSelect", fontPreview.c_str()))
            {
                // Default font option
                if (ImGui::Selectable("<Default>", txt.font_id.empty()))
                {
                    txt.font_id = "";
                    std::cout << "[Inspector] Using default font\n";
                }

                // List all registered fonts
                for (auto const &[id, fontRes] : fontResources)
                {
                    bool isSelected = (txt.font_id == id);
                    std::string label = id + " (" + std::to_string(fontRes.pixelHeight) + "px)";

                    if (ImGui::Selectable(label.c_str(), isSelected))
                    {
                        txt.font_id = id;
                        std::cout << "[Inspector] Selected font: " << id << "\n";
                    }

                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (beforeFont != txt)
            {
                PushTextEdit(mUndo, selectedHandle, beforeFont, txt);
            }

            // === Font Size ===
            ImGui::Text("Size:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(150.0f);
            ImGui::DragFloat("##FontSize", &txt.font_size, 1.0f, 8.0f, 200.0f, "%.0f px"); //font size edit
            HandleTextUndo(txt, mUndo, selectedHandle);
            // === Text Color ===
            ImGui::Text("Color:");
            ImGui::SameLine();
            float col[3] = {txt.color.r, txt.color.g, txt.color.b};
            if (ImGui::ColorEdit3("##TextColor", col)) //color edit
            {
                txt.color = {col[0], col[1], col[2]};
            }
            HandleTextUndo(txt, mUndo, selectedHandle);
            // === Offset from Transform ===
            ImGui::Text("Offset:");
            ImGui::SameLine();
            float offset[2] = {txt.offset.x, txt.offset.y};
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::DragFloat2("##TextOffset", offset, 1.0f)) // slider
            {
                txt.offset = {offset[0], offset[1]};
            }
            HandleTextUndo(txt, mUndo, selectedHandle);
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Offset from the entity's Transform position.\nUseful for positioning text relative to a sprite.");
            }

            // === Text Alignment ===
            ImGui::Text("Alignment:");
            ImGui::SameLine();
            TextComponent beforeAlign = txt;
            const char* alignmentOptions[] = { "Left", "Center", "Right" };
            int currentAlignment = static_cast<int>(txt.alignment);

            ImGui::SetNextItemWidth(150.0f);
            if (ImGui::Combo("##TextAlignment", &currentAlignment, alignmentOptions, IM_ARRAYSIZE(alignmentOptions)))
            {
                txt.alignment = static_cast<TextAlignment>(currentAlignment);
            }
            if (beforeAlign != txt)
            {
                PushTextEdit(mUndo, selectedHandle, beforeAlign, txt);
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Left: Text starts at position\nCenter: Text centered on position\nRight: Text ends at position");
            }

            // === Visibility Toggle ===
            ImGui::Checkbox("Visible##TextComponent", &txt.visible);
            HandleTextUndo(txt, mUndo, selectedHandle);
            ImGui::Spacing();
            ImGui::Separator();

            // === Preview Info ===
            if (!txt.text.empty())
            {
                ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Preview:");
                ImGui::TextWrapped("%s", txt.text.c_str());
            }

            ImGui::Unindent();
        }

        ImGui::Separator();
    }

    void Inspector_HealthComponent(Coordinator* g)
    {
        if (!g->HasComponent<Health>(selectedEntity))
            return;

        auto& health = g->GetComponent<Health>(selectedEntity);

        bool removeHealth = false;
        bool healthOpen = ImGui::CollapsingHeader("Health", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);
        removeHealth = DrawInlineRemoveButton("Health");

        if (removeHealth)
        {

            Health beforeRemove = health;
            PushRemoveHealth(mUndo, selectedHandle, beforeRemove);
            g->RemoveComponent<Health>(selectedEntity);
        }
        else if (healthOpen)
        {
            ImGui::Indent();

            ImGui::Text("HP:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(150.0f);
            ImGui::DragInt("##HP", &health.hp, 1.0f, 0, 10000); // slide1
            HandleHealthUndo(health, mUndo, selectedHandle);
            ImGui::Text("Damage:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(150.0f);
            ImGui::DragInt("##Damage", &health.damage, 1.0f, 0, 1000); //slide2
            HandleHealthUndo(health, mUndo, selectedHandle);
            ImGui::Checkbox("Is Alive", &health.isAlive); //checkbox1
            HandleHealthUndo(health, mUndo, selectedHandle);
            ImGui::Unindent();
        }

        ImGui::Separator();
    }

    void Inspector_AABBComponent(Coordinator* g)
    {
        if (!g->HasComponent<AABB>(selectedEntity))
            return;

        auto& aabb = g->GetComponent<AABB>(selectedEntity);

        bool removeAABB = false;
        bool aabbOpen = ImGui::CollapsingHeader("AABB", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);
        removeAABB = DrawInlineRemoveButton("AABB");

        if (removeAABB)
        {
            AABB beforeRemove = aabb;
            PushRemoveAABB(mUndo, selectedHandle, beforeRemove);
            g->RemoveComponent<AABB>(selectedEntity);
        }
        else if (aabbOpen)
        {
            ImGui::Indent();

            ImGui::Text("Min:");
            ImGui::SameLine();
            float min[2] = { aabb.min.x, aabb.min.y };
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::DragFloat2("##AABBMin", min, 0.1f))
            {
                aabb.min = { min[0], min[1] };
            }
            HandleAABBUndo(aabb, mUndo, selectedHandle);

            ImGui::Text("Max:");
            ImGui::SameLine();
            float max[2] = { aabb.max.x, aabb.max.y };
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::DragFloat2("##AABBMax", max, 0.1f))
            {
                aabb.max = { max[0], max[1] };
            }
            HandleAABBUndo(aabb, mUndo, selectedHandle);

            ImGui::Spacing();

            float sizeX = aabb.max.x - aabb.min.x;
            float sizeY = aabb.max.y - aabb.min.y;
            float centerX = (aabb.min.x + aabb.max.x) * 0.5f;
            float centerY = (aabb.min.y + aabb.max.y) * 0.5f;

            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Size: %.2f x %.2f", sizeX, sizeY);
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Center Offset: %.2f, %.2f", centerX, centerY);

            ImGui::Unindent();
        }

        ImGui::Separator();
    }

} // namespace PulseEditor
