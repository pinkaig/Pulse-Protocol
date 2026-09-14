/******************************************************************************/
/**
 * @file        pch_temp.h
 * @project     Pulse Protocol
 * @author      Chia Wei Xuan Rachael
 * @brief       This is a precompiled header file that centralizes all and only standard library,
 *				and external library, for optimized
 *				compilation. We do this to make code clean and organization. :]
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/
#pragma once

// Suppress third-party library warnings
#pragma warning(disable : 4146)  // unary minus on unsigned
#pragma warning(disable : 26495) // uninitialized member
#pragma warning(disable : 26451) // arithmetic overflow
#pragma warning(disable : 26498) // constexpr functions
#pragma warning(disable : 4244)  // conversion warnings
#pragma warning(disable : 26812) // enum type preference
#pragma warning(disable : 6285)
#pragma warning(disable : 28800)
#pragma warning(disable : 26819) // fallthrough annotation
#pragma warning(disable : 6262)  // stack usage

// Standard Library Headers
#include <fstream>
#include <iostream>
#include <functional>
#include <string>
#include <cstring>
#include <memory> // for std::shared_ptr
#include <any>
#include <set>
#include <cassert>       // for debug throwassertion
#include <cstdint>       // for uint32
#include <bitset>        // for System ID/signature
#include <array>         // for std::array
#include <vector>        // for std::vectors
#include <deque>         // for std::deque
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
#include <optional>
#include <stdexcept>      

#include <ranges>
#include <cctype>    

// Graphics Library Headers
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#pragma warning(push, 0)
#include <glm/glm.hpp>
#pragma warning(pop)

// Rapid JSON Library Headers
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>