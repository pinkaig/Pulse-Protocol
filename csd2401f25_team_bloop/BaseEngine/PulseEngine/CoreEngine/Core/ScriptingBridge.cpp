/******************************************************************************/
/**
 * @file        ScriptingBridge.cpp
 * @project     Pulse Protocol
 * @author		Chia Wei Xuan Rachael - 100%
 * @brief		Bridge between C++ and C# scripting.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#include "CoreEngine.h"

#include "Components/Health.h"
#include "ScriptingBridge.h"
#include "Graphics/Transform.h"
#include "GameLogic/GameLogic.h"
#include "CoreEngine/ECS/Coordinator.h"
#include "Audio/AudioSource.h"

Framework::Transform* ScriptBridge::GetTransformComponent(int ID)
{
	auto g_coordinator = Coordinator::GetInstance();
	return &g_coordinator->GetComponent <Framework::Transform>(ID);
}

Health* ScriptBridge::GetHealthComponent(int ID)
{
	auto g_coordinator = Coordinator::GetInstance();
	return &g_coordinator->GetComponent<Health>(ID);
}

AudioSource* ScriptBridge::GetAudioSourceComponent(int entityID)
{
    auto g_coordinator = Coordinator::GetInstance();
    return &g_coordinator->GetComponent<AudioSource>(entityID);
}



//LogicComponent* ScriptBridge::GetLogicComponent(int ID)
//{
//	auto g_coordinator = Coordinator::GetInstance();
//	return &g_coordinator->GetComponent<LogicComponent>(ID);
//}
//
//float ScriptBridge::GetDeltaTime(float dt)
//{
//	return dt;
//}