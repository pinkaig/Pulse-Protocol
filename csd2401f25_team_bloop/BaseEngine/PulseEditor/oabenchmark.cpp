/******************************************************************************/
/**
 * @file        oabenchmark.cpp
 * @project     Pulse Protocol
 * @author      Ban Kai Wei Benjamin
 * @brief       Implementation of allocation benchmarks 
 *                  - Uses best-of-N timing to reduce OS jitter.
 *                  - Objects are written to after construction to prevent
 *                    compiler dead-code elimination.
 *                  - Both heap and ObjectAllocator paths use std::unique_ptr
 *                    to enforce identical RAII lifetime semantics.
 *                  - ObjectAllocator uses placement new and a custom deleter
 *                    to ensure destructor and Free() are both invoked.
 *                  - Allocator initialization cost is excluded from timing.
 *              
 *              using Student data struct to showcase that the object allocator works
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#pragma message("ENGINE compiling oabenchmark.cpp")
#include "pch/pch_temp.h"
#include "../../PulseEngine/ObjectAllocator/ObjectAllocator.h"
#include "../../PulseEngine/CoreEngine/ECS/Coordinator.h"
#include "../../PulseEngine/CoreEngine/Factory/Factory.h"
#include "oabenchmark.h"
//#include <new>
//#include <cstdlib>
//#include <cstdio>
//#include <windows.h>
namespace Benchmark {
     long long student_benchmark_heap_unique_ptr(int N)
    {
        return time_us([&]() {
            //vector of unique_ptrs
            std::vector<std::unique_ptr<Student>> u_ptrs;
            u_ptrs.reserve(N);

            for (int i = 0; i < N; ++i) {
                //Student* p = new Student();   // alloc + ctor
                std::unique_ptr<Student> p = std::make_unique<Student>(); // alloc + ctor
                p->Age = i;                   // small side-effect
                //ptrs.push_back(p);      // cannot be copied
                u_ptrs.push_back(std::move(p)); // transfer ownership
            }

            u_ptrs.clear();
            });
    }

    //need to do reserve?
    long long student_benchmark_oa_allocate_free_unique_ptr(int N, ObjectAllocator& oa)
    {
        return time_us([&]() { //lambda
            std::vector<std::unique_ptr<Student, OADeleter>> u_ptrs;
            u_ptrs.reserve(N);
            for (int i = 0; i < N; ++i) {
                //void* mem = oa.Allocate();        // raw block
                //Student* p = new (mem) Student(); // placement-new ctor
                std::unique_ptr<Student, OADeleter> p = oa_make_unique<Student>(oa);
                p->Age = i;
                //ptrs.push_back(p);
                u_ptrs.push_back(std::move(p));
            }

            // for (Student* p : ptrs) {
            //   p->~Student();                    // manual dtor
            //   oa.Free(p);                       // return block to OA
            // }
            // Force destruction 
            u_ptrs.clear(); // force deleter calls now
            });
    }

    OABenchmarkResult RunOABenchmark(int N,
        unsigned objectsPerPage,
        unsigned maxPages,
        bool debugOn)
    {
        // Heap From OS
        long long heap = student_benchmark_heap_unique_ptr(N);

        // OA config (adjust constructor params to match OAConfig)
        // (UseCPPMemManager, ObjectsPerPage, MaxPages, DebugOn, PadBytes, HeaderBlocks, Alignment)
        OAConfig cfg(
            false,                     // UseCPPMemManager_
            objectsPerPage,            // ObjectsPerPage_
            maxPages,                  // MaxPages_ (0 = unlimited)
            debugOn,                   // DebugOn_
            0,                         // PadBytes_
            0,                         // HeaderBlocks_
            alignof(std::max_align_t)  // Alignment_
        );

        ObjectAllocator oa(sizeof(Student), cfg);

        // OUR OA
        long long oa_time = student_benchmark_oa_allocate_free_unique_ptr(N, oa);

        return OABenchmarkResult{ N, heap, oa_time };
    }

    // convenience printer
    std::string PrintOABenchmark( OABenchmarkResult const& r)
    {
        return
            std::string("[OA Benchmark]\n") +  
            "N = " + std::to_string(r.N) + "\n" +
            "Heap make_unique: " + std::to_string(r.heap_us) + " us\n" +
            "OA  oa_make_unique: " + std::to_string(r.oa_us) + " us\n";
    }

    /*
    * Usage
    * i f (ImGui::Button("Run OA Benchmark"))
        {
             Only N needs to be defined explicitly
            auto result = RunOABenchmark(100000);
            PrintOABenchmark(result);

            
        }
    */
    /*
    *  Overload works, but spam console not useful in telling anything
    */
    //void* Student::operator new(std::size_t size)
    //{
    //    std::printf("[Student] new %zu bytes\n", size);
    //    return ::operator new(size);
    //}

    //void Student::operator delete(void* ptr) noexcept
    //{

    //   std::printf("[Student] delete\n");
    //   ::operator delete(ptr);
    //}

    // void* Student::operator new(std::size_t, void* place) noexcept
    //{
    //    return place;
    //}

    // void Student::operator delete(void*, void*) noexcept
    //{
    //    // never called, but required by the language
    //}


     //long long particle_benchmark_heap_unique_ptr(int N)
     //{
     //    return time_us([&]() {
     //        std::vector<std::unique_ptr<Particle>> v;
     //        v.reserve(N);

     //        for (int i = 0; i < N; ++i)
     //            v.push_back(std::make_unique<Particle>());

     //        v.clear(); // destroy
     //        });
     //}

     //long long particle_benchmark_oa_unique_ptr(int N, ObjectAllocator& oa)
     //{
     //    return time_us([&]() {
     //        std::vector<std::unique_ptr<Particle, OADeleter>> v;
     //        v.reserve(N);

     //        for (int i = 0; i < N; ++i)
     //            v.push_back(oa_make_unique<Particle>(oa));

     //        v.clear(); // calls OADeleter -> Free
     //        });
     //}


}// end of namespace

// C++ has multiple delete signatures, and the compiler chooses which one to call based on
// 4 categories
// 1. Scalar delete 
//    - void operator delete(void* ptr) noexcept;
// 2. SZIED Scalar delete 
//    - void operator delete(void* ptr, std::size_t size) noexcept;
// 3. Array delete 
//    - void operator delete[](void* ptr) noexcept;
// 4. Sized array delete -> void operator delete(void* ptr) noexcept;
//    - void operator delete[](void* ptr, std::size_t size) noexcept;

// Global operator new
/*
 Constantly/perma printing stuff
 possibly due to IMGUI,logging, iostream
*/
//void* operator new(std::size_t size)
//{
//    if (size >= 256)
//    std::printf("[NEW ] %zu bytes\n", size);
//
//    if (void* p = std::malloc(size))
//        return p;
//
//    throw std::bad_alloc{};
//}
//
//// Global operator delete
//void operator delete(void* ptr) noexcept
//{
//
//    std::printf("[DEL ] %p\n", ptr);
//    std::free(ptr);
//}