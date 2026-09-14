/******************************************************************************/
/**
* @file        TransformComponent.h
* @project     Pulse Protocol
* @author      Chia Wei Xuan Rachael - 100%
* @brief       
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
	public value struct TransformComponent
	{
	public:
		property float ScaleX
		{
			float get();
			void set(float value);
		}

		property float ScaleY
		{
			float get();
			void set(float value);
		}

		property float RotX
		{
			float get();
			void set(float value);
		}

		property float RotY
		{
			float get();
			void set(float value);
		}

		property float X
		{
			float get();
			void set(float value);
		}

		property float Y
		{
			float get();
			void set(float value);
		}

		property bool IsVisible
		{
			bool get();
			void set(bool value);
		}

	internal:
		//Constrcutor when GetTransform() with Entity ID gets called
		TransformComponent(unsigned int ID);
	private:
		// the ID that this instance has
		unsigned int entityID;
	};

}