/******************************************************************************/
/**
 * @file        oabenchmark.h
 * @project     Pulse Protocol
 * @author      Ban Kai Wei Benjamin
 * @brief       Benchmarks heap allocation versus a custom ObjectAllocator.
 *
 *              Provides APIs to measure allocation and deallocation performance
 *              for OS heap-based allocation and ObjectAllocator-based allocation.
 *              Timing results are reported in microseconds(us).
 *
 *              Intended for profiling allocator performance under repeated
 *              construction/destruction workloads.
 * 
 *              Added to improve memory based performance
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once
#include "../../PulseEngine/CoreEngine/ECS/Types.h"
#include <string>
#include <chrono>
class ObjectAllocator; // forward declaration

// -----------------------------
// Benchmark type
// -----------------------------
namespace Benchmark {
    struct Student {
        int   Age;
        long  Year;
        float GPA;
        long  ID;
        //Only for showing whether heap allocation actually okays
        // void* operator new(std::size_t size);
        // void operator delete(void* ptr) noexcept;
        //void* operator new(std::size_t, void* place) noexcept;
        //void operator delete(void*, void*) noexcept;
    };
    struct OABenchmarkResult
    {
        int        N;
        long long heap_us;
        long long oa_us;
    };
    // -----------------------------
    // OA Student benchmark API 
    // -----------------------------
    long long student_benchmark_heap_unique_ptr(int N);
    long long student_benchmark_oa_allocate_free_unique_ptr(int N, ObjectAllocator& oa);
    //// Runs the benchmark and returns results
    //// Safe to call from editor / tools / tests
    OABenchmarkResult RunOABenchmark(
        int N,
        unsigned objectsPerPage = 64,
        unsigned maxPages = 0,
        bool debugOn = false
    );
    std::string PrintOABenchmark( OABenchmarkResult const& r);
    // -----------------------------
    // timing helper, time in microsconds(us)
    // -----------------------------
    template <typename T>
    inline long long time_us(T&& fn, int trials = 7)
    {
        //init huge number
        long long best = (1LL << 62);
        for (int t = 0; t < trials; ++t)
        {
            auto start = std::chrono::steady_clock::now();
            fn();
            auto end = std::chrono::steady_clock::now();
            //convert duration to microsec
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            if (us < best) { best = us; } //keep best time
        }
        return best;
    }  
}//end of namespace
