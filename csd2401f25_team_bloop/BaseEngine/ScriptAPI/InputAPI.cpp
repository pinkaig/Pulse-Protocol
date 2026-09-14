/******************************************************************************/
/**
* @file        InputAPI.cpp
* @project     Pulse Protocol
* @author      Chloe Lau Rey En - (95%)
* @author      Goh Pin Kai - (5%)
* @brief       Implementation of mouse & gamepad input wrapper for C# scripts
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#pragma once
#define generic generic_workaround
#include "InputAPI.h"
#include "CoreEngine/Core/ScriptingBridge.h"
#include "Input/InputManager.h"
#include "Graphics/Transform.h"
#include "../CoreEngine/ECS/Coordinator.h"
#undef generic

namespace ScriptAPI
{
	// constructor for each instance, Param: ECS entity ID. We make both ECS and script system use same ID.
	InputComponent::InputComponent(unsigned int ID) : entityID(ID) {}

	float InputComponent::MouseX::get()
	{
		auto inputMgr = Coordinator::GetInstance()->GetSystem<InputManager>();
		if (!inputMgr) return 0.0f;
		return inputMgr->GetMousePositionX();
	}

	float InputComponent::MouseY::get()
	{
		auto inputMgr = Coordinator::GetInstance()->GetSystem<InputManager>();
		if (!inputMgr) return 0.0f;
		return inputMgr->GetMousePositionY();
	}

	float InputComponent::WorldMouseX::get()
	{
		auto inputMgr = Coordinator::GetInstance()->GetSystem<InputManager>();
		if (!inputMgr) return 0.0f;
		return inputMgr->GetMouseWorldPositionX();
	}

	float InputComponent::WorldMouseY::get()
	{
		auto inputMgr = Coordinator::GetInstance()->GetSystem<InputManager>();
		if (!inputMgr) return 0.0f;
		return inputMgr->GetMouseWorldPositionY();
	}

	bool InputComponent::InGameViewport::get()
	{
		auto inputMgr = Coordinator::GetInstance()->GetSystem<InputManager>();
		if (!inputMgr) return false;
		return inputMgr->GetMouseInGameViewport();
	}

	// Mouse
	bool InputComponent::IsMouseButtonPressed(int button)
	{
		auto inputMgr = Coordinator::GetInstance()->GetSystem<InputManager>();
		if (!inputMgr) return false;
		return inputMgr->isMouseButtonPressed(button);
	}

	bool InputComponent::IsMouseButtonTriggered(int button)
	{
		auto inputMgr = Coordinator::GetInstance()->GetSystem<InputManager>();
		if (!inputMgr) return false;
		return inputMgr->isMouseButtonTriggered(button);
	}

	bool InputComponent::IsMouseButtonReleased(int button)
	{
		auto inputMgr = Coordinator::GetInstance()->GetSystem<InputManager>();
		if (!inputMgr) return false;
		return inputMgr->isMouseButtonReleased(button);
	}

	// Keyboard
	bool InputComponent::IsKeyPressed(int keyCode)
	{
		auto inputMgr = Coordinator::GetInstance()->GetSystem<InputManager>();
		if (!inputMgr) return false;
		return inputMgr->isKeyPressed(keyCode);
	}

	bool InputComponent::IsKeyTriggered(int keyCode)
	{
		auto inputMgr = Coordinator::GetInstance()->GetSystem<InputManager>();
		if (!inputMgr) return false;
		return inputMgr->isKeyTriggered(keyCode);
	}

	bool InputComponent::IsKeyReleased(int keyCode)
	{
		auto inputMgr = Coordinator::GetInstance()->GetSystem<InputManager>();
		if (!inputMgr) return false;
		return inputMgr->isKeyReleased(keyCode);
	}

	// Gamepad
	bool InputComponent::IsGamepadConnected::get()
	{
		auto inputMgr = Coordinator::GetInstance()->GetSystem<InputManager>();
		if (!inputMgr) return false;
		return inputMgr->isGamepadConnected();
	}

	bool InputComponent::IsGamepadButtonPressed(int button)
	{
		auto inputMgr = Coordinator::GetInstance()->GetSystem<InputManager>();
		if (!inputMgr) return false;
		return inputMgr->isGamepadButtonPressed(button);
	}

	bool InputComponent::IsGamepadButtonTriggered(int button)
	{
		auto inputMgr = Coordinator::GetInstance()->GetSystem<InputManager>();
		if (!inputMgr) return false;
		return inputMgr->isGamepadButtonTriggered(button);
	}

	bool InputComponent::IsGamepadButtonReleased(int button)
	{
		auto inputMgr = Coordinator::GetInstance()->GetSystem<InputManager>();
		if (!inputMgr) return false;
		return inputMgr->isGamepadButtonReleased(button);
	}

	float InputComponent::GetGamepadAxis(int axis)
	{
		auto inputMgr = Coordinator::GetInstance()->GetSystem<InputManager>();
		if (!inputMgr) return 0.0f;
		return inputMgr->getGamepadAxis(axis);
	}

	void InputComponent::RumbleGamepad(float leftMotor, float rightMotor, float durationSec)
	{
		auto inputMgr = Coordinator::GetInstance()->GetSystem<InputManager>();
		if (!inputMgr) return;
		inputMgr->rumbleGamepad(leftMotor, rightMotor, durationSec);
	}
}