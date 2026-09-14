/******************************************************************************/
/**
* @file        Animation.h
* @project     Pulse Protocol
* @author      Chia Wei Xuan Rachael - 100%
* @brief	   Managed wrapper class providing C# scripts with animation control 
*              capabilities for individual entities through sprite and timing manipulation.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#pragma once

namespace ScriptAPI
{
	public value class Animation 
	{
	public:
		// Gib the entity a new sprite sheet
		void SetAnimation(System::String^ SpriteName, int rows, int columns, int TotalFrames, float speed);

		// Get the current SpriteName from this entity
		System::String^ GetCurrentAnimation();

		//change animation speed(duh)
		void SetAnimationSpeed(float speed);

		// Jump to a specific frame index (0-based) without restarting the animation
		void SetCurrentFrame(int frame);

	internal:
		//Constrcutor when GetAnimation() with Entity ID gets called
		Animation(unsigned int ID);
	private:
		// the ID that this instance has
		unsigned int entityID;
	};
}