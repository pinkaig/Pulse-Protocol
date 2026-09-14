# BaseEngine/CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(PulseEngine VERSION 0.1 LANGUAGES C CXX)

# -------------------------------
# CMake / VS defaults
# -------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakePredefinedTargets")

# Keep runtime consistent (C++/CLI friendly)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Put FetchContent downloads in build/libraries/
set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/libraries")
include(FetchContent)

# Let CMake find your cmake/*.cmake helpers
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Disable noisy 3rd-party extras BEFORE fetching deps
set(JSON_BuildTests          OFF CACHE BOOL "Disable nlohmann_json tests" FORCE)
set(RAPIDJSON_BUILD_TESTS    OFF CACHE BOOL "Disable RapidJSON tests"      FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "Disable RapidJSON examples"   FORCE)
set(RAPIDJSON_BUILD_DOC      OFF CACHE BOOL "Disable RapidJSON docs"       FORCE)

# Suppress warnings from external libraries (MSVC only)
if(MSVC)
  add_compile_options(
    /experimental:external
    /external:W0
    /external:anglebrackets
  )
endif()

# -------------------------------
# 3rd-party deps (ONCE)
# -------------------------------
include("${CMAKE_CURRENT_LIST_DIR}/cmake/ImportDependencies.cmake")
cmake_policy(PUSH)
set(CMAKE_FOLDER "Libraries")
importDependencies()
cmake_policy(POP)

# Keep your real projects OUTSIDE "Libraries"
set(CMAKE_FOLDER "")

# -------------------------------
# ImGui
# -------------------------------
add_library(imgui
  ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui.cpp
  ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_draw.cpp
  ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_widgets.cpp
  ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_tables.cpp
)
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_LIST_DIR}/imgui)
set_property(TARGET imgui PROPERTY FOLDER "3rdparty/ImGui")

add_library(imgui_backend
  ${CMAKE_CURRENT_LIST_DIR}/imgui/backends/imgui_impl_glfw.cpp
  ${CMAKE_CURRENT_LIST_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_backend PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/imgui
  ${CMAKE_CURRENT_LIST_DIR}/imgui/backends
)
target_compile_definitions(imgui_backend PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLEW)
target_link_libraries(imgui_backend PRIVATE imgui glfw)
set_property(TARGET imgui_backend PROPERTY FOLDER "3rdparty/ImGui")

# Auto download de .NET DLLs into the folder extern/dotnet/bin/ if its missing
include(DotnetRuntime)

# -------------------------------
# Main projects (ONCE each)
# -------------------------------
add_subdirectory(PulseEngine)     # defines PulseEngine_Editor / PulseEngine_Game
add_subdirectory(ScriptAPI)

# Dynamically build the active game ManagedScripts by reading ActiveGame.json at config time. But must rerun cmake 2 load in.
set(ACTIVE_GAME_JSON_PATH "${CMAKE_CURRENT_LIST_DIR}/ActiveGame.json")  

# Check if ActiveGame.json exists
if(EXISTS "${ACTIVE_GAME_JSON_PATH}")
    file(READ "${ACTIVE_GAME_JSON_PATH}" ACTIVE_GAME_JSON_CONTENT)               # reads whats inside
    string(JSON ACTIVE_GAME_NAME GET "${ACTIVE_GAME_JSON_CONTENT}" "ActiveGame") # grab ActiveGame value
    message(STATUS "[PulseEngine] Active game: ${ACTIVE_GAME_NAME}")
else()
    # if cannot find just hardcode it.
    set(ACTIVE_GAME_NAME "PulseProtocol")
    message(WARNING "[PulseEngine] ActiveGame.json not found \u2014 defaulting to PulseProtocol")
endif()

# Check: Only add ManagedScripts if the folder actually exists, incase game name is wrong or folder wasn't created yet
if(IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/${ACTIVE_GAME_NAME}/ManagedScripts")
    add_subdirectory(${ACTIVE_GAME_NAME}/ManagedScripts)
else()
    message(WARNING "[PulseEngine] '${ACTIVE_GAME_NAME}/ManagedScripts' not found! Did you create the game via New Game button in editor?")
endif()
add_subdirectory(PulseEditor)     # depends on ManagedScripts

# ONLY build the active games exe by reading from ActiveGame.json(Need rerun run.bat tho)
if(IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/${ACTIVE_GAME_NAME}")
    add_subdirectory(${ACTIVE_GAME_NAME})
else()
    message(WARNING "[PulseEngine] Game folder '${ACTIVE_GAME_NAME}' not found! Check ActiveGame.json.")
endif()

# -------------------------------
# Nice Solution Explorer layout
# -------------------------------
function(pulse_rootify tgt)
  if(TARGET ${tgt})
    set_target_properties(${tgt} PROPERTIES FOLDER "")
  endif()
endfunction()

pulse_rootify(PulseEditor)
pulse_rootify(${ACTIVE_GAME_NAME})
pulse_rootify(ScriptAPI)

# Visible "PulseEngine" node in the solution (meta target)
if(NOT TARGET PulseEngine)
  add_custom_target(PulseEngine)
  if(TARGET PulseEngine_Editor)
    add_dependencies(PulseEngine PulseEngine_Editor)
  endif()
  if(TARGET PulseEngine_Game)
    add_dependencies(PulseEngine PulseEngine_Game)
  endif()
  set_target_properties(PulseEngine PROPERTIES FOLDER "Engine")
endif()

# --- Hide engine variants by grouping into a folder ---
if(TARGET PulseEngine_Editor)
  set_target_properties(PulseEngine_Editor PROPERTIES FOLDER "Engine")
endif()

if(TARGET PulseEngine_Game)
  set_target_properties(PulseEngine_Game PROPERTIES FOLDER "Engine")
endif()

if(TARGET PulseEngine)
  set_target_properties(PulseEngine PROPERTIES FOLDER "Engine")
endif()

# Default startup project in VS
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT  PulseEditor)
