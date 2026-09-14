/******************************************************************************/
/**
 * @file        ProfilingAPI.h
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En
 * @brief       Static utility class providing C# scripts with access to profiling
 *              data and FPS information for debug display purposes.
 * 
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without
 *              the prior written consent of DigiPen Institute of Technology
 *              is prohibited.
 */
 /******************************************************************************/
#pragma once

namespace ScriptAPI
{
	// SystemProfile data structure for C# interop
	// NOTE: This is a VALUE type (not ref struct) so it can be used in generics
	public value struct SystemProfile
	{
	public:
		double totalTime;
		double percentage;
		int callCount;
	};

	// ProfilingManager wrapper - provides C# with profiling data
	public ref class ProfilingAPI abstract sealed
	{
	public:
		// Get the profiling data for all systems
		// Returns Dictionary<string, SystemProfile> with system names and their percentages
		static System::Collections::Generic::Dictionary<System::String^, SystemProfile>^ GetDisplayProfiles();

		// Get the cached frame time (in seconds)
		// Use: fps = 1.0 / frameTime
		static double GetCachedFrameTime();

		// Get current FPS from GLHelper
		// Returns the real-time FPS value
		static float GetFPS();
	};

	// GLHelper wrapper - provides C# with graphics helper utilities
	public ref class GLHelper abstract sealed
	{
	public:
		// Get current frames per second
		static property float fps { float get(); }
	};
}