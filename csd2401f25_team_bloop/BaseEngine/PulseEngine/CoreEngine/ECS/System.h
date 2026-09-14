/******************************************************************************/
/**
 * @file       System.h
 * @project    Pulse Protocol
 * @author     Chia Wei Xuan Rachael
 * @brief      Base class for all ECS systems that provides a container of
 *             entities matching the system's component signature. Defines
 *             virtual Init() and Update() methods for derived systems to
 *             implement their specific logic EXP: Graphics, Physics, Input
 * 
 * @copyright  Copyright (C) 2026 DigiPen Institute of Technology.
 *             Reproduction or disclosure of this file or its contents without the
 *             prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once

#include "pch/pch_temp.h"
#include "Types.h"

//for .NET
#include "CoreEngine/Core/ImportExport.h" // for DLL_API

// stupid warning just saying to make sure we are using same compilier type and runtime MDd. WHICH WE ARE SO WHY WARNING STILL??????????
#pragma warning(push)
#pragma warning(disable: 4251)

// Each system can inherit from this class which allows the System Manager
// to keep a list of pointers to systems.
// exp: auto const& entity : EntityMember
// auto& rigidBody = GetComponent<RigidBody>(entity);

// Will be our base class. Exp: Graphics, Physics...
class DLL_API Systems
{
public:
	// functions that all of the system will have.


	// Only contains entities that have ALL the components this system needs
	// We do this using the signatures 
	std::set<Entity> EntityMember;

	// All system can have a init() and update(), just need to override
	virtual void Init() {};
	virtual void Update(float) {};
	virtual void Exit() {};
};

#pragma warning(pop)