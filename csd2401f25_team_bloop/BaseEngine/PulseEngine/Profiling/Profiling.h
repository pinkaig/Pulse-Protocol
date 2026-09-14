/******************************************************************************/
/**
 * @file        Profiling.h
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En
 * @brief       Standalone performance profiling system for measuring and
 *              displaying execution times of systems as percentages of total
 *              frame time. Provides ProfilingManager for per-frame tracking
 *              and ScopedProfiler (RAII) helper for automatic profiling of
 *              code regions.
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without
 *              the prior written consent of DigiPen Institute of Technology
 *              is prohibited.
 */
 /******************************************************************************/

#pragma once
#include "pch/pch_temp.h"
// for .net
#include "CoreEngine/Core/ImportExport.h"
#include "../PulseEngine/ObjectAllocator/ObjectAllocator.h"

#pragma warning(push)
#pragma warning(disable: 4251) // stupid warning just saying to make sure we are using same compilier type and runtime MDd. WHICH WE ARE SO WHY WARNING STILL??????????


class DLL_API ProfilingManager {
public:
    struct SystemProfile {
        double totalTime = 0.0;
        double percentage = 0.0;
        int callCount = 0;
    };

    static ProfilingManager* GetInstance();
    static void DestroyInstance();

    void StartFrame();
    void EndFrame();

    void StartSystem(const std::string& systemName);
    void EndSystem(const std::string& systemName);

    const std::unordered_map<std::string, SystemProfile>& GetProfiles() const;
    const std::unordered_map<std::string, SystemProfile>& GetDisplayProfiles() const { return displayProfiles; }

    double GetCachedFrameTime() const { return cachedFrameTime; }

    void Reset();

    void SetResetInterval(double interval) { resetInterval = interval; }
    void SetDisplayUpdateInterval(double interval) { displayUpdateInterval = interval; }

private:
    ProfilingManager() = default;
    static ProfilingManager* instance;

    std::unordered_map<std::string, SystemProfile> profiles;
    std::unordered_map<std::string, SystemProfile> displayProfiles; // for ImGui
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> startTimes;

    std::chrono::high_resolution_clock::time_point frameStartTime;

    double cachedFrameTime = 0.0;

    double resetInterval = 60.0;
    double timeSinceReset = 0.0;
    double displayUpdateInterval = 1.0;
    double timeSinceDisplay = 0.0;
};

// RAII helper - use only in CoreEngine, NOT in individual systems
class ScopedProfiler {
public:
    ScopedProfiler(const std::string& systemName) : name(systemName) {
        ProfilingManager::GetInstance()->StartSystem(name);
    }
    ~ScopedProfiler() {
        ProfilingManager::GetInstance()->EndSystem(name);
    }
private:
    std::string name;
};

#define PROFILE_SCOPE(name) ScopedProfiler profiler_##__LINE__(name)
#pragma warning(pop)