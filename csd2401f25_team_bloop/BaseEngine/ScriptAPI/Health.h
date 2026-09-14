/******************************************************************************/
/**
* @file        Health.h
* @project     Pulse Protocol
* @author      Chia Wei Xuan Rachael - 100%
* @brief       Managed wrapper struct exposing Health component data as C# properties 
*              for script-based health management and combat systems.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#pragma once

namespace ScriptAPI
{
	// Wrapper function for modifier and accessor for TransformComponent SRT
	public value struct HealthComponent
	{
	public:
		property int hp
		{
			int get();
			void set(int value);
		}

		property int damage
		{
			int get();
			void set(int value);
		}

		property bool isAlive
		{
			bool get();
			void set(bool value);
		}

	internal:
		//Constrcutor when GetTransform() with Entity ID gets called
		HealthComponent(unsigned int ID);
	private:
		// the ID that this instance has
		unsigned int entityID;
	};
}