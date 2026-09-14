/******************************************************************************/
/**
 * @file        ProfilingAPI.cpp
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En
 * @brief       Implementation fof ScriptAPI wrappers for profiling data and FPS
 * 
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without
 *              the prior written consent of DigiPen Institute of Technology
 *              is prohibited.
 */
 /******************************************************************************/
#pragma once
#define generic generic_workaround
#include <msclr/marshal_cppstd.h>
#include "ProfilingAPI.h"  // ScriptAPI header
#include "Profiling/Profiling.h"  // C++ ProfilingManager (native)
#include "Graphics/glhelper.h"     // C++ GLHelper

#undef generic

namespace ScriptAPI
{
	// =========================================================================
	// ProfilingAPI Implementation
	// =========================================================================

	System::Collections::Generic::Dictionary<System::String^, SystemProfile>^ ProfilingAPI::GetDisplayProfiles()
	{
		// Create the dictionary without calling constructor directly
		System::Collections::Generic::Dictionary<System::String^, SystemProfile>^ result =
			gcnew System::Collections::Generic::Dictionary<System::String^, SystemProfile>(10);

		try
		{
			// Get C++ profiler instance
			auto profiler = ProfilingManager::GetInstance();
			if (!profiler)
				return result;

			// Get the display profiles from C++
			const auto& profiles = profiler->GetDisplayProfiles();

			// Convert each profile to managed C#
			for (auto it = profiles.begin(); it != profiles.end(); ++it)
			{
				const auto& systemName = it->first;
				const auto& profile = it->second;

				// Convert C++ string to C# string
				System::String^ managedName = msclr::interop::marshal_as<System::String^>(systemName);

				// Create managed SystemProfile struct
				SystemProfile managedProfile;
				managedProfile.totalTime = profile.totalTime;
				managedProfile.percentage = profile.percentage;
				managedProfile.callCount = profile.callCount;

				// Add to dictionary using Add method
				result->Add(managedName, managedProfile);
			}
		}
		catch (System::Exception^ ex)
		{
			System::Console::WriteLine("[ProfilingAPI] Error getting display profiles: " + ex->Message);
		}

		return result;
	}

	double ProfilingAPI::GetCachedFrameTime()
	{
		try
		{
			auto* profiler = ProfilingManager::GetInstance();
			if (!profiler)
				return 0.0;

			return profiler->GetCachedFrameTime();
		}
		catch (System::Exception^ ex)
		{
			System::Console::WriteLine("[ProfilingAPI] Error getting frame time: " + ex->Message);
			return 0.0;
		}
	}

	float ProfilingAPI::GetFPS()
	{
		try
		{
			// Calculate FPS
			return ::GLHelper::fps;
		}
		catch (System::Exception^ ex)
		{
			System::Console::WriteLine("[ProfilingAPI] Error getting FPS: " + ex->Message);
			return 0.0f;
		}
	}

	// =========================================================================
	// GLHelper Implementation
	// =========================================================================

	float GLHelper::fps::get()
	{
		try
		{
			// Access the C++ GLHelper::fps static variable
			return ::GLHelper::fps;
		}
		catch (System::Exception^ ex)
		{
			System::Console::WriteLine("[GLHelper] Error getting FPS: " + ex->Message);
			return 0.0f;
		}
	}
}