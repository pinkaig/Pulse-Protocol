/******************************************************************************/
/**
 * @file        glhelper.cpp
 * @project     Pulse Protocol
 * @author      Goh Pin Kai
 * @brief		Implements OpenGL helper utilities for window management,
				input handling, timing, and context setup.
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#include "glhelper.h"
#include <iostream>
#include "CoreEngine/ECS/Coordinator.h"

/*                                                   objects with file scope
----------------------------------------------------------------------------- */
// static data members declared in GLHelper
GLint GLHelper::width;
GLint GLHelper::height;
GLdouble GLHelper::fps;
GLdouble GLHelper::delta_time;
std::string GLHelper::title;
GLFWwindow* GLHelper::ptr_window;
GLboolean GLHelper::keystateU = GL_FALSE;
GLboolean GLHelper::keystateP = GL_FALSE;
GLboolean GLHelper::keystateS = GL_FALSE;
GLboolean GLHelper::leftmousebtn = GL_FALSE;
GLboolean GLHelper::rightmousebtn = GL_FALSE;
GLboolean GLHelper::keystateC = GL_TRUE;
GLboolean GLHelper::keystate1 = GL_FALSE;
GLboolean GLHelper::keystate2 = GL_FALSE;
GLboolean GLHelper::keyUp = GL_FALSE;
GLboolean GLHelper::keyDown = GL_FALSE;
GLboolean GLHelper::keyLeft = GL_FALSE;
GLboolean GLHelper::keyRight = GL_FALSE;
GLboolean GLHelper::keyPause = GL_FALSE;

GLint  GLHelper::GameWidth;
GLint  GLHelper::GameHeight;

GLint  GLHelper::GameOffsetX;
GLint  GLHelper::GameOffsetY;
std::pair<double, double> GLHelper::s_mousePos = { 0.0, 0.0 };

GLboolean GLHelper::leftmousetriggerme = GL_FALSE;

double GLHelper::mouseX = 0.0;
double GLHelper::mouseY = 0.0;

static bool is_fullscreen;
static int windowed_width, windowed_height;
static int windowed_pos_x, windowed_pos_y;

std::unordered_map<int, bool> GLHelper::keyState;  // Initialize the key state map

/*  _________________________________________________________________________ */
/*! init

@param GLint width
@param GLint height
Dimensions of window requested by program

@param std::string title_str
String printed to window's title bar

@return bool
true if OpenGL context and GLEW were successfully initialized.
false otherwise.

Uses GLFW to create OpenGL context. GLFW's initialization follows from here:
http://www.glfw.org/docs/latest/quick.html
a window of size width x height pixels
and its associated OpenGL context that matches a core profile that is
compatible with OpenGL 4.5 and doesn't support "old" OpenGL, has 32-bit RGBA,
double-buffered color buffer, 24-bit depth buffer and 8-bit stencil buffer
with each buffer of size width x height pixels
*/
bool GLHelper::init(GLint awidth, GLint aheight, std::string atitle) {
	GLHelper::width = awidth;
	GLHelper::height = aheight;
	GLHelper::title = atitle;

	is_fullscreen = false;
	windowed_width = width;
	windowed_height = height;
	windowed_pos_x = 100;
	windowed_pos_y = 100;

	// Part 1
	if (!glfwInit()) {
		std::cout << "GLFW init has failed - abort program!!!" << std::endl;
		return false;
	}

	// In case a GLFW function fails, an error is reported to callback function
	glfwSetErrorCallback(GLHelper::error_cb);

	// Before asking GLFW to create an OpenGL context, we specify the minimum constraints
	// in that context:
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
	glfwWindowHint(GLFW_DEPTH_BITS, 24);
	glfwWindowHint(GLFW_RED_BITS, 8); glfwWindowHint(GLFW_GREEN_BITS, 8);
	glfwWindowHint(GLFW_BLUE_BITS, 8); glfwWindowHint(GLFW_ALPHA_BITS, 8);

	GLHelper::ptr_window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
	if (!GLHelper::ptr_window) {
		std::cerr << "GLFW unable to create OpenGL context - abort program\n";
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(GLHelper::ptr_window);
	glfwSwapInterval(1); // Enable VSync

	setup_event_callbacks();

	// this is the default setting ...
	glfwSetInputMode(GLHelper::ptr_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	// Part 2: Initialize entry points to OpenGL functions and extensions
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		std::cerr << "Unable to initialize GLEW - error: "
			<< glewGetErrorString(err) << " abort program" << std::endl;
		return false;
	}
	if (GLEW_VERSION_4_5) {
		std::cout << "Using glew version: " << glewGetString(GLEW_VERSION) << std::endl;
		std::cout << "Driver supports OpenGL 4.5\n" << std::endl;
	}
	else {
		std::cerr << "Driver doesn't support OpenGL 4.5 - abort program" << std::endl;
	}

	return true;
}

void GLHelper::setup_event_callbacks() {

	glfwSetFramebufferSizeCallback(GLHelper::ptr_window, GLHelper::fbsize_cb);
	glfwSetKeyCallback(GLHelper::ptr_window, GLHelper::key_cb);
	glfwSetMouseButtonCallback(GLHelper::ptr_window, GLHelper::mousebutton_cb);
	glfwSetCursorPosCallback(GLHelper::ptr_window, GLHelper::mousepos_cb);
	glfwSetScrollCallback(GLHelper::ptr_window, GLHelper::mousescroll_cb);
	glfwSetWindowFocusCallback(GLHelper::ptr_window, GLHelper::focus_cb);
}
/*  _________________________________________________________________________ */
/*! cleanup

@param none

@return none

For now, there are no resources allocated by the application program.
The only task is to have GLFW return resources back to the system and
gracefully terminate.
*/
void GLHelper::cleanup() {
	// Part 1
	glfwTerminate();
}

/*  _________________________________________________________________________*/
/*! key_cb

@param GLFWwindow*
Handle to window that is receiving event

@param int
the keyboard key that was pressed or released

@parm int
Platform-specific scancode of the key

@parm int
GLFW_PRESS, GLFW_REPEAT or GLFW_RELEASE
action will be GLFW_KEY_UNKNOWN if GLFW lacks a key token for it,
for example E-mail and Play keys.

@parm int
bit-field describing which modifier keys (shift, alt, control)
were held down

@return none

This function is called when keyboard buttons are pressed.
When the ESC key is pressed, the close flag of the window is set.
*/
void GLHelper::key_cb(GLFWwindow* pwin, int key, int scancode, int action, int mod) {
	(void)pwin; (void)mod; (void)scancode; // Suppress unused parameter warnings
	if (GLFW_PRESS == action) {
		//if ((mod & GLFW_MOD_CONTROL) && (mod & GLFW_MOD_ALT) && key == GLFW_KEY_9) {
		//    GAM200_CORE_INFO("Key Ctrl+Alt+Del pressed");
		//    glfwIconifyWindow(pwin);
		//}
		if (key == GLFW_KEY_U) {
			keystateU = (keystateU == GL_TRUE) ? GL_FALSE : GL_TRUE;
		}
		if (key == GLFW_KEY_C) {
			keystateC = (keystateC == GL_TRUE) ? GL_FALSE : GL_TRUE;
		}
		//if ((key == GLFW_KEY_9 || key == GLFW_KEY_F11) && action == GLFW_PRESS) {
		//    GAM200_CORE_INFO("Key F11 pressed");
		//    toggleFullScreen();
		//}
		keystateP = (key == GLFW_KEY_P) ? GL_TRUE : GL_FALSE;
		keystateS = (key == GLFW_KEY_S) ? GL_TRUE : GL_FALSE;
		keystate1 = (key == GLFW_KEY_1) ? GL_TRUE : GL_FALSE;
		keystate2 = (key == GLFW_KEY_2) ? GL_TRUE : GL_FALSE;

		keyUp = (key == GLFW_KEY_UP) ? GL_TRUE : GL_FALSE;
		keyDown = (key == GLFW_KEY_DOWN) ? GL_TRUE : GL_FALSE;
		keyLeft = (key == GLFW_KEY_LEFT) ? GL_TRUE : GL_FALSE;
		keyRight = (key == GLFW_KEY_RIGHT) ? GL_TRUE : GL_FALSE;

		keyState[key] = true;
#ifdef _DEBUG

		// Attempt to get the key name using glfwGetKeyName (for printable keys)
		const char* keyName = glfwGetKeyName(key, scancode);
		(void)keyName; // Suppress unused variable warning

		//if (keyName) {
		//    GAM200_CORE_INFO("Key released: {}", keyName);
		//}
		//else {
		//    // Handle special keys that glfwGetKeyName doesn't handle
		//    switch (key) {
		//    case GLFW_KEY_SPACE: GAM200_CORE_INFO("Key released: Space"); break;
		//    case GLFW_KEY_ENTER: GAM200_CORE_INFO("Key released: Enter"); break;
		//    //case GLFW_KEY_ESCAPE: GAM200_CORE_INFO("Key released: Escape"); glfwSetWindowShouldClose(pwin, GLFW_TRUE); break;
		//    case GLFW_KEY_LEFT: GAM200_CORE_INFO("Key released: Left Arrow"); break;
		//    case GLFW_KEY_RIGHT: GAM200_CORE_INFO("Key released: Right Arrow"); break;
		//    case GLFW_KEY_UP: GAM200_CORE_INFO("Key released: Up Arrow"); break;
		//    case GLFW_KEY_DOWN: GAM200_CORE_INFO("Key released: Down Arrow"); break;
		//    default: GAM200_CORE_INFO("Key released: Unknown (code {})", key); break;
		//    }
		//}

		std::cout << "Key pressed" << std::endl;
#endif
	}
	else if (GLFW_REPEAT == action) {
		keystateP = GL_FALSE;
		keystateS = GL_FALSE;
		keystate1 = GL_FALSE;
		keystate2 = GL_FALSE;

		keyUp = GL_FALSE;
		keyDown = GL_FALSE;
		keyLeft = GL_FALSE;
		keyRight = GL_FALSE;

		keyState[key] = true;
#ifdef _DEBUG
		std::cout << "Key repeatedly pressed" << std::endl;
#endif
	}
	else if (GLFW_RELEASE == action) {
		keystateP = GL_FALSE;
		keystateS = GL_FALSE;
		keystate1 = GL_FALSE;
		keystate2 = GL_FALSE;

		keyUp = GL_FALSE;
		keyDown = GL_FALSE;
		keyLeft = GL_FALSE;
		keyRight = GL_FALSE;

		keyState[key] = false;
#ifdef _DEBUG
		std::cout << "Key released" << std::endl;
#endif
	}

	if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action) {
		keyPause = (keyPause == GL_FALSE) ? GL_TRUE : GL_FALSE;
	}
}

/*  _________________________________________________________________________*/
/*! mousebutton_cb

@param GLFWwindow*
Handle to window that is receiving event

@param int
the mouse button that was pressed or released
GLFW_MOUSE_BUTTON_LEFT and GLFW_MOUSE_BUTTON_RIGHT specifying left and right
mouse buttons are most useful

@parm int
action is either GLFW_PRESS or GLFW_RELEASE

@parm int
bit-field describing which modifier keys (shift, alt, control)
were held down

@return none

This function is called when mouse buttons are pressed.
*/
void GLHelper::mousebutton_cb(GLFWwindow* pwin, int button, int action, int mod) {
	(void)pwin; (void)mod; // Suppress unused parameter warnings
	//UNREFERENCED_PARAMETER(*pwin);
	//UNREFERENCED_PARAMETER(mod);
	switch (button) {
	case GLFW_MOUSE_BUTTON_LEFT:
		leftmousebtn = GL_TRUE ? GL_FALSE : GL_TRUE;
#ifdef _DEBUG
		std::cout << "Left mouse button ";
#endif
		break;
	case GLFW_MOUSE_BUTTON_RIGHT:
#ifdef _DEBUG
		std::cout << "Right mouse button ";
#endif
		break;
	}
	switch (action) {
	case GLFW_PRESS:
#ifdef _DEBUG
		std::cout << "pressed!!!" << std::endl;
#endif
		break;
	case GLFW_RELEASE:
#ifdef _DEBUG
		std::cout << "released!!!" << std::endl;
#endif
		break;
	}

	if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT && leftmousetriggerme == GL_FALSE) {
		leftmousetriggerme = GL_TRUE;
		std::cout << "haha" << std::endl;
	}
	else leftmousetriggerme = GL_FALSE;
	if (GLFW_PRESS == action)
	{
		leftmousebtn = (button == GLFW_MOUSE_BUTTON_LEFT) ? GL_TRUE : GL_FALSE;
		rightmousebtn = (button == GLFW_MOUSE_BUTTON_RIGHT) ? GL_TRUE : GL_FALSE;
	}

}

/*  _________________________________________________________________________*/
/*! mousepos_cb

@param GLFWwindow*
Handle to window that is receiving event

@param double
new cursor x-coordinate, relative to the left edge of the client area

@param double
new cursor y-coordinate, relative to the top edge of the client area

@return none

This functions receives the cursor position, measured in screen coordinates but
relative to the top-left corner of the window client area.
*/
void GLHelper::mousepos_cb(GLFWwindow* pwin, double xpos, double ypos) {
	(void)pwin; (void)xpos; (void)ypos; // Suppress unused parameter warnings
#ifdef _DEBUG
	//UNREFERENCED_PARAMETER(*pwin);
  //std::cout << "Mouse cursor position: (" << xpos << ", " << ypos << ")" << std::endl;

	mouseX = xpos;
	mouseY = ypos;
	s_mousePos = { xpos, ypos };
#endif
}

std::pair<double, double> GLHelper::getMousePosition() {
	return s_mousePos;
}


/*  _________________________________________________________________________*/
/*! mousescroll_cb

@param GLFWwindow*
Handle to window that is receiving event

@param double
Scroll offset along X-axis

@param double
Scroll offset along Y-axis

@return none

This function is called when the user scrolls, whether with a mouse wheel or
touchpad gesture. Although the function receives 2D scroll offsets, a simple
mouse scroll wheel, being vertical, provides offsets only along the Y-axis.
*/
void GLHelper::mousescroll_cb(GLFWwindow* pwin, double xoffset, double yoffset) {
	(void)pwin; (void)xoffset; (void)yoffset; // Suppress unused parameter warnings
#ifdef _DEBUG
	//UNREFERENCED_PARAMETER(*pwin);
	std::cout << "Mouse scroll wheel offset: ("
		<< xoffset << ", " << yoffset << ")" << std::endl;
#endif
}

/*  _________________________________________________________________________ */
/*! error_cb

@param int
GLFW error code

@parm char const*
Human-readable description of the code

@return none

The error callback receives a human-readable description of the error and
(when possible) its cause.
*/
void GLHelper::error_cb(int error, char const* description) {
	(void)error; (void)description; // Suppress unused parameter warnings
#ifdef _DEBUG
	//UNREFERENCED_PARAMETER(error);
	std::cerr << "GLFW error: " << description << std::endl;
#endif
}

/*  _________________________________________________________________________ */
/*! fbsize_cb

@param GLFWwindow*
Handle to window that is being resized

@parm int
Width in pixels of new window size

@parm int
Height in pixels of new window size

@return none

This function is called when the window is resized - it receives the new size
of the window in pixels.
*/
void GLHelper::fbsize_cb(GLFWwindow* ptr_win, int fbwidth, int fbheight) {
	(void)ptr_win; // Suppress unused parameter warning
#ifdef _DEBUG
	//UNREFERENCED_PARAMETER(*ptr_win);
	std::cout << "fbsize_cb getting called!!!" << std::endl;
#endif

	/* New

	GLHelper::width = fbwidth;
	GLHelper::height = fbheight;

	// Set the viewport to cover the entire framebuffer
	GLHelper::GameWidth = fbwidth - GLHelper::EditorWidth;;
	GLHelper::GameHeight = fbheight - GLHelper::EditorHeight;

	// Ensure the offsets are recalculated dynamically
	GLHelper::GameOffsetX = GLHelper::EditorWidth / 2; // Centered horizontally if EditorWidth is applied on both sides
	GLHelper::GameOffsetY = GLHelper::GameOffsetY;                         // No vertical offset applied in this layout

	// Update the viewport to reflect these changes
	glViewport(GLHelper::GameOffsetX, GLHelper::GameOffsetY, GLHelper::GameWidth, GLHelper::GameHeight);
	*/
	GLHelper::width = fbwidth;
	GLHelper::height = fbheight;

	/*GLint w = GLHelper::width - GLHelper::EditorWidth, h = GLHelper::height - GLHelper::EditorHeight;*/
	//float a = (9.0f / 16.0f) * w; // a has a 9:16 ratio with w
	//float b = (16.0f / 9.0f) * h; // b has a 16:9 ratio with h

	//if (h > a) {
	//	h = static_cast<GLint>(a);
	//}
	//else if (w > b) {
	//	w = static_cast<GLint>(b);
	//}

	//GLHelper::GameWidth = w;
	//GLHelper::GameHeight = h;

	/*glViewport(GameOffsetX, GameOffsetY, w, h);*/
}

void GLHelper::focus_cb(GLFWwindow* window, int focused) {
	(void)window; // Suppress unused parameter warning
	//UNREFERENCED_PARAMETER(window);
	if (focused) {
		// Window has gained focus, resume game and audio
		//GAM200_CORE_INFO("Window focused, resuming game...");
		//ResumeGame();
		//UnmuteAudio();
		//keyPause = GL_FALSE;
	}
	else {
		// Window has lost focus, pause game and audio
		//GAM200_CORE_INFO("Window lost focus, pausing game...");
		//PauseGame();
		//MuteAudio();
		keyPause = GL_TRUE;
		//glfwIconifyWindow(window);

	}
}

/*  _________________________________________________________________________*/
/*! update_time

@param double
fps_calc_interval: the interval (in seconds) at which fps is to be
calculated

This function must be called once per game loop. It uses GLFW's time functions
to compute:
1. the interval in seconds between each frame
2. the frames per second every "fps_calc_interval" seconds
*/
void GLHelper::update_time(double fps_calc_interval) {
	// get elapsed time (in seconds) between previous and current frames
	static double prev_time = glfwGetTime();
	double curr_time = glfwGetTime();
	GLHelper::delta_time = curr_time - prev_time;
	prev_time = curr_time;

	// fps calculations
	static double count = 0.0; // number of game loop iterations
	static double start_time = glfwGetTime();
	// get elapsed time since very beginning (in seconds) ...
	double elapsed_time = curr_time - start_time;

	++count;

	// update fps at least every 10 seconds ...
	fps_calc_interval = (fps_calc_interval < 0.0) ? 0.0 : fps_calc_interval;
	fps_calc_interval = (fps_calc_interval > 10.0) ? 10.0 : fps_calc_interval;
	if (elapsed_time > fps_calc_interval) {
		if (count / elapsed_time > 120.f)
			GLHelper::fps = 120.f;
		else
			GLHelper::fps = count / elapsed_time;
		start_time = curr_time;
		count = 0.0;
	}
}

void GLHelper::print_specs()
{
	std::cout << "--------------------------------------------" << std::endl;
	std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "GL Version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "GL Shading Language: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

	GLint majorVersion;
	glGetIntegerv(GL_MINOR_VERSION, &majorVersion);
	std::cout << "GL Major Version : " << majorVersion << std::endl;

	GLint minorVersion;
	glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
	std::cout << "GL Minor Version : " << minorVersion << std::endl;

	bool printEtensions = false;
	if (printEtensions) {
		GLint nExtensions;
		glGetIntegerv(GL_NUM_EXTENSIONS, &nExtensions);

		for (int i = 0; i < nExtensions; i++) {
			std::cout << "GL Shading Language: " << glGetStringi(GL_EXTENSIONS, i) << std::endl;
		}
	}
	std::cout << "--------------------------------------------" << std::endl;
}

void GLHelper::toggleFullScreen() {
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	if (is_fullscreen) {
		// Switch to windowed mode
		glfwSetWindowMonitor(ptr_window, nullptr, windowed_pos_x, windowed_pos_y, windowed_width, windowed_height, 0);
		is_fullscreen = false;
	}
	else {
		// Save current window size and position
		glfwGetWindowPos(ptr_window, &windowed_pos_x, &windowed_pos_y);
		glfwGetWindowSize(ptr_window, &windowed_width, &windowed_height);

		// Switch to full-screen mode
		glfwSetWindowMonitor(ptr_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
		is_fullscreen = true;
	}
}

