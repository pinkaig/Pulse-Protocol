/******************************************************************************/
/**
 * @file        pch.h
 * @project     Pulse Protocol
 * @author      Chia Wei Xuan Rachael 
 * @brief       This is a precompiled header file that centralizes all standard library,
 *				external library, and internal engine headers for optimized
 *				compilation. We do this to make code clean and organization. :]
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once

// pch.h: This is a precompiled header file.
// I'm doing this as we only need to compile it once and make  the files neater, we will put all our lib that we are using here. 
// Just put #include "pch/pch.h" and you will have access to it

// Suppress third-party library warnings
#pragma warning(disable: 4146)  // unary minus on unsigned
#pragma warning(disable: 26495) // uninitialized member
#pragma warning(disable: 26451) // arithmetic overflow  
#pragma warning(disable: 26498) // constexpr functions
#pragma warning(disable: 4244)  // conversion warnings
#pragma warning(disable: 26812) // enum type preference
#pragma warning(disable: 6285)
#pragma warning(disable: 28800)
#pragma warning(disable: 26819)  // fallthrough annotation
#pragma warning(disable: 6262)   // stack usage


// Standard Library Headers
#include <fstream>
#include <iostream>
#include <functional>
#include <string>
#include <cstring>
#include <memory>		 // for std::shared_ptr
#include <any>
#include <set>			
#include <cassert>		 // for debug throwassertion
#include <cstdint>		 // for uint32
#include <bitset>		 // for System ID/signature
#include <array>		 // for std::array
#include <vector>		 // for std::vectors
#include <deque>		 // for std::deque
#include <unordered_map> // for std::unordered_map
#include <map>
#include <cmath>
#include <iomanip> //debug
#include <algorithm>
#include <random>
#include <sstream>
#include <chrono> // for profiling
#include <list>
#include <filesystem>
#include <cstdlib> 
#include <stdexcept>      

// Graphics Library Headers
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#pragma warning(push, 0)
#include <glm/glm.hpp>
#pragma warning(pop)

// FMOD Headers
#pragma warning(push, 0)
#include "FMOD_API/api/core/inc/fmod.hpp"
#include "FMOD_API/api/core/inc/fmod_errors.h"
#pragma warning(pop)


//Serialize
#pragma warning(push, 0)
#include <rapidjson/document.h>     // for Document, Value, AllocatorType
#include <rapidjson/prettywriter.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h> // for StringBuffer
#pragma warning(pop)

// System Library Headers
#include "Graphics/glhelper.h"
#include "Graphics/glslshader.h"

#include "Profiling/Profiling.h"
#include "Math/math.h"
#include "Input/InputManager.h"
#include "Input/RhythmGameplayInput.h"
#include "Audio/AudioManager.h"
#include "Graphics/GraphicsManager.h"
#include "Graphics/DebugDraw.h"
#include "Graphics/Transform.h"
#include "Graphics/Animation.h"
#include "Graphics/Renderable.h"
#include "Physics/collision.h"
#include "Resources/AssetRegistry.h"
//#include "../PulseEngine/Serialization/Serialization.h"

// Core Engine
#include "CoreEngine/Core/CoreEngine.h"

// ECS
#include "CoreEngine/ECS/Types.h"
#include "CoreEngine/ECS/System.h"
#include "CoreEngine/ECS/Singleton.h"
#include "CoreEngine/ECS/Coordinator.h"
#include "CoreEngine/ECS/SystemManager.h"
#include "CoreEngine/ECS/EntityManager.h"
#include "CoreEngine/ECS/ComponentArray.h"
#include "CoreEngine/ECS/ComponentManager.h"

// Systems
#include "GameLogic/GameLogic.h"
#include "CoreEngine/Window.h"
#include "CoreEngine/Assets.h"
#include "CoreEngine/Scene/SceneManager.h"
#include "CoreEngine/Asset/AssetsManager.h"
#include "CoreEngine/Message/MessageManager.h"
