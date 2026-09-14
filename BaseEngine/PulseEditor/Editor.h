/******************************************************************************/
/**
 * @file        Editor.h
 * @project     Pulse Protocol
 * @author      Leu Jun Yong (primary) - 98%
 * @author      Goh Pin Kai (secondary) - 2%
 * @brief       Initializes, updates, renders, and manages the ImGui
 *              editor interface with toggle support.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once

#include "../../imgui/imgui.h"
#include "../../imgui/backends/imgui_impl_glfw.h"
#include "../../imgui/backends/imgui_impl_opengl3.h"
#include "pch/pch_temp.h"
#include "Gizmo.h"
#include "ProjectPanel.h"
#include "Hierarchy.h"
#include "Inspector.h"
#include "TopBarPanel.h"
#include "BuildSizeAnalyzer.h"
#include "DebugConsole.h"
#include "ProfilingTab.h"
#include "LayerManagerPanel.h"
#include "ConfigPanel.h"

#include "CoreEngine/Factory/Factory.h" // for creating/deleting entities
#include "CoreEngine/Prefab/PrefabManager.h"
#include "CoreEngine/ECS/System.h"   // For Entity type
#include "Components/DisplayName.h"     // For display name in hierarchy list
#include "Graphics/glapp.h"
#include "Math/math.h"
#include "oabenchmark.h"
#include "ObjectAllocator/ObjectAllocator.h"

#include "UndoManager.h" //FOR UNDO
#include "EditorTypes.h"
struct GLFWwindow;


namespace PulseEditor {
    void Init(GLFWwindow* window);
    void NewFrame();
    void Draw();
    void Render();
    void Shutdown();
    bool IsPlaying();
    void SetPlaying(bool p);

    void EnqueueExternalDrop(const std::vector<std::string>& paths);
    void PushOSDropPath(const char* path);
    void Editor_FrameCallback();
    
    // Returns the middle "game" rectangle in FRAMEBUFFER PIXELS, computed from current panel sizes
    GameViewport ComputeMiddleGameViewportPixels(GLFWwindow* window, bool keep_16x9 = true);
    // update cursor
    static void UpdateImGuiCursor(GLFWwindow* window);

    struct GizmoState {
        bool  dragging = false;
        Entity active = INVALID_ENTITY;
        glm::vec2 dragStartMouseScreen{ 0,0 }; // ImGui (screen) px
        Vector2  dragStartWorld{ 0,0 };        // world coords at press
        Vector2  entityStartPos{ 0,0 };        // starting entity world pos
    };


    struct AssetItem
    {
        std::string name;        // just the file/folder name
        std::string fullPath;    // absolute-ish path (../../PulseEngine/Assets/...)
        bool isDir = false;

        // thumbnail stuff (for files)
        unsigned int texID = 0;
        bool triedLoad = false; // so we don't spam-load every frame
    };


}// end of namespace PulseEditor
