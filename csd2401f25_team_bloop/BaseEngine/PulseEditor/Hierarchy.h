/******************************************************************************/
/**
 * @file        Hierarchy.h
 * @project     Pulse Protocol
 * @author      Leu Jun Yong
 * @brief       Scene hierarchy panel interface for entity list and management.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/

#pragma once

#include "../../imgui/imgui.h"     // for ImGuiID
#include "CoreEngine/ECS/System.h" // for Entity (same type you use in Editor)
#include "Gizmo.h"

namespace PulseEditor
{

    // Draw the full Hierarchy ImGui window (toolbar + list).
    // dockID is the dockspace ID from Editor.cpp (LeftDockSpace).
    void DrawHierarchyWindow(ImGuiID dockID);
}
