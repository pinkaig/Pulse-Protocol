/******************************************************************************/
/**
* @file        TransformComponent.cpp
* @project     Pulse Protocol
* @author      Chia Wei Xuan Rachael - 100%
* @brief	   Accessors and Modifier for transform component that Csharp can use
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#pragma once
#define generic generic_workaround
#include "TransformComponent.h"
#include "Graphics/Transform.h"
#include "CoreEngine/Core/ScriptingBridge.h"
#include "../CoreEngine/ECS/Coordinator.h"
#undef generic

namespace Framework { class Transform; } // forward delare

namespace ScriptAPI
{
	// constrcutro for each instance, Param: ECS entity ID. We make both ECS and script system use same ID.
	TransformComponent::TransformComponent(unsigned int ID): entityID(ID) {}

	//Scale accessor and modifer
	float TransformComponent::ScaleX::get()
	{
		auto g_coordinator = Coordinator::GetInstance();
		auto& transform = g_coordinator->GetComponent<Framework::Transform>(entityID);
		return transform.Scale.x;
	}

	float TransformComponent::ScaleY::get()
	{
		auto g_coordinator = Coordinator::GetInstance();
		auto& transform = g_coordinator->GetComponent<Framework::Transform>(entityID);
		return transform.Scale.y;
	}

	void TransformComponent::ScaleX::set(float value)
	{
		auto g_coordinator = Coordinator::GetInstance();
		auto& transform = g_coordinator->GetComponent<Framework::Transform>(entityID);
		transform.Scale.x = value;
	}

	void TransformComponent::ScaleY::set(float value)
	{
		auto g_coordinator = Coordinator::GetInstance();
		auto& transform = g_coordinator->GetComponent<Framework::Transform>(entityID);
		transform.Scale.y = value;
	}


	// Rotation acessor and modifer
	float TransformComponent::RotX::get()
	{
		auto g_coordinator = Coordinator::GetInstance();
		auto& transform = g_coordinator->GetComponent<Framework::Transform>(entityID);
		return transform.Rotation.x;
	}

	void TransformComponent::RotX::set(float value)
	{
		auto g_coordinator = Coordinator::GetInstance();
		auto& transform = g_coordinator->GetComponent<Framework::Transform>(entityID);
		transform.Rotation.x = value;
	}

	float TransformComponent::RotY::get()
	{
		auto g_coordinator = Coordinator::GetInstance();
		auto& transform = g_coordinator->GetComponent<Framework::Transform>(entityID);
		return transform.Rotation.y;
	}

	void TransformComponent::RotY::set(float value)
	{
		auto g_coordinator = Coordinator::GetInstance();
		auto& transform = g_coordinator->GetComponent<Framework::Transform>(entityID);
		transform.Rotation.y = value;
	}


	// Translate accessor and modifer 
	float TransformComponent::X::get()
	{
		auto g_coordinator = Coordinator::GetInstance();
		auto& transform = g_coordinator->GetComponent<Framework::Transform>(entityID);
		return transform.Pos.x;
	}

	float TransformComponent::Y::get()
	{
		auto g_coordinator = Coordinator::GetInstance();
		auto& transform = g_coordinator->GetComponent<Framework::Transform>(entityID);
		return transform.Pos.y;
	}

	void TransformComponent::X::set(float value)
	{
		auto g_coordinator = Coordinator::GetInstance();
		auto& transform = g_coordinator->GetComponent<Framework::Transform>(entityID);
		transform.Pos.x = value;
	}

	void TransformComponent::Y::set(float value)
	{
		auto g_coordinator = Coordinator::GetInstance();
		auto& transform = g_coordinator->GetComponent<Framework::Transform>(entityID);
		transform.Pos.y = value;
	}

	bool TransformComponent::IsVisible::get()
	{
		auto g_coordinator = Coordinator::GetInstance();
		auto& transform = g_coordinator->GetComponent<Framework::Transform>(entityID);
		return transform.isVisible;
	}

	void TransformComponent::IsVisible::set(bool value)
	{
		auto g_coordinator = Coordinator::GetInstance();
		auto& transform = g_coordinator->GetComponent<Framework::Transform>(entityID);
		transform.isVisible = value;
	}
}