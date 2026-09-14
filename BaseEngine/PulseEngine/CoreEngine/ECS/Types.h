/******************************************************************************/
/**
 * @file       Types.h
 * @project    Pulse Protocol
 * @author     Chia Wei Xuan Rachael
 * @brief      Defines core ECS type aliases and constants including Entity
 *             identifiers, ComponentType indices, maximum entity/component
 *             limits, and Signature bitsets used for system-component matching
 *             throughout the Entity-Component-System architecture
 * 
 * @copyright  Copyright (C) 2026 DigiPen Institute of Technology.
 *             Reproduction or disclosure of this file or its contents without the
 *             prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once
#include "pch/pch_temp.h"

// ECS
// JUST KNOW THAT ENTITY IS JUST A NUMBER. (1,2,3,4,5)
using Entity = std::uint32_t;

// Used to define the size of our container later on
const Entity MaxEntity = 5000;

// 2^8 type if component tyoe for now
using ComponentType = std::uint8_t;

// max amount of component per type
const ComponentType MAX_COMPONENTS = 32;

// for System ID
using Signature = std::bitset<MAX_COMPONENTS>;



