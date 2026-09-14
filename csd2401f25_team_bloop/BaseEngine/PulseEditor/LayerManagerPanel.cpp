/******************************************************************************/
/**
 * @file        LayerManagerPanel.cpp
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En (primary) - 90%
 * @author      Ban Kai Wei Benjamin (secondary) - 10%
 * 
 * @brief       Layer management window using LayerRegistry for editable names
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#include "pch/pch_temp.h"       // PCH must be FIRST
#include "LayerManagerPanel.h"  // Own header second

// editor headers
#include "Editor.h"

// engine headers
#include "Graphics/LayerRegistry.h"
#include "Graphics/Layer.h"
#include "CoreEngine/ECS/Coordinator.h"
#include "Graphics/GraphicsManager.h"
#include "UndoHelper.h"

namespace PulseEditor
{
    // Track which layer is being edited (for inline text input)
    static int s_editingLayerIndex = -1;
    static char s_editBuffer[64] = "";

    // -------------------------------------------------------------------------
    // Update graphics filter when visibility changes
    // -------------------------------------------------------------------------
    void UpdateGraphicsLayerFilter()
    {
        auto* coord = Coordinator::GetInstance();
        if (!coord) return;

        auto gfxMgr = coord->GetSystem<GraphicsManager>();
        if (!gfxMgr) return;

        LayerMask visibleMask = LayerRegistry::Get().ComputeVisibleLayerMask();
        gfxMgr->SetEditorLayerFilter(visibleMask);
    }

    // -------------------------------------------------------------------------
    // Main Layer Manager Window
    // -------------------------------------------------------------------------
    void DrawLayerManagerWindow()
    {
        // LayerRegistry auto-initializes on first access (loads from layers.json if exists)

        ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin("Layer Manager", nullptr))
        {
            ImGui::End();
            return;
        }

        ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "Scene Layers");
        ImGui::Separator();
        ImGui::Spacing();

        auto* coord = Coordinator::GetInstance();
        if (!coord)
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Coordinator not found!");
            ImGui::End();
            return;
        }

        auto& registry = LayerRegistry::Get();
        auto& layers = registry.GetAllLayersMutable();

        // Count objects per layer
        std::vector<int> layerCounts(layers.size(), 0);

        for (Entity e : coord->GetAllEntities())
        {
            if (coord->HasComponent<LayerTag>(e))
            {
                auto& tag = coord->GetComponent<LayerTag>(e);
                int idx = registry.GetLayerIndex(tag.mask);
                if (idx >= 0 && idx < static_cast<int>(layerCounts.size()))
                    layerCounts[idx]++;
            }
        }

        // =====================================================================
        // Draw each layer
        // =====================================================================
        bool needsFilterUpdate = false;

        for (size_t i = 0; i < layers.size(); ++i)
        {
            auto& layer = layers[i];
            ImGui::PushID(static_cast<int>(i));

            // Visibility toggle
            bool visible = layer.visible;
            if (ImGui::Checkbox("##Visible", &visible))
            {
                auto before = registry.MakeSnapshot();
                layer.visible = visible;
                needsFilterUpdate = true;
                auto after = registry.MakeSnapshot();
                PushLayerRegistryEdit(mUndo, before, after);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Toggle visibility");

            ImGui::SameLine();

            // Lock toggle
           // ImGui::Checkbox("##Locked", &layer.locked);

            bool locked = layer.locked;
            if (ImGui::Checkbox("##Locked", &locked))
            {
                auto before = registry.MakeSnapshot();

                layer.locked = locked;

                auto after = registry.MakeSnapshot();
                PushLayerRegistryEdit(mUndo, before, after);
            }

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Lock layer (prevent selection)");

            ImGui::SameLine();

            // Layer name (editable via double-click)
            if (s_editingLayerIndex == static_cast<int>(i))
            {
                // Editing mode - show input field
                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::InputText("##EditName", s_editBuffer, sizeof(s_editBuffer),
                    ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                {
                    // Enter pressed - save the name using SetLayerName (triggers auto-save)
                    if (strlen(s_editBuffer) > 0)
                    {
                        auto before = registry.MakeSnapshot();
                        registry.SetLayerName(layer.mask, s_editBuffer);
                        auto after = registry.MakeSnapshot();
                        PushLayerRegistryEdit(mUndo, before, after);
                    }
                    s_editingLayerIndex = -1;
                }

                // Cancel on escape
                if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                {
                    s_editingLayerIndex = -1;
                }

                // Set focus on first frame
                if (ImGui::IsItemVisible() && !ImGui::IsItemActive())
                {
                    ImGui::SetKeyboardFocusHere(-1);
                }
            }
            else
            {
                // Display mode - show colored text
                ImVec4 color(layer.colorR, layer.colorG, layer.colorB, layer.colorA);
                ImGui::TextColored(color, "%s", layer.name.c_str());

                // Double-click to edit
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                {
                    s_editingLayerIndex = static_cast<int>(i);
                    strncpy(s_editBuffer, layer.name.c_str(), sizeof(s_editBuffer) - 1);
                    s_editBuffer[sizeof(s_editBuffer) - 1] = '\0';
                }

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Double-click to rename");
            }

            ImGui::SameLine();
            ImGui::TextDisabled("(%d)", layerCounts[i]);

            // Status indicators
            if (!layer.visible)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "[HIDDEN]");
            }

            if (layer.locked)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "[LOCKED]");
            }

            // Remove button for custom layers only
            if (!layer.isDefault)
            {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.15f, 0.15f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                if (ImGui::SmallButton("X"))
                {
                    auto before = registry.MakeSnapshot();

                    registry.RemoveCustomLayer(layer.mask);  // Auto-saves internally
                    needsFilterUpdate = true;

                    auto after = registry.MakeSnapshot();
                    PushLayerRegistryEdit(mUndo, before, after);

                    ImGui::PopStyleColor(2);
                    ImGui::PopID();
                    break; // Exit loop since we modified the vector
                }
                ImGui::PopStyleColor(2);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Remove this custom layer");
            }

            ImGui::PopID();
            ImGui::Spacing();
        }

        // Update renderer if visibility changed
        if (needsFilterUpdate)
        {
            UpdateGraphicsLayerFilter();
        }

        // =====================================================================
        // Summary Section
        // =====================================================================
        ImGui::Separator();
        ImGui::Spacing();

        int totalObjects = 0;
        for (int count : layerCounts)
            totalObjects += count;
        ImGui::Text("Total Objects: %d", totalObjects);
        ImGui::Text("Layer Count: %zu", layers.size());

        ImGui::Spacing();
        ImGui::TextDisabled("Double-click layer name to rename");
        ImGui::TextDisabled("Changes auto-save to Assets/Config/layers.json");

        // =====================================================================
        // Quick Actions
        // =====================================================================
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Show All"))
        {
            auto before = registry.MakeSnapshot();

            registry.ShowAllLayers();
            UpdateGraphicsLayerFilter();

            auto after = registry.MakeSnapshot();
            PushLayerRegistryEdit(mUndo, before, after);
        }

        ImGui::SameLine();

        if (ImGui::Button("Hide All"))
        {
            auto before = registry.MakeSnapshot();

            registry.HideAllLayers();
            UpdateGraphicsLayerFilter();

            auto after = registry.MakeSnapshot();
            PushLayerRegistryEdit(mUndo, before, after);
        }

        ImGui::SameLine();

        if (ImGui::Button("Unlock All"))
        {
            auto before = registry.MakeSnapshot();

            registry.UnlockAllLayers();

            auto after = registry.MakeSnapshot();
            PushLayerRegistryEdit(mUndo, before, after);
        }

        // =====================================================================
        // Add Custom Layer
        // =====================================================================
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        static char newLayerName[64] = "";
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputTextWithHint("##NewLayerName", "New layer name...", newLayerName, sizeof(newLayerName));

        ImGui::SameLine();

        if (ImGui::Button("Add Layer"))
        {
            if (strlen(newLayerName) > 0)
            {
                auto before = registry.MakeSnapshot();

                LayerMask newMask = registry.AddCustomLayer(newLayerName);  // Auto-saves internally
                if (newMask != 0)
                {
                    newLayerName[0] = '\0'; // Clear input on success
                }

                auto after = registry.MakeSnapshot();
                PushLayerRegistryEdit(mUndo, before, after);
            }
        }

        // No more Save/Load buttons - everything auto-saves!

        ImGui::End();
    }

}