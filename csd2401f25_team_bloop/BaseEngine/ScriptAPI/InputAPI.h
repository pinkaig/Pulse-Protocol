/******************************************************************************/
/**
* @file        InputAPI.h
* @project     Pulse Protocol
* @author      Chloe Lau Rey En - (95%)
* @author      Goh Pin Kai - (5%)
* @brief       Wrapper for mouse & gamepad input access from C# scripts
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#pragma once

namespace ScriptAPI
{
	// Wrapper function for modifier and accessor for MouseInput
	public value struct InputComponent
	{
	public:
		property float MouseX
		{
			float get();
		}

		property float MouseY
		{
			float get();
		}

		property float WorldMouseX
		{
			float get();
		}

		property float WorldMouseY
		{
			float get();
		}

		property bool InGameViewport
		{
			bool get();
		}

		// FOR MOUSE
		bool IsMouseButtonPressed(int button);
		bool IsMouseButtonTriggered(int button);
		bool IsMouseButtonReleased(int button);

		// FOR INPUT (W, A, S, D)
		bool IsKeyPressed(int keyCode);
		bool IsKeyTriggered(int keyCode);
		bool IsKeyReleased(int keyCode);

		// FOR GAMEPAD/CONTROLLER (PS4 via Bluetooth)
		// Button indices match GLFW_GAMEPAD_BUTTON_* constants:
		//   0 = Cross(A), 1 = Circle(B), 2 = Square(X), 3 = Triangle(Y)
		//   4 = L1, 5 = R1, 6 = Select/Share, 7 = Options/Start
		//   11 = D-pad Up, 12 = D-pad Right, 13 = D-pad Down, 14 = D-pad Left
		property bool IsGamepadConnected { bool get(); }
		bool IsGamepadButtonPressed(int button);
		bool IsGamepadButtonTriggered(int button);
		bool IsGamepadButtonReleased(int button);
		float GetGamepadAxis(int axis);
		void RumbleGamepad(float leftMotor, float rightMotor, float durationSec);

	internal:
		//Constructor when GetInputComponent() with Entity ID gets called
		InputComponent(unsigned int ID);
	private:
		// the ID that this instance has
		unsigned int entityID;
	};
}