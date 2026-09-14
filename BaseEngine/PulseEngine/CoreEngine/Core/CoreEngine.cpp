/******************************************************************************/
/*
 * @file        CoreEngine.cpp
 * @project     Pulse Protocol
 * @author		Chia Wei Xuan Rachael (primary) - 50%
 * @author		Chloe, Ben, Reginald, Pin Kai, Jun Yong (secondary) - 10% each
 * @brief		Implementation of the core engine class that initializes all
 *				engine subsystems (graphics, input, audio, physics), manages
 *				the main game loop with delta time calculation, handles system
 *				updates, integrates ImGui editor support, and provides debug
 *				rendering utilities for development visualization.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>			   // for OutputDebugString, etc.
#include <mmsystem.h>			   // for timeGetTime()
#pragma comment(lib, "winmm.lib")  //shortcut instead of doing in CMake

#include "pch/pch.h"
#include "graphics/stb_image.h"
#include "CoreEngine/Factory/Factory.h"
#include "Serialization/Serialization.h"
#include "CoreEngine/Prefab/PrefabInstance.h"
#ifdef ENABLE_EDITOR
#include "../PulseEditor/Editor.h"
#endif
#include "TestCases/TestCase.h"
// #include "glapp.h"
#include "Resources/AssetRegistry.h"
#include "CoreEngine/Asset/AssetsManager.h"
#include "GameState.h"

#include "Components/DisplayName.h"
#include "Components/EntityType.h"
#include "Components/ParentChild.h"


#include "Audio/AudioSource.h"
#include "Graphics/Text.h"
#pragma comment(lib, "winmm.lib")

#include <shlwapi.h>                // GetModuleFileNameA(), PathRemoveFileSpecA()
#pragma comment(lib, "shlwapi.lib") // Needed for <shlwapi.h>

// Game window dimensions and title
CoreEngine* Engine = nullptr;
const char WindowTitle[] = "Game Window";
Config config;
AssetsManager mAssets;
int WindowWidth;
int WindowHeight;
double g_scrollY;
std::filesystem::path FilePathToGame;

using HelloWorldFunctionPtr = void(*)(void);
using InitFunctionPtr = void(*)(CoreEngine*);

namespace
{
	std::filesystem::path ResolveSharedUserSettingsPath(const std::filesystem::path& gameRoot)
	{
		// Single shared runtime settings location:
		//   .../build/PulseProtocol/JSON/user_settings.json
		if (!gameRoot.empty())
		{
			const std::filesystem::path devBuildShared =
				gameRoot.parent_path() / "build" / "PulseProtocol" / "JSON" / "user_settings.json";
			if (std::filesystem::exists(devBuildShared.parent_path()))
				return devBuildShared;

			const std::filesystem::path viaConfigParent = gameRoot.parent_path() / "JSON" / "user_settings.json";
			if (std::filesystem::exists(viaConfigParent.parent_path()))
				return viaConfigParent;

			return gameRoot / "JSON" / "user_settings.json";
		}

		return std::filesystem::path("user_settings.json");
	}

	bool TryLoadStartupFullscreenFromUserSettings(const std::filesystem::path& gameRoot, bool& fullscreenOut)
	{
		const std::filesystem::path settingsPath = ResolveSharedUserSettingsPath(gameRoot);
		if (!std::filesystem::exists(settingsPath))
			return false;

		rapidjson::Document doc;
		if (!JSONUtils::ReadJSONFromFile(doc, settingsPath) || !doc.IsObject())
			return false;

		auto it = doc.FindMember("Fullscreen");
		if (it == doc.MemberEnd() || !it->value.IsBool())
			return false;

		fullscreenOut = it->value.GetBool();
		return true;
	}

	void PersistFullscreenToUserSettings(const std::filesystem::path& gameRoot, bool fullscreen)
	{
		if (gameRoot.empty())
			return;

		auto writeFullscreen = [fullscreen](const std::filesystem::path& settingsPath)
		{
			rapidjson::Document doc;
			if (!JSONUtils::ReadJSONFromFile(doc, settingsPath) || !doc.IsObject())
			{
				doc.SetObject();
			}

			auto& alloc = doc.GetAllocator();
			if (doc.HasMember("Fullscreen"))
			{
				doc["Fullscreen"].SetBool(fullscreen);
			}
			else
			{
				doc.AddMember(rapidjson::Value("Fullscreen", alloc), rapidjson::Value(fullscreen), alloc);
			}

			if (!JSONUtils::WriteJSONToFile(doc, settingsPath, true))
			{
				std::cerr << "[CoreEngine] WARNING: Failed to persist fullscreen setting to "
					<< settingsPath << "\n";
			}
		};

		// Runtime settings used by the running executable.
		writeFullscreen(ResolveSharedUserSettingsPath(gameRoot));
	}
}

Framework::Transform* CoreEngine::GetTransformComponent(int ID)
{
	auto g_coordinator = Coordinator::GetInstance();
	return &g_coordinator->GetComponent <Framework::Transform>(ID);
}

// Load coreclr.dll from our application so that we are able to use the functions 
// contained within it to interface with the .NET runtime
void CoreEngine::startScriptEngine()
{
	std::cout << "[Hot Reload] Working dir: " << std::filesystem::current_path() << std::endl;

	// Steppppppppppp 1, we get the current executable directory so that we can find the coreclr.dll to load
	std::string runtimePath(MAX_PATH, '\0');

	// Get the path to PulseEngine.dll instead of the .exe
	HMODULE hEngine = GetModuleHandleA("PulseEngine.dll");
	if (!hEngine)
	{
		throw std::runtime_error("Failed to get PulseEngine.dll module handle");
	}

	GetModuleFileNameA(hEngine, runtimePath.data(), MAX_PATH);
	PathRemoveFileSpecA(runtimePath.data());

	// Since PathRemoveFileSpecA() removes from data(), the size is not updated, so we must manually update it
	runtimePath.resize(std::strlen(runtimePath.data()));

	//std::cout << "[CoreEngine] Runtime path: \n\n\n\n\n\n\n\n" << runtimePath << std::endl;
	
	// Also, while we're at it, set the current working directory to the current executable directory
	std::filesystem::current_path(runtimePath);


	// Construct the CoreCLR path
	std::string coreClrPath(runtimePath); // Works
	coreClrPath += "\\coreclr.dll";
	
	//std::cout << "[CoreEngine] Looking for:  \n\n\n\n\n\n\n\n" << coreClrPath << std::endl;

	// Load the CoreCLR DLL
	coreClr = LoadLibraryExA(coreClrPath.c_str(), nullptr, 0);
	if (!coreClr)
	{
		DWORD error = GetLastError();  
		std::cout << error << std::endl;

		std::ostringstream oss;
		oss << "Failed to load CoreCLR from: " << coreClrPath
			<< "\nWindows Error Code: " << error;

		throw std::runtime_error("Failed to load CoreCLR.");
	}

	// Step 2, we get CoreCLR hosting functions
	initializeCoreClr = getCoreClrFuncPtr<coreclr_initialize_ptr>("coreclr_initialize");
	createManagedDelegate = getCoreClrFuncPtr<coreclr_create_delegate_ptr>("coreclr_create_delegate");
	shutdownCoreClr = getCoreClrFuncPtr<coreclr_shutdown_ptr>("coreclr_shutdown");

	// Step 3, we then construct AppDomain properties used when starting the runtime
	std::string tpaList = buildTpaList(runtimePath);
	// Define CoreCLR properties
	std::array propertyKeys =
	{
		"TRUSTED_PLATFORM_ASSEMBLIES",      // Trusted assemblies (like the GAC)
		"APP_PATHS",                        // Directories to probe for application assemblies
	};
	std::array propertyValues =
	{
		tpaList.c_str(),
		runtimePath.c_str()
	};

	//Step 4, kms.. jk, start the CoreCLR runtime (linking part)
	int result = initializeCoreClr
	(
		runtimePath.c_str(),     // AppDomain base path
		"SampleHost",            // AppDomain friendly name, this can be anything you want really
		static_cast<int>(propertyKeys.size()),     // Property count
		propertyKeys.data(),     // Property names
		propertyValues.data(),   // Property values
		&hostHandle,             // Host handle
		&domainId                // AppDomain ID
	);

	// Check if intiialization of CoreCLR failed
	if (result < 0)
	{
		std::ostringstream oss;
		oss << std::hex << std::setfill('0') << std::setw(8)
			<< "Failed to initialize CoreCLR. Error 0x" << result << "\n";
		throw std::runtime_error(oss.str());
	}

	std::cout << "[Hot Reload] Working dir: " << std::filesystem::current_path() << std::endl;
}

// Unload the .NET runtime 
void CoreEngine::stopScriptEngine()
{
    // Shutdown CoreCLR
    const int RESULT = shutdownCoreClr(hostHandle, domainId);
    if (RESULT < 0)
    {
        std::stringstream oss;
        oss << std::hex << std::setfill('0') << std::setw(8)
            << "Failed to shut down CoreCLR. Error 0x" << RESULT << "\n";
        throw std::runtime_error(oss.str());
    }
}

// Construct a list of DLLs that tells the CLR which are the DLLs it can load at runtime and 
// where to find them. Any DLLs not a part of this list will not be able to be loaded. SO RMB TO ADD
std::string CoreEngine::buildTpaList(const std::string& directory)
{
	// Constants
	static const std::string SEARCH_PATH = directory + "\\*.dll";
	static constexpr char PATH_DELIMITER = ';';

	// Create a osstream object to compile the string
	std::ostringstream tpaList;
	
	// Search the current directory for the TPAs (.DLLs)
	WIN32_FIND_DATAA findData;
	HANDLE fileHandle = FindFirstFileA(SEARCH_PATH.c_str(), &findData);
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			// Append the assembly to the list
			tpaList << directory << '\\' << findData.cFileName << PATH_DELIMITER;
		} while (FindNextFileA(fileHandle, &findData));
		FindClose(fileHandle);
	}
	return tpaList.str();
}


void CoreEngine::CompileScriptAssembly()
{
#ifdef _DEBUG
	std::cout << "[Hot Reload] Working dir: " << std::filesystem::current_path() << std::endl;
#endif

	// Calculate from current working dir (wherever we are)
	auto currentDir = std::filesystem::current_path();

	// Find and go into the RootFolderName folder :p
	auto baseEngineDir = currentDir;
	while (baseEngineDir.filename() != RootFolderName)
	{
		auto parent = baseEngineDir.parent_path();
		if (parent.empty() || parent == baseEngineDir)
		{
			break;
		}
		baseEngineDir = parent;
	}

#ifdef _DEBUG
	std::cout << "[Hot Reload] BaseEngine dir: " << baseEngineDir << std::endl;
#endif

	auto projectPath = FilePathToGame / "ManagedScripts" / "ManagedScripts.csproj";

#ifdef _DEBUG
	std::cout << "[Hot Reload] Project path: " << projectPath << std::endl;
#endif

	if (!std::filesystem::exists(projectPath))
	{
		std::cerr << "[Hot Reload] ERROR: Project not found!" << std::endl;
		return;
	}

	std::cout << "[Hot Reload] Working dir: " << std::filesystem::current_path() << std::endl;


	// Change the Configuration depending on mode
	std::string configuration = "Debug";
#ifdef NDEBUG
	configuration = "Release";
#endif

	std::wstring configWStr(configuration.begin(), configuration.end());
	std::wstring buildCmd = L" build \"" + projectPath.wstring() + L"\" -c " + configWStr + L" -p:Platform=x64";

	std::cout << "[Hot Reload] Working dir: " << std::filesystem::current_path() << std::endl;

	// Define the struct to config the compiler process call
	STARTUPINFOW startInfo;
	PROCESS_INFORMATION pi;
	ZeroMemory(&startInfo, sizeof(startInfo));
	ZeroMemory(&pi, sizeof(pi));
	startInfo.cb = sizeof(startInfo);

	std::cout << "[Hot Reload] Working dir: " << std::filesystem::current_path() << std::endl;

	// Start compiler process
	const auto Launch = CreateProcessW(L"C:\\Program Files\\dotnet\\dotnet.exe", 
		                              buildCmd.data(),nullptr, nullptr, true, NULL, nullptr, nullptr,
		                              &startInfo, &pi);

	std::cout << Launch << "\n\n\n\n\n";

	// Check that we launched the process
	if (!Launch)
	{	
		auto err = GetLastError();
		std::cerr << "[CoreEngine] ERROR: Failed to launch dotnet.exe. Error: 0x" << std::hex << err << std::endl;
		return;
	}

	// Wait for process to end
	DWORD exitCode{};
	while (true)
	{
		const auto EXEC_SUCCESS = GetExitCodeProcess(pi.hProcess, &exitCode);
		
		if (!EXEC_SUCCESS)
		{
			auto err = GetLastError();
			std::cerr << "[CoreEngine] ERROR: Failed to query build process. Error: 0x" << std::hex << err << std::endl;
			return;
		}

		if (exitCode != STILL_ACTIVE)
		{
			break;
		}
	}

	if (exitCode == 0)
	{
#ifdef _DEBUG
		std::cout << "[CoreEngine] Scripts compiled successfully!\n";
#endif

		// Copy ManagedScripts.dll next to the currently running executable. And 2 make it not hard coded just do in bin then ask 2 grab if needed
		std::filesystem::path dllSource = baseEngineDir / "build" / ".bin" / (configuration + "-x64") / "ManagedScripts.dll";

		// Get the directory of the running executable(all exe yippeeeeeeeeeeeeeee)
		char exePathBuf[MAX_PATH];
		GetModuleFileNameA(nullptr, exePathBuf, MAX_PATH);
		std::filesystem::path dllDest = std::filesystem::path(exePathBuf).parent_path() / "ManagedScripts.dll";

		if (!std::filesystem::exists(dllSource))
		{
			std::cerr << "[Hot Reload] ERROR: Built DLL not found at: " << dllSource << std::endl;
			return;
		}

		try
		{
			// just 1 place and they all copy
			std::filesystem::copy_file(dllSource, dllDest, std::filesystem::copy_options::overwrite_existing);
#ifdef _DEBUG
			std::cout << "[CoreEngine] ManagedScripts.dll copied to: " << dllDest << "\n";
#endif
		}

		catch (const std::exception& e)
		{
			std::cerr << "[Hot Reload] ERROR copying DLL: " << e.what() << std::endl;
		}
	}

	else
	{
		std::cerr << "[CoreEngine] Build failed! Exit code: " << exitCode << "\n";
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}


void APIENTRY glDebugOutput(GLenum source,
	GLenum type,
	unsigned int id,
	GLenum severity,
	GLsizei,
	const char* message,
	const void* userParam)
{
	(void)userParam;
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
		return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:
		std::cout << "Source: API";
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		std::cout << "Source: Window System";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		std::cout << "Source: Shader Compiler";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		std::cout << "Source: Third Party";
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		std::cout << "Source: Application";
		break;
	case GL_DEBUG_SOURCE_OTHER:
		std::cout << "Source: Other";
		break;
	}
	std::cout << std::endl;

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
		std::cout << "Type: Error";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		std::cout << "Type: Deprecated Behaviour";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		std::cout << "Type: Undefined Behaviour";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		std::cout << "Type: Portability";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		std::cout << "Type: Performance";
		break;
	case GL_DEBUG_TYPE_MARKER:
		std::cout << "Type: Marker";
		break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		std::cout << "Type: Push Group";
		break;
	case GL_DEBUG_TYPE_POP_GROUP:
		std::cout << "Type: Pop Group";
		break;
	case GL_DEBUG_TYPE_OTHER:
		std::cout << "Type: Other";
		break;
	}
	std::cout << std::endl;

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		std::cout << "Severity: high";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		std::cout << "Severity: medium";
		break;
	case GL_DEBUG_SEVERITY_LOW:
		std::cout << "Severity: low";
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		std::cout << "Severity: notification";
		break;
	}
	std::cout << std::endl;
	std::cout << std::endl;
}

CoreEngine::CoreEngine() : window(nullptr), DebugMode(false)
{
	LastTime = 0;
	GameRunning = false;
}

CoreEngine::~CoreEngine()
{
	if (window)
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	//clean upo for c#scripting
	auto shutdownFunc = GetFunctionPtr<void(*)()>(
		"ScriptAPI",
		"ScriptAPI.EngineInterface",
		"Shutdown"
	);

	if (shutdownFunc)
	{
		shutdownFunc();
	}
}

// Call in init to get the game name fro ActiveGame.json and load that game for editing/playing.
// then set the game file director(FilePathToGame), if it has not been set yet.
static void EnsureGameProjectPath()
{
	// already set, fuck off from here
	if (!FilePathToGame.empty()) 
	{
		return;
	}

	// Find the PulseEngine.dll in the build folder
	char exePath[MAX_PATH];
	GetModuleFileNameA(GetModuleHandleA("PulseEngine.dll"), exePath, MAX_PATH);

	// Go back 1 file path to get the json file that is next to the exe.
	std::filesystem::path currentDir = std::filesystem::path(exePath).parent_path();

	// Try to find the RootFolderName folder by walking up (debug/editor build)
	auto baseEngineDir = currentDir;
	while (baseEngineDir.filename() != RootFolderName)
	{
		auto parent = baseEngineDir.parent_path();
		if (parent.empty() || parent == baseEngineDir)
		{
			break;
		}
		baseEngineDir = parent;
	}

	if (baseEngineDir.filename() == RootFolderName)
	{
		// Debug build: found BaseEngine, read ActiveGame.json to get the active game
		std::string activeGame = "PulseProtocol"; // if ActiveGame.json is missing

		std::filesystem::path ConfigFile = baseEngineDir / "ActiveGame.json";
		if (std::filesystem::exists(ConfigFile))
		{	
			//deseralize game name :>
			std::ifstream f(ConfigFile);
			if (f.is_open())
			{
				rapidjson::IStreamWrapper isw(f);
				rapidjson::Document doc;
				doc.ParseStream(isw);
				if (doc.IsObject() && doc.HasMember("ActiveGame") && doc["ActiveGame"].IsString())
				{
					activeGame = doc["ActiveGame"].GetString();
				}
			}
		}

		// FilePathToGame = BaseEngine / Game Name 
		FilePathToGame = baseEngineDir / activeGame;
	}

	else if (std::filesystem::exists(currentDir / "JSON"))
	{
		// Release build: no BaseEngine found, game assets are beside the exe
		FilePathToGame = currentDir;
	}
	
	else
	{
		// Last resort: just use the folder the DLL is in
		FilePathToGame = currentDir;
	}

#ifdef _DEBUG
	std::cout << "[CoreEngine] Active game project: " << FilePathToGame << std::endl;
#endif
}

void CoreEngine::Init()
{
	timeBeginPeriod(1); // Set Windows timer resolution to 1ms for accurate Sleep() in frame cap

	// Get and set the file directory for game name(FilePathToGame)
	EnsureGameProjectPath();

	// FilePathToGame is already set correctly by EnsureGameProjectPath() above.
	// It handles both dev (BaseEngine/GameName) and release (exe folder) cases.
	std::filesystem::path configPath = FilePathToGame / "JSON" / "config.json";

	config.SetConfigPath(configPath.string());

	/*Config load window width and height*/
	if (!LoadConfig(config, config.GetConfigPath()))
	{
		std::cerr << "Failed to load config!\n";
	}
	else
	{
		std::cout << "Load config From FilePath: " << std::filesystem::absolute(config.GetConfigPath()) << "\n";
	}

	WindowWidth = config.GetWindowWidth();
	WindowHeight = config.GetWindowHeight();

	std::cout << "WindowWidth is: " << WindowWidth << "\n";
	std::cout << "WindowHeight is: " << WindowHeight << "\n";

	// m_startupFullscreen is set by the entry point (GameMain/EditorMain) via
	// SetStartupFullscreen() before Init() is called. Do NOT read it from
	// user_settings.json here — that would overwrite the per-project default
	// (game = fullscreen, editor = windowed).
	std::cout << "[CoreEngine] Startup fullscreen: "
		<< (m_startupFullscreen ? "true" : "false") << "\n";

	///*saving config data*/ //tested saving works
	// std::string newConfigPath = "../../PulseEngine/JSON/config2.json";
	// config.SetWindowWidth(800);
	// config.SetWindowHeight(600);
	// std::cout << "set WindowWidth is: " << config.GetWindowWidth() << "\n";
	// std::cout << "set WindowHeight is: " << config.GetWindowHeight() << "\n";
	// if (!SaveConfig(config, newConfigPath))
	//{
	//	std::cerr << "Failed to load config!\n";
	// }
	// else
	//{
	//	std::cout << "save config to FilePath: " << std::filesystem::absolute(newConfigPath) << "\n";
	// }

	if (!glfwInit())
	{
		//spdlog::error("Failed to initialize GLFW");
		return;
	}

	unsigned int version = 0;
	FMOD::System* system = nullptr;

	FMOD::System_Create(&system);
	system->getVersion(&version);

	std::cout << "FMOD version: "
		<< (version >> 16) << "."
		<< ((version >> 8) & 0xFF) << "."
		<< (version & 0xFF) << std::endl;

	system->release();

	//// --- Logging Setup ---
	// try {
	//	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	//	auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
	//		"logs/engine.log", 1024 * 1024 * 5, 3);
	//	// 5 MB max per file, keep 3 backups

	//	std::vector<spdlog::sink_ptr> sinks{ console_sink, file_sink };
	//	auto logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
	//	spdlog::set_default_logger(logger);

	//	spdlog::set_level(spdlog::level::debug); // debug/info/warn/error
	//	spdlog::flush_on(spdlog::level::info);

	//	spdlog::info("=== Engine Logging Initialized ===");
	//}
	// catch (const spdlog::spdlog_ex& ex) {
	//	std::cerr << "Log init failed: " << ex.what() << std::endl;
	//}
	//// ----------------------
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	window = glfwCreateWindow(WindowWidth, WindowHeight, WindowTitle, nullptr, nullptr);
	if (!window)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(window);

	m_windowedX = 100; m_windowedY = 100;
	m_windowedW = WindowWidth; m_windowedH = WindowHeight;

	if (m_startupFullscreen)
	{
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
		m_isFullscreen = true;
	}

	// --- Scroll wheel callback (for Editor + ImGui) ---
	glfwSetScrollCallback(window,
		[](GLFWwindow*, double /*x*/, double y)
		{
			g_scrollY += y;
		});


	glfwSetCharCallback(window, [](GLFWwindow*, unsigned int c)
		{
			auto input = Coordinator::GetInstance()->GetSystem<InputManager>();
			if (input)
				input->pushChar(c);
	});

	// Pause/resume on focus change (covers ALT+TAB and returning from Ctrl+Alt+Del)
	glfwSetWindowFocusCallback(window,
		[](GLFWwindow* win, int focused)
		{
			(void)win;
			if (!Engine) return;
			if (focused)
				Engine->SetPaused(false, PauseSource::Focus);
			else
				Engine->SetPaused(true, PauseSource::Focus);
		});

	// Also pause when minimized / restored
	glfwSetWindowIconifyCallback(window,
		[](GLFWwindow* win, int iconified)
		{
			(void)win;
			if (!Engine) return;
			if (iconified == GLFW_TRUE)
				Engine->SetPaused(true, PauseSource::Focus);
			// When restored, focus callback will unpause if the window regains focus
		});


	glfwSetWindowAspectRatio(window, 16, 9);
	glfwSetFramebufferSizeCallback(window,
		[](GLFWwindow*, int fbw, int fbh)
		{
			if (fbw <= 0 || fbh <= 0)
				return;
			GLApp::ComputeLetterbox(fbw, fbh);

			WindowWidth = fbw;
			WindowHeight = fbh;

			if (auto g = Coordinator::GetInstance()->GetSystem<GraphicsManager>())
			{
				g->OnResize(fbw, fbh);
			}
		});

	int fbw = 0, fbh = 0;
	glfwGetFramebufferSize(window, &fbw, &fbh);

	WindowWidth = fbw;
	WindowHeight = fbh;

	glewInit();
	DebugDraw::Init();

	GLApp::ComputeLetterbox(fbw, fbh);

	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}

	// Just incase, do again.
	EnsureGameProjectPath();

	// Check were we are before we set the asset path.
	std::cout << "[DEBUG] Setting asset paths, FilePathToGame = " << FilePathToGame << std::endl;
	mAssets.getRegistry().SetAssetPaths(FilePathToGame);

	std::cout << "[DEBUG] Testing path resolution:" << std::endl;
	std::cout << "[DEBUG] Base path: " << FilePathToGame << std::endl;

	// Load manifests
	mAssets.Initialize();

	// Test if path resolution works
	if (mAssets.getRegistry().FindTexture("MainMenu_BG")) {
		std::string rawPath = mAssets.getRegistry().FindTexture("MainMenu_BG")->path;
		std::string resolvedPath = mAssets.getRegistry().GetTextureItemPath("MainMenu_BG");
		std::cout << "[DEBUG] MainMenu_BG raw path: " << rawPath << std::endl;
		std::cout << "[DEBUG] MainMenu_BG resolved path: " << resolvedPath << std::endl;
	}
	//mAssets.getRegistry().SetAssetPaths(pulseEnginePath);
	//mAssets.Initialize(); //loaded registry inside
	// Singleton for Engine, but once game use EventBus/messaging system
	// PLEASE CALL THIS EVERY SINGLE U WANT 2 USE SINGLETON
	auto g_coordinator = Coordinator::GetInstance();

	auto g_inputSystem = g_coordinator->GetSystem<InputManager>();
	//auto g_rhythmSystem = g_coordinator->GetSystem<RhythmGameplayInput>();
	auto g_audioSystem = g_coordinator->GetSystem<AudioManager>();		//default cons
	auto g_graphicsSystem = g_coordinator->GetSystem<GraphicsManager>(); //default cons
	auto g_CollisionManager = g_coordinator->GetSystem<CollisionManager>();

	g_audioSystem->SetRegistry(&mAssets.getRegistry());
	g_graphicsSystem->SetRegistry(&mAssets.getRegistry());

	mAssets.SetAudioManager(g_audioSystem.get());
	mAssets.SetGraphicsManager(g_graphicsSystem.get());

	//g_graphicsSystem->GM_FindFont("PatrickHand_font");
	//g_graphicsSystem->GM_FindFont("liberation_font");

	MessageManager::GetInstance();
	std::cout << "[MessageManager] Initialized" << std::endl;

	// Initialize systems AFTER GL is ready
	g_inputSystem->Initialize(window);
	//g_rhythmSystem->Initialize();
	g_graphicsSystem->Initialize();
	g_audioSystem->Initialize();
	//g_coordinator->InitializeAllSystems();

	g_graphicsSystem->GM_FindFont("PatrickHand_font");
	g_graphicsSystem->GM_FindFont("liberation_font");

	ProfilingManager::GetInstance()->SetResetInterval(60.0);
	ProfilingManager::GetInstance()->SetDisplayUpdateInterval(1.0);

	// GLuint barTexture = LoadTexture("../../PulseEngine/Assets/Sprites/perfect.png");
	// if (barTexture > 0) {
	//	Entity box2 = g_inputSystem->GetFeedbackEntity2();  // Use getter
	//	auto& transform2 = g_coordinator->GetComponent<Framework::Transform>(box2);
	//	transform2.textureID = barTexture;
	//	std::cout << "Texture loaded and assigned to box 2: " << barTexture << std::endl;
	// }

	GameRunning = true;	 // 3.. 2.. 1.. GAME START
	startScriptEngine(); // 3.. 2.. 1.. DLL SCRIPT! START ROLLINGGGGGGGGGGGGGGGGGG

	// ManagedScripts.dll is now built and copied by CMake POST_BUILD.
	// CompileScriptAssembly() is only called at runtime for hot-reload (R key in Debug).


	std::cout << "[Hot Reload] Working dir: " << std::filesystem::current_path() << std::endl;
	// Step 1. Get the function ptrs
	InitFunctionPtr initFunc = GetFunctionPtr<InitFunctionPtr>
		(
			"ScriptAPI",                 // Name of the Assembly
			"ScriptAPI.EngineInterface", // Full name of the class
			"Init"                 // Name of the function
		);

	//auto addScript = GetFunctionPtr<bool(*)(int, const char*)>
	//	(
	//		"ScriptAPI",
	//		"ScriptAPI.EngineInterface",
	//		"AddScriptViaName"
	//	);
	clearScriptsFunc = GetFunctionPtr<void(*)(void)>
		(
			"ScriptAPI",
			"ScriptAPI.EngineInterface",
			"ClearAllScripts"
		);

	// Step 2: Initialize it with  pointer to CoreEngine as param
	initFunc(this); 

	g_coordinator->InitializeAllSystems();
}

void CoreEngine::Setup()
{
	// 3 in 1 system
	Coordinator::GetInstance()->init();

	auto g_coordinator = Coordinator::GetInstance();

	// Register your component here
	g_coordinator->RegisterComponent<Framework::Transform>();
	g_coordinator->RegisterComponent<Framework::Renderable>();
	g_coordinator->RegisterComponent<Framework::Animation>();
	g_coordinator->RegisterComponent<NPC>();
	g_coordinator->RegisterComponent<Name>();
	g_coordinator->RegisterComponent<AABB>();
	g_coordinator->RegisterComponent<Health>();
	g_coordinator->RegisterComponent<Parent>();
	g_coordinator->RegisterComponent<LayerTag>();
	g_coordinator->RegisterComponent<Children>();
	g_coordinator->RegisterComponent<AudioSource>();
	g_coordinator->RegisterComponent<TextComponent>();
	g_coordinator->RegisterComponent<LogicComponent>();
	g_coordinator->RegisterComponent<PrefabInstance>();
	g_coordinator->RegisterComponent<NamingComponent>();

	// Register your systems here, ORDER = UPDATE ORDER every frame
	// 1. Input   -read raw input first
	// 2. FSM     -state machine reacts to input
	// 3. Audio   -FMOD ticks, fresh position ready for Conductor
	// 4. Collision
	// 5. Scene
	// 6. Logic   -C# scripts run, Conductor reads fresh audio position
	// 7. Graphics -renders the final game state last
	g_coordinator->RegisterSystem<InputManager>();
	g_coordinator->RegisterSystem<LogicSystem>();
	g_coordinator->RegisterSystem<GraphicsManager>();  
	g_coordinator->RegisterSystem<AudioManager>();
	g_coordinator->RegisterSystem<CollisionManager>();
	g_coordinator->RegisterSystem<SceneManager>();

	// for clearing script when scene change. pass an instance of scenemanager
	auto sceneManager = g_coordinator->GetSystem<SceneManager>();
	sceneManager->SetCoreEngine(this);

	// Set signature for component and system
	Signature collisionSig;
	collisionSig.set(g_coordinator->GetComponentType<Framework::Transform>());
	collisionSig.set(g_coordinator->GetComponentType<AABB>());
	collisionSig.set(g_coordinator->GetComponentType<Health>());
	g_coordinator->SetSystemSignature<CollisionManager>(collisionSig);

	Signature inputSig;
	inputSig.set(g_coordinator->GetComponentType<Framework::Transform>());
	inputSig.set(g_coordinator->GetComponentType<Framework::Renderable>());
	inputSig.set(g_coordinator->GetComponentType<Framework::Animation>());
	g_coordinator->SetSystemSignature<InputManager>(inputSig);

	//Signature rhythmSig;
	//rhythmSig.set(g_coordinator->GetComponentType<Framework::Transform>());
	//rhythmSig.set(g_coordinator->GetComponentType<Framework::Renderable>());
	//rhythmSig.set(g_coordinator->GetComponentType<Framework::Animation>());
	//g_coordinator->SetSystemSignature<RhythmGameplayInput>(rhythmSig);

	//Signature UISig;
	//UISig.set(g_coordinator->GetComponentType<Framework::Transform>());
	//UISig.set(g_coordinator->GetComponentType<Framework::Renderable>());
	//UISig.set(g_coordinator->GetComponentType<Name>());
	//UISig.set(g_coordinator->GetComponentType<LayerTag>());
	//g_coordinator->SetSystemSignature<GUIManager>(UISig);

	Signature graphicsSig;
	graphicsSig.set(g_coordinator->GetComponentType<Framework::Transform>());  // <-- needs Transform
	graphicsSig.set(g_coordinator->GetComponentType<Framework::Renderable>()); // <-- needs Renderable
	graphicsSig.set(g_coordinator->GetComponentType<Framework::Animation>());  // <-- needs Animation
	graphicsSig.set(g_coordinator->GetComponentType<LayerTag>());
	g_coordinator->SetSystemSignature<GraphicsManager>(graphicsSig);

	Signature sceneSig;
	sceneSig.set(g_coordinator->GetComponentType<NPC>());
	sceneSig.set(g_coordinator->GetComponentType<AABB>());
	sceneSig.set(g_coordinator->GetComponentType<Health>());
	sceneSig.set(g_coordinator->GetComponentType<NamingComponent>());
	sceneSig.set(g_coordinator->GetComponentType<Framework::Transform>());
	sceneSig.set(g_coordinator->GetComponentType<Framework::Renderable>());
	sceneSig.set(g_coordinator->GetComponentType<Framework::Animation>());
	g_coordinator->SetSystemSignature<SceneManager>(sceneSig);

	Signature LogicSig;
	LogicSig.set(g_coordinator->GetComponentType<LogicComponent>());
	g_coordinator->SetSystemSignature<LogicSystem>(LogicSig);
}

void CoreEngine::Update(float dt)
{
	auto g_coordinator = Coordinator::GetInstance();
	auto g_audioSystem = g_coordinator->GetSystem<AudioManager>();
	auto g_inputSystem = g_coordinator->GetSystem<InputManager>();
	//auto g_rhythmSystem = g_coordinator->GetSystem<RhythmGameplayInput>();
	auto g_graphicsSystem = g_coordinator->GetSystem<GraphicsManager>();
	auto g_sceneSystem = g_coordinator->GetSystem<SceneManager>();
	auto profiler = ProfilingManager::GetInstance();

	// Always process engine / message stuff
	MessageManager::GetInstance()->ProcessQueue(glfwGetTime());

	// update all the system
	profiler->StartSystem("UpdateAllSystems");
	g_coordinator->UpdateAllSystems(dt);
	profiler->EndSystem("UpdateAllSystems");


	profiler->StartSystem("Graphics System");
	if (g_graphicsSystem)
		g_graphicsSystem->Update();
	profiler->EndSystem("Graphics System");

	//--- Start of script/hot reload ---//
	// Press R to reload the script changes 
	static bool wasRPressed = false;

#ifdef _DEBUG
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
	{
		if (!wasRPressed)
		{
#ifdef _DEBUG
			std::cout << "[Hot Reload] Recompiling scripts...\n";
#endif
			CompileScriptAssembly();

			//change the working directory so that i can get the .net stuff for hot reload else I will skin my pet goldfish alive
			HMODULE hEngine = GetModuleHandleA("PulseEngine.dll");
			if (hEngine)
			{
				std::string debugPath(MAX_PATH, '\0');
				GetModuleFileNameA(hEngine, debugPath.data(), MAX_PATH);
				PathRemoveFileSpecA(debugPath.data());
				debugPath.resize(std::strlen(debugPath.data()));

#ifdef _DEBUG
				std::cout << "[Hot Reload] Setting working dir to: " << debugPath << std::endl;
#endif
				std::filesystem::current_path(debugPath);
			}


			auto reloadScripts = GetFunctionPtr<void(*)(void)>
				(
					"ScriptAPI",
					"ScriptAPI.EngineInterface",
					"Reload"
				);

			reloadScripts();

		}

		// Add scene Component to 
		auto SceneSystem = g_coordinator->GetSystem<SceneManager>();
		auto addScriptFunc = GetFunctionPtr<bool(*)(int, const char*)>(
			"ScriptAPI",
			"ScriptAPI.EngineInterface",
			"AddScriptViaName"
		);

		for (Entity entity : SceneSystem->GetSceneEntities())
		{
			if (g_coordinator->HasComponent<LogicComponent>(entity))
			{
				auto const& logic = g_coordinator->GetComponent<LogicComponent>(entity);
				for (auto const& script : logic.ScriptName)
				{
					addScriptFunc(entity, script.c_str());
				}
			}
		}

		wasRPressed = true;
	}

	else
	{
		//std::cout << "back to normal\n";
		wasRPressed = false;
	}
#endif // !_DEBUG

	//move it to gamelogic.cpp
	/*auto executeUpdate = GetFunctionPtr<void(*)(void)>
		(
			"ScriptAPI",
			"ScriptAPI.EngineInterface",
			"ExecuteUpdate"
		);
	executeUpdate();*/
	//--- End of script/hot reload ---//

	if (m_paused)
	{
		return;
	}

}

void CoreEngine::GameLoop()
{
    //LastTime = timeGetTime();		
	LastFrame = timeGetTime();

    auto profiler = ProfilingManager::GetInstance();

    while (!glfwWindowShouldClose(window) && GameRunning)
    {
        // --- dt calculation ---
        //unsigned currenttime = timeGetTime();
        //float dt = (currenttime - LastTime) / 1000.0f;
        //LastTime = currenttime;

		CurrentFrame = timeGetTime();
		DeltaTime = (CurrentFrame - LastFrame) / 1000.f;
		LastFrame = CurrentFrame;

		// --- count frames and time ---
		FrameCount++;
		TimePassed += DeltaTime;

        glfwPollEvents();

        // --- Game update (ECS systems) ---
        profiler->StartFrame();
        Update(DeltaTime);
        profiler->EndFrame();

        // --- Game rendering ---
        RenderDebugDraw();      // your OpenGL game draw

        // --- Editor (ImGui) on top of game ---
        if (m_editorFrameCallback)
        {
            m_editorFrameCallback();   // calls NewFrame + Draw + Render
        }

        // --- Present frame ---
        glfwSwapBuffers(window);

        // --- 60 FPS cap ---
        const unsigned TARGET_FRAME_MS = 16; // ~60fps (16.67ms)
        unsigned frameElapsed = timeGetTime() - CurrentFrame;
        if (frameElapsed < TARGET_FRAME_MS) {
            Sleep(TARGET_FRAME_MS - frameElapsed);
        }

        profiler->Reset();
    }
}


void CoreEngine::Exit()
{
	timeEndPeriod(1); // Restore Windows timer resolution

	stopScriptEngine(); // stop the Csharp Scripts from using "bridge". iykyk

	auto g_coordinator = Coordinator::GetInstance();

	auto g_inputSystem = g_coordinator->GetSystem<InputManager>();
	//auto g_rhythmSystem = g_coordinator->GetSystem<RhythmGameplayInput>();
	auto g_audioSystem = g_coordinator->GetSystem<AudioManager>();
	auto g_graphicsSystem = g_coordinator->GetSystem<GraphicsManager>();

	if (g_inputSystem)
		g_inputSystem->Shutdown();

	//if (g_rhythmSystem)
	//	g_rhythmSystem->Shutdown();

	if (g_audioSystem)
		g_audioSystem->Shutdown();

	if (g_graphicsSystem)
		g_graphicsSystem->Shutdown();

	g_coordinator->ExitAllSystems();

	DebugDraw::Shutdown();
	MessageManager::DestroyInstance();
	ProfilingManager::DestroyInstance();

	if (window)
	{
		glfwDestroyWindow(window);
		window = nullptr;
	}
	glfwTerminate();

}

void CoreEngine::RenderDebugDraw()
{
	auto g_coordinator = Coordinator::GetInstance();
	auto g_inputSystem = g_coordinator->GetSystem<InputManager>();
	if (!g_inputSystem)
		return;

	// Match the camera used by sprite rendering so debug boxes align with sprites.
	// Sprites are drawn with ZoomNDC applied in NDC space (zoom * ndc + center_ndc),
	// so the equivalent ortho half-extents are VIRTUAL / (2 * zoom), and the ortho
	// center in world space is -(cam.center_ndc * halfExtent).
	const auto& cam = Framework::GameState::IsPlaying() ? GLApp::gGameCamera : GLApp::gEditorCamera;
	const float halfW = static_cast<float>(GLApp::VIRTUAL_W) / (2.0f * cam.zoom);
	const float halfH = static_cast<float>(GLApp::VIRTUAL_H) / (2.0f * cam.zoom);
	const float cx = -cam.center.x * halfW;
	const float cy = -cam.center.y * halfH;

	glm::mat4 projection = glm::ortho(cx - halfW, cx + halfW, cy - halfH, cy + halfH);
	DebugDraw::Begin(projection);

	// --- VFX Particles ---
	// Render active particles from the VFX system as small shrinking dots.
	// Particles carry world-space positions, which match the DebugDraw projection.
	if (Framework::GameState::IsPlaying())
	{
		if (auto logic = g_coordinator->GetSystem<LogicSystem>())
		{
			for (const auto& p : logic->GetVfx().GetParticles())
			{
				if (!p.active) continue;
				if (p.textureID != 0) continue; // textured particles rendered via gBatch in glapp
				float lifeRatio = (p.lifeMax > 0.0f) ? (p.life / p.lifeMax) : 0.0f;
				float radius = p.size * lifeRatio; // shrinks to 0 as particle expires
				glm::vec3 color = { 1.0f, 0.75f + 0.25f * lifeRatio, 0.2f * lifeRatio };
				DebugDraw::DrawCircleFilled({ p.pos.x, p.pos.y }, radius, color, 6);
			}
		}
	}

	// F1 - DrawRect
	if (g_inputSystem->isDebugRectsEnabled())
	{
		for (auto& entity : g_inputSystem->DebugEntities)
		{
			auto& transform = g_coordinator->GetComponent<Framework::Transform>(entity);
			glm::vec2 center = { transform.Pos.x, transform.Pos.y };
			glm::vec2 half = 0.5f * glm::vec2(transform.Scale.x, transform.Scale.y);
			glm::vec2 min = center - half;
			glm::vec2 max = center + half;

			DebugDraw::DrawRect(min, max, { 1.0f, 1.0f, 0.0f }); // Yellow
		}
	}

	// F2 - DrawLine
	if (g_inputSystem->isDebugLinesEnabled())
	{
		DebugDraw::DrawLine({ -800.0f, -450.0f }, { 800.0f, 450.0f }, { 1.0f, 0.0f, 0.0f });
		DebugDraw::DrawLine({ 800.0f, -450.0f }, { -800.0f, 450.0f }, { 0.0f, 1.0f, 0.0f });
		DebugDraw::DrawLine({ 0.0f, -450.0f }, { 0.0f, 450.0f }, { 0.0f, 0.0f, 1.0f });
		DebugDraw::DrawLine({ -800.0f, 0.0f }, { 800.0f, 0.0f }, { 1.0f, 1.0f, 0.0f });
	}

	// F3 - DrawCircle
	if (g_inputSystem->isDebugCirclesEnabled())
	{
		DebugDraw::DrawCircle({ 0.0, 0.0f }, 200.0f, { 1.0f, 0.0f, 1.0f }, 32);
		DebugDraw::DrawCircle({ 400.0f, 300.0f }, 100.0f, { 0.0f, 1.0f, 1.0f }, 24);
	}

	// F4 - DrawCircleFilled
	if (g_inputSystem->isDebugCirclesFilledEnabled())
	{
		DebugDraw::DrawCircleFilled({ 100.0f, 300.0f }, 80.0f, { 1.0f, 0.5f, 0.0f }, 24);
		DebugDraw::DrawCircleFilled({ 100.0f, -300.0f }, 60.0f, { 0.5f, 0.0f, 1.0f }, 16);
	}

	// F5 - DrawPoint
	if (g_inputSystem->isDebugPointsEnabled())
	{
		for (float x = -800.0f; x < 800.0f; x += 200.0f)
		{
			for (float y = -450.0f; y < 450.0f; y += 200.0f)
			{
				DebugDraw::DrawPoint({ x, y }, 5.0f, { 1.0f, 1.0f, 0.0f });
			}
		}
	}

	// F6 - Grid
	if (g_inputSystem->isDebugGridEnabled())
	{
		for (float x = -800.0f; x < 800.0f; x += 100.0f)
		{
			DebugDraw::DrawLine({ x, -450.0f }, { x, 450.0f }, { 0.3f, 0.3f, 0.3f });
		}
		for (float y = -450.0f; y < 450.0f; y += 100.0f)
		{
			DebugDraw::DrawLine({ -800.0f, y }, { 800.0f, y }, { 0.3f, 0.3f, 0.3f });
		}
	}

	DebugDraw::End(); // end world-space pass (F1-F6, particles)

	// F7 - Collision AABB Debug
	// Uses the sprite's own mdl_to_ndc_xform so the box perfectly tracks the sprite.
	// AABB min/max are stored in /50 design units (50 units = one Scale dimension),
	// which equals model-space when divided by 50 (model quad is -0.5..0.5).
	// We submit pre-computed NDC positions, so DebugDraw uses an identity MVP.
	if (g_inputSystem->isDebugCollisionEnabled())
	{
		DebugDraw::Begin(glm::mat4(1.0f)); // identity — vertices already in NDC

		auto collisionSystem = g_coordinator->GetSystem<CollisionManager>();
		if (collisionSystem)
		{
			// Same camera the sprite batch uses
			const glm::mat3 ZoomNDC = GLApp::BuildPanZoomNDC(cam);

			for (auto& entity : collisionSystem->EntityMember)
			{
				if (!g_coordinator->HasComponent<Framework::Transform>(entity) ||
					!g_coordinator->HasComponent<AABB>(entity))
					continue;

				auto& transform = g_coordinator->GetComponent<Framework::Transform>(entity);
				auto& baseAABB  = g_coordinator->GetComponent<AABB>(entity);

				// Build the same modelToNDC used by the sprite renderer
				glm::mat3 M        = glm::make_mat3(transform.mdl_to_ndc_xform.Begin());
				glm::mat3 modelNDC = ZoomNDC * M;

				// Convert /50 design units → model space (model quad spans -0.5..0.5)
				constexpr float INV50 = 1.0f / 50.0f;
				glm::vec2 corners[4] = {
					{ baseAABB.min.x * INV50, baseAABB.min.y * INV50 },
					{ baseAABB.max.x * INV50, baseAABB.min.y * INV50 },
					{ baseAABB.max.x * INV50, baseAABB.max.y * INV50 },
					{ baseAABB.min.x * INV50, baseAABB.max.y * INV50 },
				};

				// Transform each corner to NDC via the sprite's matrix
				glm::vec2 ndc[4];
				for (int i = 0; i < 4; ++i)
				{
					glm::vec3 h = modelNDC * glm::vec3(corners[i], 1.0f);
					ndc[i] = { h.x, h.y };
				}

				// Green box = collision box
				for (int i = 0; i < 4; ++i)
					DebugDraw::DrawLine(ndc[i], ndc[(i + 1) % 4], { 0.0f, 1.0f, 0.0f });

				// Red point = sprite center in NDC
				glm::vec3 c = modelNDC * glm::vec3(0.0f, 0.0f, 1.0f);
				DebugDraw::DrawPoint({ c.x, c.y }, 4.0f, { 1.0f, 0.0f, 0.0f });
			}
		}

		DebugDraw::End();
	}

	//// ---------------------------------------------------------------------
	//// F7: spawn/delete 1,000 moving sprites (hero)
	//// ---------------------------------------------------------------------
	//static std::vector<Entity>                   s_stressEntities;
	//static std::unordered_map<Entity, glm::vec2> s_velocity;
	//static double s_lastTick = glfwGetTime();

	//// to fix warnings, previously using same variables 2 times
	//float stressWorldH = 900.0f;
	//float stressAspect = (WindowHeight > 0) ? float(WindowWidth) / float(WindowHeight) : (16.0f / 9.0f);
	//float stressWorldW = stressWorldH * stressAspect;

	//if (g_inputsystem->consumestresstogglerequest())
	//{
	//	if (!s_stressentities.empty())
	//	{
	//		factory factory;
	//		for (entity e : s_stressentities) factory.deleteentity(e);
	//		s_stressentities.clear();
	//		s_velocity.clear();
	//		std::cout << "[stresstest] deleted spawned entities (f7)\n";
	//	}
	//	else
	//	{
	//		// resolve the real path for the 'hero' texture via the registry
	//		const std::string heropath = massets.gettexturepath("hero");  // e.g. ".../textures/characters/hero.png"
	//		const gluint sharedtex = massets.getorloadtexture(heropath);  // goes through graphicsmanager cache
	//		if (!sharedtex) {
	//			std::cerr << "[stresstest] failed to load texture: " << heropath << "\n";
	//		}

	//		//const float worldh = 900.0f;
	//		//const float aspect = (windowheight > 0)
	//		//	? float(windowwidth) / float(windowheight)
	//		//	: (16.0f / 9.0f);
	//		//const float worldw = worldh * aspect;

	//		std::mt19937 rng{ std::random_device{}() };
	//		std::uniform_real_distribution<float> posx(-stressworldw, stressworldw);
	//		std::uniform_real_distribution<float> posy(-stressworldh, stressworldh);
	//		std::uniform_real_distribution<float> vel(-220.0f, 220.0f);
	//		std::uniform_real_distribution<float> size(28.0f, 64.0f);

	//		s_stressentities.reserve(1000);
	//		for (int i = 0; i < 1000; ++i)
	//		{
	//			entity e = factory{}.defaultobj();
	//			s_stressentities.push_back(e);

	//			// transform
	//			auto& t = g_coordinator->getcomponent<framework::transform>(e);
	//			t.pos = { posx(rng), posy(rng) };
	//			float s = size(rng);
	//			t.scale = { s, s };
	//			t.rotation = { 0.0f, 0.0f };

	//			// renderable  use sprite pipeline + texture
	//			auto& r = g_coordinator->getcomponent<framework::renderable>(e);
	//			r.usetexture = (sharedtex != 0);
	//			r.textureid = sharedtex;          // gl handle already cached by gm
	//			r.spritename = heropath;           // use the same key string you loaded with
	//			r.mdl_ref = 1.0f;               // ? sprite quad model (not background/other model)
	//			r.shd_ref = 2.0f;               // ? sprite shader that samples the texture
	//			// r.tintcolor stays white by default

	//			// motion
	//			s_velocity[e] = { vel(rng), vel(rng) };
	//		}

	//		std::cout << "[stresstest] spawned 1000 entities with texture: " << heropath << " (f7)\n";
	//	}
	//}

	//if (!s_stressentities.empty())
	//{
	//	const double now = glfwgettime();
	//	const float dt = float(now - s_lasttick);
	//	s_lasttick = now;

	//	//const float worldh = 900.0f;
	//	//const float aspect = (windowheight > 0)
	//	//	? float(windowwidth) / float(windowheight)
	//	//	: (16.0f / 9.0f);
	//	//const float worldw = worldh * aspect;

	//	const float left = -stressworldw;
	//	const float right = stressworldw;
	//	const float bottom = -stressworldh;
	//	const float top = stressworldh;

	//	for (entity e : s_stressentities)
	//	{
	//		if (!g_coordinator->hascomponent<framework::transform>(e)) continue;

	//		auto& t = g_coordinator->getcomponent<framework::transform>(e);
	//		auto& v = s_velocity[e];

	//		t.pos.x += v.x * dt;
	//		t.pos.y += v.y * dt;

	//		const float halfw = t.scale.x * 0.5f;
	//		const float halfh = t.scale.y * 0.5f;

	//		if (t.pos.x - halfw < left) { t.pos.x = left + halfw; v.x = -v.x; }
	//		if (t.pos.x + halfw > right) { t.pos.x = right - halfw; v.x = -v.x; }
	//		if (t.pos.y - halfh < bottom) { t.pos.y = bottom + halfh; v.y = -v.y; }
	//		if (t.pos.y + halfh > top) { t.pos.y = top - halfh; v.y = -v.y; }
	//	}
	//}
	//// ---------------------------------------------------------------------



	//debugdraw::end();
}

void CoreEngine::SetPaused(bool paused)
{
	if (m_paused == paused) return;
	m_paused = paused;
}

void CoreEngine::SetPaused(bool paused, PauseSource source)
{
	if (source == PauseSource::Game)
	{
		// Player explicitly paused/unpaused — record intent
		m_manualPause = paused;
	}
	else if (source == PauseSource::Focus && !paused && m_manualPause)
	{
		// Window regained focus but player manually paused — keep paused
		return;
	}

	if (m_paused == paused) return;
	m_paused = paused;
}

 void CoreEngine::TogglePaused()
{
	SetPaused(!m_paused);
 }

void CoreEngine::PlayGame()
{
	m_isPlaying = true;
	m_paused = false;
	m_manualPause = false;
	Framework::GameState::SetPlaying(true);

	// RESET TIME!!!!
	FrameCount = 0;
	TimePassed = 0.0f;

	auto coord = Coordinator::GetInstance();

	// ALWAYS update the original scene path when Play is pressed
	if (auto sceneMgr = coord->GetSystem<SceneManager>())
	{
		std::string sceneName = sceneMgr->GrabCurrentScene();

		// Remove "Saved" prefix if it exists
		if (sceneName.find("Saved") == 0)
		{
			sceneName = sceneName.substr(5);
		}
		// Remove "Scene" suffix if it exists
		size_t scenePos = sceneName.find("Scene");
		if (scenePos != std::string::npos)
		{
			sceneName = sceneName.substr(0, scenePos);
		}

		// ALWAYS update this path, even if already set
		m_originalScenePath = (FilePathToGame / "JSON" / (sceneName + ".json")).string();

		std::cout << "[CoreEngine] Original scene path: " << m_originalScenePath << std::endl; // DEBUG
	}

	// Call Init() on all scripts
	/*if (auto logicSys = coord->GetSystem<LogicSystem>())
	{
		for (auto const& entity : logicSys->EntityMember)
		{
			auto& scriptname = coord->GetComponent<LogicComponent>(entity);
			auto script = logicSys->GrabScript(scriptname.ScriptName);
			if (script)
			{
				script->Init(entity);
			}
		}
	}*/
}

void CoreEngine::StopGame()
{
	m_isPlaying = false;
	m_paused = true;
	Framework::GameState::SetPlaying(false);
	auto coord = Coordinator::GetInstance();

	// Stop all audio (BGM and AudioSource components)
	if (auto audio = coord->GetSystem<AudioManager>())
	{
		//audio->StopBGM(); // Stop any direct BGM calls
		audio->StopPlayingAllAudio(); //Stop ALL audio

		// Stop all AudioSource components
		for (Entity entity = 0; entity < MaxEntity; ++entity)
		{
			if (coord->HasComponent<AudioSource>(entity))
			{
				auto& audioSrc = coord->GetComponent<AudioSource>(entity);
				if (audioSrc.channel)
				{
					audioSrc.channel->stop();
					audioSrc.channel = nullptr;
				}
			}
		}
	}
}

// this might have been causing the random crash cus its calling restartBGM() too
void CoreEngine::RestartRuntimeForPlay()
{
	//auto coord = Coordinator::GetInstance();
	//if (!coord) return;

	//// 1) AUDIO: force BGM to start at 0
	//if (auto audio = coord->GetSystem<AudioManager>()) {
	//	// if engine is paused (Stop button), keep audio paused too
	//	audio->RestartBGM();
	//}

	//// 2) RHYTHM: shutdown + initialize
	//if (auto rhythm = coord->GetSystem<RhythmGameplayInput>()) {
	//	rhythm->Shutdown();
	//	rhythm->Initialize();
	//}
}

void CoreEngine::PauseGame()
{
	m_isPlaying = false;
	m_paused = true;
	Framework::GameState::SetPlaying(false);
	auto coord = Coordinator::GetInstance();

	if (auto audio = coord->GetSystem<AudioManager>()) {
		audio->PauseBGM();
	}
}

void CoreEngine::ResumeGame()
{
	m_isPlaying = true;
	m_paused = false;
	Framework::GameState::SetPlaying(true);
	auto coord = Coordinator::GetInstance();

	if (auto audio = coord->GetSystem<AudioManager>()) {
		audio->ResumeBGM();
	}
}

void CoreEngine::SetEditorFrameCallback(const std::function<void()>& cb)
{
	m_editorFrameCallback = cb;
}

void CoreEngine::SetFullscreen(bool fullscreen)
{
	if (!window) return;
	if (m_isFullscreen == fullscreen) return;

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	if (fullscreen)
	{
		glfwGetWindowPos(window, &m_windowedX, &m_windowedY);
		glfwGetWindowSize(window, &m_windowedW, &m_windowedH);
		glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
		m_isFullscreen = true;
	}
	else
	{
		const int restoreW = (m_windowedW > 0) ? m_windowedW : config.GetWindowWidth();
		const int restoreH = (m_windowedH > 0) ? m_windowedH : config.GetWindowHeight();
		glfwSetWindowMonitor(window, nullptr, m_windowedX, m_windowedY, restoreW, restoreH, GLFW_DONT_CARE);
		m_isFullscreen = false;
	}

	// Refresh framebuffer sizes + inform render systems
	int fbw = 0, fbh = 0;
	glfwGetFramebufferSize(window, &fbw, &fbh);
	if (fbw > 0 && fbh > 0)
	{
		WindowWidth = fbw;
		WindowHeight = fbh;

		GLApp::ComputeLetterbox(fbw, fbh);

		if (auto g = Coordinator::GetInstance()->GetSystem<GraphicsManager>())
			g->OnResize(fbw, fbh);
	}

	// Persist immediately on every actual mode change (UI toggle, hotkey toggle, etc.).
	PersistFullscreenToUserSettings(FilePathToGame, m_isFullscreen);

	// Always keep cursor visible regardless of window mode
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}


void CoreEngine::ToggleFullscreen()
{
	SetFullscreen(!m_isFullscreen);
}

//std::filesystem::path CoreEngine::GetAssetRoot()
//{
//	// Find directory of PulseEngine.dll
//	std::string runtimePath(MAX_PATH, '\0');
//	HMODULE hEngine = GetModuleHandleA("PulseEngine.dll");
//	if (!hEngine) return std::filesystem::current_path(); // fallback
//
//	GetModuleFileNameA(hEngine, runtimePath.data(), MAX_PATH);
//	PathRemoveFileSpecA(runtimePath.data());
//	runtimePath.resize(std::strlen(runtimePath.data()));
//
//	// If your Assets folder is next to the dll:
//	// return std::filesystem::path(runtimePath) / "Assets";
//
//	// If Assets is inside BaseEngine/PulseEngine/Assets, adapt accordingly:
//	return std::filesystem::path(runtimePath) / "Assets";
//}


std::filesystem::path CoreEngine::GetAssetRoot()
{
	namespace fs = std::filesystem;

	// Find directory of PulseEngine.dll
	std::string runtimePath(MAX_PATH, '\0');
	HMODULE hEngine = GetModuleHandleA("PulseEngine.dll");
	if (!hEngine)
	{
		std::cerr << "[CoreEngine] ERROR: PulseEngine.dll not loaded. "
			"Falling back to current_path().\n";
		return fs::current_path();
	}

	GetModuleFileNameA(hEngine, runtimePath.data(), MAX_PATH);
	PathRemoveFileSpecA(runtimePath.data());
	runtimePath.resize(std::strlen(runtimePath.data()));
	fs::path dllDir = fs::path(runtimePath);

	std::cout << "[CoreEngine] dllDir = " << dllDir << "\n";

	// Walk upwards and search for the *source* Assets folder:
	//   BaseEngine/PulseProtocol/Assets
	// while skipping build output:
	//   BaseEngine/build/PulseProtocol/Assets
	fs::path cur = dllDir;
	for (int i = 0; i < 30; ++i)
	{
		fs::path candidate = cur / FilePathToGame.filename() / "Assets";
		if (fs::exists(candidate) && fs::is_directory(candidate))
		{
			fs::path resolved = fs::weakly_canonical(candidate);
			std::string s = resolved.string();

			// Skip build output assets
			const bool isBuild =
				(s.find("\\build\\") != std::string::npos) ||
				(s.find("/build/") != std::string::npos);

			if (!isBuild)
			{
				std::cout << "[CoreEngine] AssetRoot resolved to: " << resolved << "\n";
				return resolved;
			}
			else
			{
				std::cout << "[CoreEngine] Skipping build AssetRoot candidate: " << resolved << "\n";
			}
		}

		if (!cur.has_parent_path())
			break;

		cur = cur.parent_path();
	}

	// Final fallback (DLL-local Assets). This is usually build output.
	fs::path fallback = dllDir / "Assets";
	std::cerr << "[CoreEngine] WARNING: Could not find source AssetRoot. "
		"Using DLL-local Assets folder: " << fallback << "\n";
	return fallback;
}
