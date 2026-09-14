/******************************************************************************/
/**
 * @file        Profiling.cpp
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En
 * @brief       Implements the performance profiling system with high-precision
 *              timing and percentage-based reporting. Provides per-frame FPS
 *              calculation, system-level time tracking, and formatted console
 *              output for performance analysis and debugging.
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without
 *              the prior written consent of DigiPen Institute of Technology
 *              is prohibited.
 */
 /******************************************************************************/

#include "pch/pch_temp.h"
#include "Profiling.h"

ProfilingManager* ProfilingManager::instance = nullptr;

ProfilingManager* ProfilingManager::GetInstance() {
    if (!instance) {
        instance = new ProfilingManager();
    }
    return instance;
}

void ProfilingManager::StartFrame() {
    frameStartTime = std::chrono::high_resolution_clock::now();
}

void ProfilingManager::EndFrame() {
    auto frameEndTime = std::chrono::high_resolution_clock::now();
    double totalFrameTime = std::chrono::duration<double>(frameEndTime - frameStartTime).count();

    if (totalFrameTime > 0.0) {
        //double fps = 1.0 / totalFrameTime;

        // Calculate percentages
        for (auto& [name, profile] : profiles) {
            profile.percentage = (profile.totalTime / totalFrameTime) * 100.0;
        }

        // Only update display every displayUpdateInterval seconds
        timeSinceDisplay += totalFrameTime;

        if (timeSinceDisplay >= displayUpdateInterval) {
            //std::cout << "UPDATING DISPLAY NOW!" << std::endl; // DEBUG
            displayProfiles = profiles;
            cachedFrameTime = totalFrameTime;
            timeSinceDisplay = 0.0;
        }

            // Reset profiles after displaying, to make sure that it does not exceed 100
            for (auto& [name, profile] : profiles) {
                profile.totalTime = 0.0;
                profile.percentage = 0.0;
                profile.callCount = 0;
            

            
        }
    }
}

void ProfilingManager::StartSystem(const std::string& systemName) {
    startTimes[systemName] = std::chrono::high_resolution_clock::now();
}

void ProfilingManager::EndSystem(const std::string& systemName) {
    auto endTime = std::chrono::high_resolution_clock::now();
    auto it = startTimes.find(systemName);

    if (it != startTimes.end()) {
        double elapsed = std::chrono::duration<double>(endTime - it->second).count();
        profiles[systemName].totalTime += elapsed;
        profiles[systemName].callCount++;
        startTimes.erase(it);
    }
}

const std::unordered_map<std::string, ProfilingManager::SystemProfile>&
ProfilingManager::GetProfiles() const {
    return profiles;
}

void ProfilingManager::Reset() {
    if (timeSinceReset >= resetInterval) {
        for (auto& [name, profile] : profiles) {
            profile.totalTime = 0.0;
            profile.percentage = 0.0;
            profile.callCount = 0;
        }
        timeSinceReset = 0.0;
    }
}

void ProfilingManager::DestroyInstance() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}