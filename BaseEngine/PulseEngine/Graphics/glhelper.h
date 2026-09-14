/******************************************************************************/
/**
 * @file        glhelper.h
 * @project     Pulse Protocol
 * @author      Goh Pin Kai
 * @brief		Declares OpenGL helper utilities for window management,
				input handling, timing, and context setup.
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#ifndef GLHELPER_H
#define GLHELPER_H
#pragma once
/*                                                                   includes
----------------------------------------------------------------------------- */
#include "pch/pch_temp.h"
#include <string>
#include <unordered_map>
#include "CoreEngine/Core/ImportExport.h"

/*  _________________________________________________________________________ */
struct GLHelper
	/*! GLHelper structure to encapsulate initialization stuff ...
	*/
{
	static bool init(GLint width, GLint height, std::string title);
	static void cleanup();

	// callbacks ...
	static void error_cb(int error, char const* description);
	static void fbsize_cb(GLFWwindow* ptr_win, int width, int height);
	static void focus_cb(GLFWwindow* window, int focused);
	// I/O callbacks ...
	static void key_cb(GLFWwindow* pwin, int key, int scancode, int action, int mod);
	static void mousebutton_cb(GLFWwindow* pwin, int button, int action, int mod);
	static void mousescroll_cb(GLFWwindow* pwin, double xoffset, double yoffset);
	static void mousepos_cb(GLFWwindow* pwin, double xpos, double ypos);

	static void update_time(double fpsCalcInt = 1.0);
	static void print_specs();
	static void setup_event_callbacks();

	static void toggleFullScreen();

	static std::pair<double, double> getMousePosition();  // Function to retrieve mouse position

	// Static variables for storing mouse position
	static std::pair<double, double> s_mousePos;
	static std::unordered_map<int, bool> keyState;

	static DLL_API GLint width, height;
	static DLL_API GLint GameWidth, GameHeight;
	static DLL_API GLint GameOffsetX, GameOffsetY;
	//static GLint EditorWidth, EditorHeight;  // Keep the static members as they are

	static DLL_API GLdouble fps;
	static GLdouble delta_time; // time taken to complete most recent game loop
	static std::string title;
	static GLFWwindow* ptr_window;

	static GLboolean keystateU;

	static GLboolean keystateP;
	static GLboolean keystateS;
	static GLboolean leftmousebtn;
	static GLboolean rightmousebtn;
	static GLboolean keystateC;
	static GLboolean keystate1;
	static GLboolean keystate2;

	static GLboolean keyUp;
	static GLboolean keyDown;
	static GLboolean keyLeft;
	static GLboolean keyRight;

	static GLboolean keyPause;

	static double mouseX, mouseY;
	static GLboolean leftmousetriggerme;
};

static float MouseToWorldX(float m) {
	if (m < static_cast<float>(GLHelper::GameOffsetX)) {
		return -1600.f;
	}
	else if (m > (static_cast<float>(GLHelper::GameOffsetX) + static_cast<float>(GLHelper::GameWidth))) {
		return 1600.f;
	}

	float a = static_cast<float>(GLHelper::GameOffsetX), b = static_cast<float>(GLHelper::GameOffsetX + GLHelper::GameWidth), c = -1600.f, d = 1600.f;
	return c + (m - a) * (d - c) / (b - a);
}

static float MouseToWorldY(float m) {
	float emptySpace = static_cast<float>(GLHelper::height) - static_cast<float>(GLHelper::GameOffsetY) - static_cast<float>(GLHelper::GameHeight);

	if (m < emptySpace) {
		return 900.f;
	}
	else if (m > (static_cast<float>(GLHelper::height) - static_cast<float>(GLHelper::GameOffsetY))) {
		return -900.f;
	}
	float a = static_cast<float>(GLHelper::height - GLHelper::GameOffsetY), b = emptySpace, c = -900.f, d = 900.f;
	return c + (m - a) * (d - c) / (b - a);
}

static float WorldToMouseX(float w) {
	float a = -1600.f, b = 1600.f, c = static_cast<float>(GLHelper::GameOffsetX), d = static_cast<float>(GLHelper::GameOffsetX + GLHelper::GameWidth);
	return c + (w - a) * (d - c) / (b - a);
}

static float WorldToMouseY(float w) {
	float emptySpace = static_cast<float>(GLHelper::height) - static_cast<float>(GLHelper::GameOffsetY) - static_cast<float>(GLHelper::GameHeight);

	float a = -900.f, b = 900.f, c = static_cast<float>(GLHelper::height - GLHelper::GameOffsetY), d = emptySpace;
	return c + (w - a) * (d - c) / (b - a);
}

static float WorldToScreenScaleX(float s) {
	float a = 0.f, b = 3200.f, c = 0, d = static_cast<float>(GLHelper::GameWidth);
	return c + (s - a) * (d - c) / (b - a);
}

static float WorldToScreenScaleY(float s) {
	float a = 0.f, b = 1800.f, c = 0, d = static_cast<float>(GLHelper::GameHeight);
	return c + (s - a) * (d - c) / (b - a);
}

static float ScreenToWorldScaleX(float s) {
	float a = 0.f, b = static_cast<float>(GLHelper::GameWidth), c = 0.f, d = 3200.f;

	return c + (s - a) * (d - c) / (b - a);
}

static float ScreenToWorldScaleY(float s) {
	float a = 0.f, b = static_cast<float>(GLHelper::GameHeight), c = 0.f, d = 1800.f;

	return c + (s - a) * (d - c) / (b - a);
}

inline void PreventUnusedFunctions_V2() {
	MouseToWorldX(0);
	MouseToWorldY(0);
	WorldToMouseX(0);
	WorldToMouseY(0);
	WorldToScreenScaleX(0);
	WorldToScreenScaleY(0);
	ScreenToWorldScaleX(0);
	ScreenToWorldScaleY(0);
}

#endif /* GLHELPER_H */
