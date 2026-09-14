	/******************************************************************************/
/**
* @file        Scene.h
* @project     Pulse Protocol
* @author      Chia Wei Xuan Rachael - 95%
* @author      Leu Jun Yong (secondary) - 5%
* @brief	   C++/CLI wrapper providing static scene loading functionality directly 
*              accessible to C# scripts within the ScriptAPI namespace.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#pragma once

namespace ScriptAPI
{
	// abstract sealed make it global I think? 
	public value class Scene // abstract sealed
	{
	public:
		static void LoadScene(System::String^ Filename);

		static void RestartScene();

		static System::String^ GetCurrentScene();

		static bool SceneExists(System::String^ Filename);

		// Returns true once when a scene has fully finished loading.
		static bool ConsumeSceneLoadedPulse();
	};
}
