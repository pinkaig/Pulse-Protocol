/******************************************************************************/
/**
 * @file        CoreEngine.h
 * @project     Pulse Protocol
 * @author		Chia Wei Xuan Rachael - 90%
 *				Goh Pin Kai - 10%
 * @brief		Declares the core engine class that manages subsystem
 *				initialization, the main game loop, and development utilities.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/
#pragma once
// #include "../PulseEngine/Serialization/Serialization.h"
// #include "../PulseEngine/Resources/AssetRegistry.h"
// #include "../Asset/AssetsManager.h"
//#include "pch/pch.h"
#include "pch/pch_temp.h"
#include "Components/Health.h"
#include "Components/EntityType.h"
#include "CoreEngine/Asset/AssetsManager.h"

// for .net
#include <stdexcept>
#include "CoreEngine/Core/ImportExport.h"
#include "../extern/dotnet/include/coreclrhost.h" 
#pragma warning(push)
#pragma warning(disable: 4251) // stupid warning just saying to make sure we are using same compilier type and runtime MDd. WHICH WE ARE SO WHY WARNING STILL??????????

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 
#include <Windows.h>

// forward declaration
class Config;
class AssetManager;
namespace Framework { class Transform; }

class DLL_API CoreEngine
{
public:
	CoreEngine();
	~CoreEngine();

	enum class PauseSource { Game, Editor, Focus };

	// Engine 3 states
	void Init();
	void Setup();
	void Update(float dt);
	void GameLoop();
	void Exit();
	void HelloWorld() {
		std::cout << "Hello Native World!" << std::endl;
	}
	static Framework::Transform* GetTransformComponent(int ID);
	static std::filesystem::path GetAssetRoot();
	// void BroadcastMessage(Message* message);	  // One of the ways to glue systems tgt.
	// AssetsManager& getAssetsManager() { return mAssets; }

	// void SetPaused(bool p);
	// void TogglePaused();
	// bool IsPaused() const { return m_paused; }
	GLFWwindow *GetWindow() const { return window; }	// modifier for window
	bool IsGameRunning() const { return GameRunning; }	// for controlling main game loop
	bool IsPlaying() const { return m_isPlaying; }		// for controlling scripts
	void SetPaused(bool p);
	void SetPaused(bool p, PauseSource source);
	void TogglePaused();
	bool IsPaused() const { return m_paused; }

	void PlayGame();	// play button functionality
	void PauseGame();	// pause btn functionality
	void ResumeGame();	// resume btn functionality
	void StopGame();	// stop button functionality
	void RestartRuntimeForPlay();
	void CompileScriptAssembly(); // for hot reload 

	void SetEditorFrameCallback(const std::function<void()>& cb);

	// AssetsManager& getAssetsManager() { return mAssets; }
	std::function<void()> m_editorFrameCallback;

	void ToggleFullscreen();
	void SetStartupFullscreen(bool fullscreen) { m_startupFullscreen = fullscreen; }
	bool IsFullscreen() const { return m_isFullscreen; }

	// --- For Delta Time API :D  ---
public:
	// Returns DeltaTime, but returns 0 if game is paused
	// This ensures all movement/animations stop when m_paused is true
	// because scripts use DeltaTime for frame-independent updates (pos += speed * DeltaTime)
	// When DeltaTime = 0, no movement occurs
	float GetDeltaTime() const { return m_paused ? 0.0f : DeltaTime; }
	int GetFrameCount() const { return FrameCount; }
	float GetTimePassed() const { return TimePassed; }
	void SetTime(float time) { TimePassed = time; }

private:
	float DeltaTime = 0.0f;		//float cos it was already float and i lazy change 
	unsigned CurrentFrame = 0.0;
	unsigned LastFrame = 0.0;
	int FrameCount = 0;
	float TimePassed = 0.0f;

private:
	bool GameRunning;
	bool DebugMode = false;
	unsigned LastTime; // The last time the game was updated
	bool m_paused = true;
	bool m_manualPause = false; // set when the player explicitly pauses via ESC
	bool m_isPlaying = false;
	std::string m_originalScenePath = "";
	void RenderDebugDraw();
	GLFWwindow *window;

	// --- Window mode state  ---
	bool m_startupFullscreen = true;
	bool m_isFullscreen = false;
	int  m_windowedX = 100, m_windowedY = 100;
	int  m_windowedW = 0, m_windowedH = 0;
	void SetFullscreen(bool fullscreen);


	// ----------------- .net from here onwards  --------------------------//
	
	void startScriptEngine(); // where we start the bridge linking (Load the .NET runtime) 
	void stopScriptEngine(); // bridge destroyed (Unloading the .NET runtime)

	// References to CoreCLR key components
	HMODULE coreClr = nullptr;
	void* hostHandle = nullptr;
	unsigned int domainId = 0;

	// Function Pointers to CoreCLR functions(DLL functions for managing .Net RunTime lifecycle)
	coreclr_initialize_ptr      initializeCoreClr = nullptr;
	coreclr_create_delegate_ptr createManagedDelegate = nullptr;
	coreclr_shutdown_ptr        shutdownCoreClr = nullptr;

	// Helper Functions (same as/for the one below)
	std::string buildTpaList(const std::string& directory);

	// Helper Functions for grabbing functions from the DLL for managing the .NET Runtime�s lifecycle
	template<typename FunctType>
	FunctType getCoreClrFuncPtr(const std::string& functionName)
	{
		auto fPtr = reinterpret_cast<FunctType>(GetProcAddress(coreClr, functionName.c_str()));
		if (!fPtr)
			throw std::runtime_error("Unable to get pointer to function.");
		return fPtr;
	}

public:
	// Helper Functions (Since boilerplate is now up, grab pointers to manage function)
	template<typename FunctionType>
	FunctionType GetFunctionPtr(const std::string_view& assemblyName, const std::string_view& typeName, const std::string_view& functionName)
	{
		const std::string a{ assemblyName };
		const std::string t{ typeName };
		const std::string f{ functionName };

		FunctionType managedDelegate = nullptr;
		int result = createManagedDelegate
		(
			hostHandle,
			domainId,
			a.c_str(),
			t.c_str(),
			f.c_str(),
			reinterpret_cast<void**>(&managedDelegate)
		);
		// Check if it failed
		if (result < 0)
		{
			std::ostringstream oss;
			oss << std::hex << std::setfill('0') << std::setw(8)
				<< "[DotNetRuntime] Failed to get pointer to function \""
				<< typeName << "." << functionName << "\" in assembly (" << assemblyName << "). "
				<< "Error 0x" << result << "\n";
			throw std::runtime_error(oss.str());
		}
		return managedDelegate;
	}
	
private:
	//Function pointer for our script updates
	void(*executeUpdateFunc)() = nullptr;
public:
	void(*clearScriptsFunc)() = nullptr;

};

// A global pointer to the instance of the core
extern DLL_API CoreEngine *Engine;
extern DLL_API Config config; // global scope
extern DLL_API AssetsManager mAssets;
extern DLL_API int WindowWidth;
extern DLL_API int WindowHeight;
extern DLL_API double g_scrollY;
extern DLL_API std::filesystem::path FilePathToGame; // File path to game folder.