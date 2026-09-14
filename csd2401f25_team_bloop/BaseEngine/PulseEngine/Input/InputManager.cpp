/******************************************************************************/
/**
 * @file        InputManager.cpp
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En - 55%
 * @author      Goh Pin Kai - 35%
 * @author      Reginald Lew Yee Ren - 10%
 * @brief       Implements the InputManager system, handling GLFW-based
 *              keyboard and controller(ps4) input for rhythm gameplay. Provides real-time input
 *              state tracking, combo detection, rhythm timing feedback,
 *              event broadcasting to subscribed systems, debug toggles.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#include "pch/pch_temp.h"

#include "graphics/stb_image.h"
#include "Input/InputManager.h"
#include "CoreEngine/ECS/Coordinator.h"
#include "Graphics/GraphicsManager.h"

// DS4 USB rumble via direct HID (interrupt OUT endpoint)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <setupapi.h>
#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")

// ---------------------------------------------------------------------------
// Send a DS4 USB rumble report via the interrupt OUT endpoint.
// rightMotor = high-frequency (weak), leftMotor = low-frequency (strong).
// ---------------------------------------------------------------------------
static void RumbleDS4HID(uint8_t rightMotor, uint8_t leftMotor)
{
	GUID hidGuid;
	HidD_GetHidGuid(&hidGuid);

	HDEVINFO devInfo = SetupDiGetClassDevs(&hidGuid, NULL, NULL,
	                                        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (devInfo == INVALID_HANDLE_VALUE) return;

	SP_DEVICE_INTERFACE_DATA ifData{};
	ifData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	bool sent = false;

	for (DWORD idx = 0;
	     !sent && SetupDiEnumDeviceInterfaces(devInfo, NULL, &hidGuid, idx, &ifData);
	     ++idx)
	{
		DWORD needed = 0;
		SetupDiGetDeviceInterfaceDetailA(devInfo, &ifData, NULL, 0, &needed, NULL);
		if (needed == 0) continue;

		std::vector<uint8_t> buf(needed);
		auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_A*>(buf.data());
		detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
		if (!SetupDiGetDeviceInterfaceDetailA(devInfo, &ifData, detail, needed, NULL, NULL))
			continue;

		// Filter: Sony VID (054C) + DS4 gen1 (05C4) or gen2 (09CC)
		std::string path(detail->DevicePath);
		for (auto& c : path) c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
		bool isDS4 = path.find("vid_054c") != std::string::npos &&
		             (path.find("pid_05c4") != std::string::npos ||
		              path.find("pid_09cc") != std::string::npos);
		if (!isDS4) continue;

		HANDLE hid = CreateFileA(detail->DevicePath,
		                          GENERIC_READ | GENERIC_WRITE,
		                          FILE_SHARE_READ | FILE_SHARE_WRITE,
		                          NULL, OPEN_EXISTING, 0, NULL);
		if (hid == INVALID_HANDLE_VALUE) continue;

		// Confirm this is the gamepad interface (UsagePage=0x01, Usage=0x05)
		PHIDP_PREPARSED_DATA preparsed = nullptr;
		HIDP_CAPS caps{};
		if (HidD_GetPreparsedData(hid, &preparsed)) {
			HidP_GetCaps(preparsed, &caps);
			HidD_FreePreparsedData(preparsed);
		}
		if (caps.UsagePage != 0x0001 || caps.Usage != 0x0005) {
			CloseHandle(hid);
			continue;
		}

		// USB output report: ID 0x05, 32 bytes
		// report[1] = 0xFF  enables motors + LED
		// report[4] = right motor (weak/high-freq)
		// report[5] = left  motor (strong/low-freq)
		USHORT outLen = caps.OutputReportByteLength > 0 ? caps.OutputReportByteLength : 32;
		std::vector<uint8_t> report(outLen, 0);
		report[0] = 0x05;
		report[1] = 0xFF;
		if (outLen > 4) report[4] = rightMotor;
		if (outLen > 5) report[5] = leftMotor;

		DWORD written = 0;
		sent = WriteFile(hid, report.data(), outLen, &written, NULL) &&
		       written == static_cast<DWORD>(outLen);

		CloseHandle(hid);
	}

	SetupDiDestroyDeviceInfoList(devInfo);
}

static Vector2 ScreenToWorld_ForInput(double fbX, double fbY)
{
	// letterboxed / game viewport in framebuffer pixels
	const double drawX = double(GLHelper::GameOffsetX);
	const double drawY = double(GLHelper::GameOffsetY);
	const double drawW = double(GLHelper::GameWidth);
	const double drawH = double(GLHelper::GameHeight);

	// outside the game viewport? just return something harmless
	if (fbX < drawX || fbX > drawX + drawW ||
		fbY < drawY || fbY > drawY + drawH)
	{
		return Vector2{ 0.0f, 0.0f };
	}

	// --- framebuffer (bottom-left) -> NDC inside the game viewport ---
	const double nx = (fbX - drawX) / drawW;   // 0 at left, 1 at right
	const double ny = (fbY - drawY) / drawH;   // 0 at bottom, 1 at top

	const float ndcX = float(nx * 2.0 - 1.0);  // -1 .. 1
	const float ndcY = float(ny * 2.0 - 1.0);  // -1 .. 1
	const glm::vec3 ndc(ndcX, ndcY, 1.0f);

	// --- SAME camera logic as GLApp::draw ---
	GLApp::Camera2D cam;

	glm::mat3 camMat = GLApp::BuildPanZoomNDC(cam);

	// --- world (1920x1080) -> NDC normalization (same as Transform) ---
	const float sx = 2.0f / float(GLApp::VIRTUAL_W); // 1920
	const float sy = 2.0f / float(GLApp::VIRTUAL_H); // 1080
	const glm::mat3 N(
		sx, 0.0f, 0.0f,
		0.0f, sy, 0.0f,
		0.0f, 0.0f, 1.0f
	);

	// total forward: ndc = camMat * N * world
	// so inverse is world = (camMat * N)^-1 * ndc
	glm::mat3 invAll = glm::inverse(camMat * N);
	glm::vec3 w = invAll * ndc;

	return Vector2{ w.x, w.y };   // no magic -760 any more
}

InputManager::InputManager() : window(nullptr) {
	std::memset(currentKeys, 0, sizeof(currentKeys));
	std::memset(previousKeys, 0, sizeof(previousKeys));
	std::memset(currentMouseButtons, 0, sizeof(currentMouseButtons));
	std::memset(previousMouseButtons, 0, sizeof(previousMouseButtons));
	mousePosition = glm::vec2(0.0f);
}

InputManager::~InputManager() {
	Shutdown();
}

void InputManager::Initialize(GLFWwindow* glfwWindow) {
	window = glfwWindow;
	if (!window) {
		std::cerr << "[InputManager] Error: Invalid window pointer" << std::endl;
		return;
	}

	// Store InputManager pointer in window for callback access
	glfwSetWindowUserPointer(window, this);

	// Register PS4 DualShock 4 SDL gamepad mappings so that
	// glfwJoystickIsGamepad() returns true even without DS4Windows.
	// Covers DS4 gen 1 (05C4) and gen 2 (09CC) on Windows.
	const char* ps4Mappings =
		"030000004c050000c405000000000000,PS4 Controller,"
		"a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,"
		"guide:b12,leftshoulder:b4,leftstick:b10,lefttrigger:a3,"
		"leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,"
		"righttrigger:a4,rightx:a2,righty:a5,start:b9,x:b0,y:b3,"
		"platform:Windows,\n"
		"030000004c050000cc09000000000000,PS4 Controller,"
		"a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,"
		"guide:b12,leftshoulder:b4,leftstick:b10,lefttrigger:a3,"
		"leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,"
		"righttrigger:a4,rightx:a2,righty:a5,start:b9,x:b0,y:b3,"
		"platform:Windows,\n";
	glfwUpdateGamepadMappings(ps4Mappings);

	//std::cout << "[InputManager] Successfully initialized." << std::endl;
}

void InputManager::Update(float dt) {
	lastDt = (dt > 0.0f) ? dt : 0.016f;
	/*std::cout << "[INPUT MANAGER UPDATE]" << std::endl;*/
	updateInputStates();
	updateGamepadState();
	processDebugToggles();

	// Tick rumble timer — stop vibration when duration expires
	if (rumbleTimeRemaining > 0.0f) {
		rumbleTimeRemaining -= lastDt;
		if (rumbleTimeRemaining <= 0.0f) {
			rumbleTimeRemaining = 0.0f;
			RumbleDS4HID(0, 0);
		}
	}

	auto* coord = Coordinator::GetInstance();
	auto sceneManager = coord->GetSystem<SceneManager>();

	// will re-implemnt when editor is fully polished
	//// ESC key: Go back to previous scene
	//if (isKeyTriggered(GLFW_KEY_ESCAPE) && sceneManager) {
	//	std::string currentScene = sceneManager->CurrentScene;

	//	// If we're at MainMenu, ESC does nothing
	//	if (currentScene == "MainMenu") {
	//		std::cout << "[InputManager] ESC pressed - already at MainMenu, doing nothing" << std::endl;
	//		return; // Early exit - don't process ESC further
	//	}

	//	// For all other scenes, go back to LastScene
	//	std::string lastScene = sceneManager->GrabLastScene();

	//	if (!lastScene.empty() && lastScene != currentScene) {
	//		sceneManager->CurrentScene = lastScene;
	//		sceneManager->ClearLastScene();  // Clear so we can't toggle back
	//		std::cout << "[InputManager] ESC pressed - returning to: " << lastScene << std::endl;
	//	}
	//	else {
	//		std::cout << "[InputManager] ESC pressed - no previous scene to return to" << std::endl;
	//	}
	//}

	// Test delayed message (Advanced Feature: Message Queuing)
	//if (isKeyTriggered(GLFW_KEY_SPACE)) {
	//	auto* msgMgr = MessageManager::GetInstance();
	//	InputComboMessage* delayed = new InputComboMessage("DELAYED_COMBO", glfwGetTime());
	//	delayed->delay = 5.0;
	//	msgMgr->QueueMessage(delayed);
	//	std::cout << "[INPUT] Queued delayed combo (5s)" << std::endl;
	//}

	// Toggle fullscreen with F11
	if (isKeyTriggered(GLFW_KEY_F11))
	{
		if (Engine)
			Engine->ToggleFullscreen();
	}

	// FOR MOUSE TESTING; CAN USE FOR MAIN MENU
	//if (isMouseButtonTriggered(0)) {
	//	std::cout << "[MOUSE] Left Click at " << getMousePosition().x << ", " << getMousePosition().y << std::endl;
	//}

	//if (isMouseButtonTriggered(1)) {
	//	std::cout << "[MOUSE] Right Click at " << getMousePosition().x << ", " << getMousePosition().y << std::endl;
	//}

	//if (isMouseButtonReleased(0)) {
	//	std::cout << "[MOUSE] Left Released" << std::endl;
	//}

	// Editor toggle
	//if (isKeyTriggered(GLFW_KEY_O)) {
	//	bool now = !PulseEditor::IsEnabled();
	//	PulseEditor::SetEnabled(now);
	//	std::cout << "[Editor] ImGui " << (now ? "ON" : "OFF") << std::endl;
	//}
}

void InputManager::Shutdown() {
	if (window) {
		window = nullptr;
		std::cout << "[InputManager] Shutdown completed." << std::endl;
	}
}

void InputManager::updateInputStates() {
	std::memcpy(previousKeys, currentKeys, sizeof(currentKeys));
	std::memcpy(previousMouseButtons, currentMouseButtons, sizeof(currentMouseButtons));

	for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; key++)
		currentKeys[key] = (glfwGetKey(window, key) == GLFW_PRESS);

	for (int button = 0; button < 8; button++)
		currentMouseButtons[button] = (glfwGetMouseButton(window, button) == GLFW_PRESS);

	// Read cursor in window coordinates (top-left origin)
	double xw, yw;
	glfwGetCursorPos(window, &xw, &yw);
	mousePosition = glm::vec2(static_cast<float>(xw), static_cast<float>(yw));

	// Convert to FRAMEBUFFER pixels (handles HiDPI)
	int winW, winH, fbW, fbH;
	glfwGetWindowSize(window, &winW, &winH);
	glfwGetFramebufferSize(window, &fbW, &fbH);

	// Guard against divide-by-zero
	double fbX = 0.0, fbY = 0.0;
	if (winW > 0 && winH > 0) {
		// first go to framebuffer in TOP-LEFT
		double fbX_tl = xw * static_cast<double>(fbW) / static_cast<double>(winW);
		double fbY_tl = yw * static_cast<double>(fbH) / static_cast<double>(winH);

		// then convert to BOTTOM-LEFT origin to match glViewport / GameOffsetY
		fbX = fbX_tl;
		fbY = static_cast<double>(fbH) - fbY_tl;
	}

	Vector2 worldPos = ScreenToWorld_ForInput(fbX, fbY);
	mousePositionWorld = glm::vec2(worldPos.x, worldPos.y);
	mousePositionVirtual = mousePositionWorld;

	// Check if mouse is in the game viewport
	double localX = fbX - GLHelper::GameOffsetX;
	double localY = fbY - GLHelper::GameOffsetY;
	mouseInGameViewport = (localX >= 0.0 && localY >= 0.0 && localX <= GLHelper::GameWidth && localY <= GLHelper::GameHeight);
}

void InputManager::processDebugToggles() {
	const int debugKeys[] = {
	GLFW_KEY_F1, GLFW_KEY_F2, GLFW_KEY_F3,
	GLFW_KEY_F4, GLFW_KEY_F5, GLFW_KEY_F6,
	GLFW_KEY_F7
	};

	bool* debugFlags[] = {
		&debugRectsEnabled, &debugLinesEnabled, &debugCirclesEnabled,
		&debugCirclesFilledEnabled, &debugPointsEnabled, &debugGridEnabled,
		&debugCollisionEnabled
	};

	const char* debugNames[] = {
		"DEBUG RECTS", "DEBUG LINES", "DEBUG CIRCLES",
		"DEBUG CIRCLES FILLED", "DEBUG POINTS", "DEBUG GRID",
		"DEBUG COLLISION"
	};

	for (int i = 0; i < 7; ++i) {
		if (isKeyTriggered(debugKeys[i])) {
			*debugFlags[i] = !(*debugFlags[i]);
			std::cout << "[" << debugNames[i] << "] "
				<< (*debugFlags[i] ? "ENABLED" : "DISABLED") << std::endl;
		}
	}

	// ========================================================================
	// F8: Toggle Debug Display Text visibility
	// ========================================================================
	if (isKeyTriggered(GLFW_KEY_F8)) {
		auto* coord = Coordinator::GetInstance();
		auto sceneManager = coord->GetSystem<SceneManager>();

		if (coord && sceneManager)
		{
			const auto& sceneEntities = sceneManager->GetSceneEntities();

			for (auto entity : sceneEntities)
			{
				if (coord->HasComponent<TextComponent>(entity) && coord->HasComponent<Name>(entity))
				{
					auto& name = coord->GetComponent<Name>(entity);
					if (name.name == "Debug Display Text")
					{
						auto& txt = coord->GetComponent<TextComponent>(entity);
						txt.visible = !txt.visible;
						std::cout << "[DEBUG DISPLAY] " << (txt.visible ? "SHOWN" : "HIDDEN") << " (F8)" << std::endl;
						break;
					}
				}
			}
		}
	}
	// ========================================================================
}

// -------------------------------------------------
// Keyboard queries - 3 states as required
// -------------------------------------------------
bool InputManager::isKeyPressed(int key) const {
	if (key < 0 || key > GLFW_KEY_LAST) return false;
	return currentKeys[key];
}

bool InputManager::isKeyTriggered(int key) const {
	if (key < 0 || key > GLFW_KEY_LAST) return false;
	return currentKeys[key] && !previousKeys[key];
}

bool InputManager::isKeyReleased(int key) const {
	if (key < 0 || key > GLFW_KEY_LAST) return false;
	return !currentKeys[key] && previousKeys[key];
}

// -------------------------------------------------
// Mouse queries
// -------------------------------------------------
bool InputManager::isMouseButtonPressed(int button) const {
	if (button < 0 || button >= 8) return false;
	return currentMouseButtons[button];
}

bool InputManager::isMouseButtonTriggered(int button) const {
	if (button < 0 || button >= 8) return false;
	return currentMouseButtons[button] && !previousMouseButtons[button];
}

bool InputManager::isMouseButtonReleased(int button) const {
	if (button < 0 || button >= 8) return false;
	return !currentMouseButtons[button] && previousMouseButtons[button];
}

glm::vec2 InputManager::getMousePosition() const {
	return mousePosition;
}

// -------------------------------------------------
// C# accessor overrides: return gamepad virtual cursor when active
// -------------------------------------------------
float InputManager::GetMouseWorldPositionX() const {
	if (gamepadCursorActive) return gamepadCursorWorld.x;
	return mousePositionWorld.x;
}

float InputManager::GetMouseWorldPositionY() const {
	if (gamepadCursorActive) return gamepadCursorWorld.y;
	return mousePositionWorld.y;
}

bool InputManager::GetMouseInGameViewport() const {
	if (gamepadCursorActive) return true;
	return mouseInGameViewport;
}

// -------------------------------------------------
// Dump raw joystick layout to console (called once on connect).
// -------------------------------------------------
static void DiagDumpRawJoystick(int jid)
{
	int btnCount = 0, axisCount = 0, hatCount = 0;
	glfwGetJoystickButtons(jid, &btnCount);
	const float* axes = glfwGetJoystickAxes(jid, &axisCount);
	glfwGetJoystickHats(jid, &hatCount);

	std::cout << "[Gamepad DIAG] Raw layout: " << btnCount
		<< " buttons, " << axisCount << " axes, " << hatCount << " hats\n";
	if (axes) {
		for (int i = 0; i < axisCount; ++i)
			std::cout << "[Gamepad DIAG]   Axis[" << i << "] at rest = " << axes[i] << "\n";
	}
	std::cout << "[Gamepad DIAG] Press buttons/move sticks to see raw indices in console.\n";
}

// -------------------------------------------------
// Build a GLFWgamepadstate from raw joystick data (PS4 BT fallback).
//
// PS4 DualShock 4 raw HID on Windows Bluetooth (no DS4Windows):
//   Buttons: Square=0, Cross=1, Circle=2, Triangle=3
//            L1=4, R1=5, L2=6, R2=7, Share=8, Options=9, L3=10, R3=11
//   Axes:    LeftX=0, LeftY=1, RightX=2, L2=3, R2=4, RightY=5
//   Hat[0]:  D-pad bitmask
// -------------------------------------------------
static void BuildGamepadStateFromRawJoystick(int jid, GLFWgamepadstate& out)
{
	std::memset(&out, 0, sizeof(out));

	int btnCount = 0, axisCount = 0, hatCount = 0;
	const unsigned char* btns = glfwGetJoystickButtons(jid, &btnCount);
	const float*         axes = glfwGetJoystickAxes(jid, &axisCount);
	const unsigned char* hats = glfwGetJoystickHats(jid, &hatCount);

	auto getBtn = [&](int idx) -> unsigned char {
		return (btns && idx < btnCount && btns[idx] == GLFW_PRESS)
			? GLFW_PRESS : GLFW_RELEASE;
	};

	// --- Diagnostic: print every raw button press (edge only) ---
	// We keep a small static array so we only print on transition
	static unsigned char prevRawBtns[32] = {};
	if (btns) {
		int check = (btnCount < 32) ? btnCount : 32;
		for (int i = 0; i < check; ++i) {
			if (btns[i] == GLFW_PRESS && prevRawBtns[i] != GLFW_PRESS)
				std::cout << "[Gamepad DIAG] Raw button[" << i << "] PRESSED\n";
			prevRawBtns[i] = btns[i];
		}
	}
	// Diagnostic: print significant axis movements
	static float prevRawAxes[8] = {};
	if (axes) {
		int check = (axisCount < 8) ? axisCount : 8;
		for (int i = 0; i < check; ++i) {
			bool nowBig  = std::abs(axes[i]) > 0.4f;
			bool wasBig  = std::abs(prevRawAxes[i]) > 0.4f;
			if (nowBig && !wasBig)
				std::cout << "[Gamepad DIAG] Raw axis[" << i << "] = " << axes[i] << "\n";
			prevRawAxes[i] = axes[i];
		}
	}

	// --- Map to gamepad state ---
	// Face buttons
	out.buttons[GLFW_GAMEPAD_BUTTON_A] = getBtn(1);   // Cross
	out.buttons[GLFW_GAMEPAD_BUTTON_B] = getBtn(2);   // Circle
	out.buttons[GLFW_GAMEPAD_BUTTON_X] = getBtn(0);   // Square
	out.buttons[GLFW_GAMEPAD_BUTTON_Y] = getBtn(3);   // Triangle
	// Shoulder
	out.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER]  = getBtn(4); // L1
	out.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER] = getBtn(5); // R1
	// Meta
	out.buttons[GLFW_GAMEPAD_BUTTON_BACK]         = getBtn(8);  // Share
	out.buttons[GLFW_GAMEPAD_BUTTON_START]        = getBtn(9);  // Options
	out.buttons[GLFW_GAMEPAD_BUTTON_LEFT_THUMB]   = getBtn(10); // L3
	out.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB]  = getBtn(11); // R3

	// D-pad from hat
	if (hats && hatCount > 0) {
		out.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP]    = (hats[0] & GLFW_HAT_UP)    ? GLFW_PRESS : GLFW_RELEASE;
		out.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] = (hats[0] & GLFW_HAT_RIGHT) ? GLFW_PRESS : GLFW_RELEASE;
		out.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN]  = (hats[0] & GLFW_HAT_DOWN)  ? GLFW_PRESS : GLFW_RELEASE;
		out.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT]  = (hats[0] & GLFW_HAT_LEFT)  ? GLFW_PRESS : GLFW_RELEASE;
	}

	// Axes
	if (axes) {
		if (axisCount > 0) out.axes[GLFW_GAMEPAD_AXIS_LEFT_X]        = axes[0];
		if (axisCount > 1) out.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]        = axes[1];
		if (axisCount > 2) out.axes[GLFW_GAMEPAD_AXIS_RIGHT_X]       = axes[2];
		if (axisCount > 5) out.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]       = axes[5];
		if (axisCount > 3) out.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER]  = axes[3];
		if (axisCount > 4) out.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] = axes[4];
	}
}

// -------------------------------------------------
// Gamepad polling + virtual cursor
// -------------------------------------------------
void InputManager::updateGamepadState() {
	previousGamepad   = currentGamepad;
	bool wasConnected = gamepadConnected;
	gamepadConnected  = false;

	for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid) {
		if (!glfwJoystickPresent(jid)) continue;

		bool newSlot  = (gamepadJoystickID != jid);
		bool connected = false;

		if (glfwJoystickIsGamepad(jid)) {
			// Standard SDL-mapped path
			if (glfwGetGamepadState(jid, &currentGamepad)) {
				connected = true;
				if (newSlot) {
					std::cout << "[Gamepad] Connected (mapped): "
						<< glfwGetGamepadName(jid) << " (slot " << jid << ")\n";
					// Diagnostic for mapped path: print initial axis values
					std::cout << "[Gamepad DIAG] Mapped axes at rest:\n";
					for (int a = 0; a <= GLFW_GAMEPAD_AXIS_LAST; ++a)
						std::cout << "[Gamepad DIAG]   Axis[" << a << "] = "
							<< currentGamepad.axes[a] << "\n";
				}
			}
		}
		else {
			// Raw joystick fallback (PS4 BT without mapping, etc.)
			int btnCount = 0, axisCount = 0;
			glfwGetJoystickButtons(jid, &btnCount);
			glfwGetJoystickAxes(jid, &axisCount);

			if (btnCount > 0 || axisCount > 0) {
				if (newSlot) {
					const char* name = glfwGetJoystickName(jid);
					std::cout << "[Gamepad] Connected (raw fallback): "
						<< (name ? name : "Unknown") << " (slot " << jid
						<< ", btns=" << btnCount << ", axes=" << axisCount << ")\n";
					DiagDumpRawJoystick(jid);
				}
				BuildGamepadStateFromRawJoystick(jid, currentGamepad);
				connected = true;
			}
		}

		if (connected) {
			gamepadConnected = true;
			if (newSlot) {
				gamepadJoystickID  = jid;
				gamepadCursorWorld = glm::vec2(0.0f, 0.0f);
			}
			break;
		}
	}

	if (!gamepadConnected) {
		if (wasConnected) {
			std::cout << "[Gamepad] Disconnected.\n";
			gamepadJoystickID  = -1;
			gamepadCursorActive = false;
		}
		std::memset(&currentGamepad, 0, sizeof(currentGamepad));
		return;
	}

	// --- Diagnostic: print mapped button triggers ---
	for (int b = 0; b <= GLFW_GAMEPAD_BUTTON_LAST; ++b) {
		if (currentGamepad.buttons[b] == GLFW_PRESS &&
			previousGamepad.buttons[b] != GLFW_PRESS) {
			const char* names[] = {
				"Cross(A)","Circle(B)","Square(X)","Triangle(Y)",
				"L1","R1","Select/Share","Start/Options","Guide",
				"L3","R3","DUp","DRight","DDown","DLeft"
			};
			if (b < 15)
				std::cout << "[Gamepad] Button[" << b << "] = " << names[b] << " triggered\n";
		}
	}

	// --- Virtual cursor driven by left stick + D-pad ---
	const float DEAD  = 0.15f;
	const float SPEED = 1400.0f;

	float lx = currentGamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
	float ly = currentGamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
	if (std::abs(lx) < DEAD) lx = 0.0f;
	if (std::abs(ly) < DEAD) ly = 0.0f;

	if (currentGamepad.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] == GLFW_PRESS) lx += 1.0f;
	if (currentGamepad.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT]  == GLFW_PRESS) lx -= 1.0f;
	if (currentGamepad.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP]    == GLFW_PRESS) ly -= 1.0f;
	if (currentGamepad.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN]  == GLFW_PRESS) ly += 1.0f;

	if (lx != 0.0f || ly != 0.0f) {
		gamepadCursorActive   = true;
		gamepadCursorWorld.x += lx * SPEED * lastDt;
		gamepadCursorWorld.y -= ly * SPEED * lastDt;
		gamepadCursorWorld.x  = glm::clamp(gamepadCursorWorld.x, -1600.0f, 1600.0f);
		gamepadCursorWorld.y  = glm::clamp(gamepadCursorWorld.y,  -900.0f,  900.0f);
	}

	// Return cursor control to mouse when it moves
	double mx = 0.0, my = 0.0;
	if (window) glfwGetCursorPos(window, &mx, &my);
	static double prevMx = mx, prevMy = my;
	if (std::abs(mx - prevMx) > 2.0 || std::abs(my - prevMy) > 2.0)
		gamepadCursorActive = false;
	prevMx = mx;
	prevMy = my;
}

bool InputManager::isGamepadButtonPressed(int button) const {
	if (!gamepadConnected || button < 0 || button > GLFW_GAMEPAD_BUTTON_LAST) return false;
	return currentGamepad.buttons[button] == GLFW_PRESS;
}

bool InputManager::isGamepadButtonTriggered(int button) const {
	if (!gamepadConnected || button < 0 || button > GLFW_GAMEPAD_BUTTON_LAST) return false;
	return (currentGamepad.buttons[button] == GLFW_PRESS) &&
		   (previousGamepad.buttons[button] != GLFW_PRESS);
}

bool InputManager::isGamepadButtonReleased(int button) const {
	if (!gamepadConnected || button < 0 || button > GLFW_GAMEPAD_BUTTON_LAST) return false;
	return (currentGamepad.buttons[button] != GLFW_PRESS) &&
		   (previousGamepad.buttons[button] == GLFW_PRESS);
}

float InputManager::getGamepadAxis(int axis) const {
	if (!gamepadConnected || axis < 0 || axis > GLFW_GAMEPAD_AXIS_LAST) return 0.0f;
	return currentGamepad.axes[axis];
}

void InputManager::rumbleGamepad(float leftMotor, float rightMotor, float durationSec) {
	if (!gamepadConnected || durationSec <= 0.0f) return;

	// DS4: report[4]=right motor (weak/high-freq), report[5]=left motor (strong/low-freq)
	RumbleDS4HID(static_cast<uint8_t>(glm::clamp(rightMotor, 0.0f, 1.0f) * 255.0f),
	             static_cast<uint8_t>(glm::clamp(leftMotor,  0.0f, 1.0f) * 255.0f));
	rumbleTimeRemaining = durationSec;
}