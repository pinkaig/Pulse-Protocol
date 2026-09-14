/******************************************************************************/
/**
 * @file        ProfilingTab.cpp
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En
 * @brief       Implements the ProfilingPanel UI for displaying real-time performance
 *              metrics. Shows FPS, frame time, and system statistics sorted by
 *              percentage of frame time consumed.
 * 
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#include "ProfilingTab.h"
#include "Profiling/Profiling.h"
#include "Graphics/glhelper.h"
#include <vector>
#include <algorithm>

ProfilingPanel::ProfilingPanel()
{
}

ProfilingPanel::~ProfilingPanel()
{
}

void ProfilingPanel::Draw()
{
    // Get profiling data
    auto* profiler = ProfilingManager::GetInstance();
    const auto& profiles = profiler->GetDisplayProfiles();

    ImGui::TextUnformatted("Performance Metrics:");
    ImGui::Separator();
    ImGui::Text("FPS: %.1f", GLHelper::fps);
    ImGui::Text("Frame Time: %.2f ms", profiler->GetCachedFrameTime() * 1000.0);
    ImGui::Separator();
    ImGui::TextUnformatted("System Stats (Sorted by %):");
    ImGui::Separator();

    if (!profiles.empty()) {
        // Convert to vector for sorting
        struct SystemData {
            std::string name;
            double percentage;
            double totalTime;
        };

        std::vector<SystemData> sortedSystems;
        sortedSystems.reserve(profiles.size());

        for (const auto& [systemName, profile] : profiles) {
            sortedSystems.push_back({ systemName, profile.percentage, profile.totalTime });
        }

        // Sort by percentage (highest to lowest)
        std::sort(sortedSystems.begin(), sortedSystems.end(),
            [](const SystemData& a, const SystemData& b) {
                return a.percentage > b.percentage;
            });

        // Display sorted systems
        ImGui::BeginChild("ProfilesChild", ImVec2(0, 200), true);
        for (const auto& sys : sortedSystems) {
            ImGui::Text("%s: %.2f%% (%.3f ms)",
                sys.name.c_str(),
                sys.percentage,
                sys.totalTime * 1000.0);
        }
        ImGui::EndChild();
    }
    else {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "No profiling data yet...");
    }
}