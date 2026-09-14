/******************************************************************************/
/**
 * @file       Singleton.h
 * @project    Pulse Protocol
 * @author     Chia Wei Xuan Rachael
 * @brief      Template-based Singleton pattern implementation that ensures only
 *             one instance of a class exists throughout the application lifetime.
 *             Provides thread-safe lazy initialization and prevents multiple
 *             instantiations of templated types
 * 
 * @copyright  Copyright (C) 2026 DigiPen Institute of Technology.
 *             Reproduction or disclosure of this file or its contents without the
 *             prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once
#include "pch/pch.h"

// allows it so that once it is made, it will never create another instance of that system again. Parent of g_coordinator(included in coordinator class)
template<typename T>
class Singleton
{
public:
    // make an instance of g_coordinator
    static T* GetInstance()
    {
        // creates instance once
        static T instance; 

        return &instance; 
    }

protected:
    Singleton() {};
    virtual ~Singleton() = default;
};