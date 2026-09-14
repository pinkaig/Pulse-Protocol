/******************************************************************************/
/**
 * @file        InputManager.h
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En - 85%
 * @author      Goh Pin Kai - 10%
 * @author      Reginald Lew Yee Ren - 5%
 * @brief       Raw input system which handles just the tracking of
				keyboard and mouse state tracking.
 *              Separated from game logic - just reads input states.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#pragma once
#include "pch/pch_temp.h"

#include "Graphics/glhelper.h"
#include "CoreEngine/Core/ImportExport.h"
#include "CoreEngine/ECS/System.h"
#include "CoreEngine/Message/MessageManager.h"

class DLL_API InputManager : public Systems {
public:
	InputManager();
	~InputManager();

	void Initialize(GLFWwindow* window);
	void Update(float);
	void Shutdown();

	// -------------------------------------------------
	// Keyboard: 3 states [IsPressed, IsTriggered, IsReleased]
	// -------------------------------------------------
	bool isKeyTriggered(int key) const;
	bool isKeyReleased(int key) const;
	bool isKeyPressed(int key) const;

	// -------------------------------------------------
	// Mouse: 3 states [IsPressed, IsTriggered, IsReleased] + Getting pos of mouse
	// -------------------------------------------------
	bool isMouseButtonPressed(int button) const;
	bool isMouseButtonTriggered(int button) const;
	bool isMouseButtonReleased(int button) const;


	glm::vec2 getMousePosition() const;
	glm::vec2 getMouseWorld() const { return mousePositionWorld; }
	glm::vec2 getMousePositionVirtual() const { return mousePositionVirtual; }

	bool isMouseInGameViewport() const { return mouseInGameViewport; }


	unsigned int getLastCharTyped() const { return lastCharTyped; }
	void pushChar(unsigned int c) { lastCharTyped = c; }
	void clearLastChar() { lastCharTyped = 0; }

	// -------------------------------------------------
	// Debug toggles
	// -------------------------------------------------
	bool isDebugRectsEnabled() const { return debugRectsEnabled; }
	bool isDebugLinesEnabled() const { return debugLinesEnabled; }
	bool isDebugCirclesEnabled() const { return debugCirclesEnabled; }
	bool isDebugCirclesFilledEnabled() const { return debugCirclesFilledEnabled; }
	bool isDebugPointsEnabled() const { return debugPointsEnabled; }
	bool isDebugGridEnabled() const { return debugGridEnabled; }
	bool isDebugCollisionEnabled() const { return debugCollisionEnabled; }
	bool ConsumeStressToggleRequest() {
		bool out = stressToggleRequested;
		stressToggleRequested = false;
		return out;
	}
	void SetWindow(GLFWwindow* w) { window = w; }
	std::set<Entity> DebugEntities;

	//======================= FOR C# ACCESS ================================
	float GetMousePositionX() const { return mousePosition.x; }
	float GetMousePositionY() const { return mousePosition.y; }
	float GetMouseWorldPositionX() const;
	float GetMouseWorldPositionY() const;
	bool GetMouseInGameViewport() const;

	// -------------------------------------------------
	// Gamepad/Controller: 3 states + axes
	// PS4 via Bluetooth uses the standard SDL gamepad layout
	// -------------------------------------------------
	bool isGamepadConnected() const { return gamepadConnected; }
	bool isGamepadButtonPressed(int button) const;
	bool isGamepadButtonTriggered(int button) const;
	bool isGamepadButtonReleased(int button) const;
	float getGamepadAxis(int axis) const;
	void rumbleGamepad(float leftMotor, float rightMotor, float durationSec);

private:
	GLFWwindow* window = nullptr;
	unsigned int lastCharTyped = 0;

	// Keyboard state
	bool currentKeys[GLFW_KEY_LAST + 1];
	bool previousKeys[GLFW_KEY_LAST + 1];
	bool keyRepeat[GLFW_KEY_LAST + 1] = { false };
	bool isKeyRepeating(int key) const { return keyRepeat[key]; }


	// Mouse state
	bool currentMouseButtons[8];
	bool previousMouseButtons[8];
	glm::vec2 mousePosition;

	glm::vec2 mousePositionVirtual{ 0.0f };    // 0..VIRTUAL_W/H
	glm::vec2 mousePositionWorld{ 0.0f };
	bool      mouseInGameViewport{ false };    // inside letterboxed viewport?

	// Debug
	bool debugRectsEnabled = false;
	bool debugLinesEnabled = false;
	bool debugCirclesEnabled = false;
	bool debugCirclesFilledEnabled = false;
	bool debugPointsEnabled = false;
	bool debugGridEnabled = false;
	bool debugToggleKeyPressed[7] = { false };
	bool stressToggleRequested = false;
	bool debugCollisionEnabled = false;

	// Gamepad state (GLFW_GAMEPAD_BUTTON_LAST = 14)
	GLFWgamepadstate currentGamepad{};
	GLFWgamepadstate previousGamepad{};
	bool gamepadConnected = false;
	int  gamepadJoystickID = -1;	// which GLFW_JOYSTICK_x is active

	// Virtual cursor driven by gamepad (world-space coords, -1600..1600 x, -900..900 y)
	glm::vec2 gamepadCursorWorld{ 0.0f, 0.0f };
	bool gamepadCursorActive = false;	// true once gamepad has moved the cursor
	float lastDt = 0.016f;				// cached from Update()

	// Rumble timer
	float rumbleTimeRemaining = 0.0f;

	// for raw input functions
	void updateInputStates();
	void updateGamepadState();
	void processDebugToggles();
};