/******************************************************************************/
/**
 * @file        ProfilingTab.h
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En
 * @brief       Declares the ProfilingPanel class for displaying performance metrics.
 *              Provides ImGui-based visualization of FPS, frame time, and sorted
 *              system performance statistics.
 * 
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once
#include <imgui.h>

class ProfilingPanel
{
public:
    ProfilingPanel();
    ~ProfilingPanel();

    // Render the profiling panel (matches original Editor.cpp profiler tab)
    void Draw();

private:
    // No additional state needed - just renders current profiling data
};