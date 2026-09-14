/******************************************************************************/
/**
* @file        Health.cpp
* @project     Pulse Protocol
* @author      Chia Wei Xuan Rachael - 100%
* @brief	   Implementation of HealthComponent wrapper providing C# property-style 
*              access to entity health data through C++/CLI property accessors.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#pragma once
#include "EngineInterface.h"
#define generic generic_workaround
#include "Health.h"
#include "CoreEngine/Core/ScriptingBridge.h"
#include "Components/Health.h"
#include "../CoreEngine/ECS/Coordinator.h"
#undef generic

namespace ScriptAPI
{
	// constrcutro for each instance, Param: ECS entity ID. We make both ECS and script system use same ID.
	HealthComponent::HealthComponent(unsigned int ID) : entityID(ID) {}


	int HealthComponent::hp::get()
	{
		auto g_coordinator = Coordinator::GetInstance();
		auto& health = g_coordinator->GetComponent<Health>(entityID);
		return health.hp;
	}

	void HealthComponent::hp::set(int value)
	{
		auto g_coordinator = Coordinator::GetInstance();
		auto& health = g_coordinator->GetComponent<Health>(entityID);
		health.hp = value;
	}

	int HealthComponent::damage::get()
	{
		auto g_coordinator = Coordinator::GetInstance();
		auto& health = g_coordinator->GetComponent<Health>(entityID);
		return health.damage;
	}

	void HealthComponent::damage::set(int value)
	{
		auto g_coordinator = Coordinator::GetInstance();
		auto& health = g_coordinator->GetComponent<Health>(entityID);
		health.damage = value;
	}

	bool HealthComponent::isAlive::get()
	{
		auto g_coordinator = Coordinator::GetInstance();
		auto& health = g_coordinator->GetComponent<Health>(entityID);
		return health.isAlive;
	}

	void HealthComponent::isAlive::set(bool value)
	{
		auto g_coordinator = Coordinator::GetInstance();
		auto& health = g_coordinator->GetComponent<Health>(entityID);
		health.isAlive = value;
	}
}