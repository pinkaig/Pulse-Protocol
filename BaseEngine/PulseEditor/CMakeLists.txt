cmake_minimum_required(VERSION 3.20)

set(EDITOR_NAME PulseEditor)
set(EDITOR_ROOT ${CMAKE_CURRENT_LIST_DIR})

message(STATUS "Configuring ${EDITOR_NAME} from: ${EDITOR_ROOT}")

# Pick up editor sources (and headers for VS filters)
file(GLOB_RECURSE EDITOR_SRC CONFIGURE_DEPENDS
    "${EDITOR_ROOT}/*.c"
    "${EDITOR_ROOT}/*.cc"
    "${EDITOR_ROOT}/*.cxx"
    "${EDITOR_ROOT}/*.cpp"
    "${EDITOR_ROOT}/*.mm"
    "${EDITOR_ROOT}/*.m"
    "${EDITOR_ROOT}/*.h"
    "${EDITOR_ROOT}/*.hpp"
)

# Always create the target (even if empty) so it shows in Solution Explorer
if (NOT EDITOR_SRC)
    message(WARNING "[${EDITOR_NAME}] No source files found. Creating __dummy_editor.cpp so the target shows up.")
    file(WRITE "${EDITOR_ROOT}/__dummy_editor.cpp" "// generated dummy file to satisfy CMake\nint __pulseeditor_dummy__ = 0;")
    list(APPEND EDITOR_SRC "${EDITOR_ROOT}/__dummy_editor.cpp")
endif()

add_executable(${EDITOR_NAME} ${EDITOR_SRC})

# Hide console in Release (GUI subsystem), keep console in Debug
if (WIN32)
  set_target_properties(${EDITOR_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)

  target_link_options(${EDITOR_NAME} PRIVATE
    $<$<CONFIG:Debug>:/SUBSYSTEM:CONSOLE>
    $<$<NOT:$<CONFIG:Debug>>:/SUBSYSTEM:WINDOWS>
    $<$<NOT:$<CONFIG:Debug>>:/ENTRY:mainCRTStartup>
  )
endif()

# ensures POST_BUILD copy commands run when DLLs change
set_target_properties(${EDITOR_NAME} PROPERTIES 
    VS_GLOBAL_DisableFastUpToDateCheck true
)

# Pass the RootFolderName into editor too.
get_filename_component(RootFolderName "${CMAKE_SOURCE_DIR}" NAME)
target_compile_definitions(${EDITOR_NAME} PRIVATE RootFolderName="${RootFolderName}")

target_link_libraries(${EDITOR_NAME} PRIVATE PulseEngine)

# Nice Solution Explorer layout
source_group(TREE "${EDITOR_ROOT}" FILES ${EDITOR_SRC})

# Editor needs to include engine headers, and often uses GLEW/FMOD types via those headers
# (FMOD path: reuse the same detection if not already set)
if (NOT DEFINED FMOD_API_DIR_FOUND)
  set(_FMOD_CANDIDATES
      "${FMOD_API_DIR}"
      "${CMAKE_SOURCE_DIR}/PulseEngine/../FMOD_API"
      "${CMAKE_SOURCE_DIR}/FMOD_API"
      "${CMAKE_SOURCE_DIR}/ThirdParty/FMOD_API"
  )
  foreach(_cand IN LISTS _FMOD_CANDIDATES)
    if(_cand AND EXISTS "${_cand}/api/core/lib/x64")
      set(FMOD_API_DIR_FOUND "${_cand}" CACHE PATH "FMOD API root" FORCE)
      break()
    endif()
  endforeach()
  if(NOT FMOD_API_DIR_FOUND)
    message(WARNING "[${EDITOR_NAME}] FMOD_API not found here; assuming Engine target carries it. If you see FMOD compile errors, set -DFMOD_API_DIR=...")
  endif()
endif()

target_include_directories(${EDITOR_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/PulseEngine
    ${CMAKE_SOURCE_DIR}/PulseEngine/include
    $<$<BOOL:${FMOD_API_DIR_FOUND}>:${FMOD_API_DIR_FOUND}/api/core/inc>
    $<$<BOOL:${FMOD_API_DIR_FOUND}>:${FMOD_API_DIR_FOUND}/api/studio/inc>
)

target_link_libraries(${EDITOR_NAME} PUBLIC
    imgui
    imgui_backend
    glfw
    GLEW::glew
    Freetype::Freetype
)

# Warnings
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${EDITOR_NAME} PRIVATE -Wall)
elseif (MSVC)
    target_compile_options(${EDITOR_NAME} PRIVATE /W4 /WX- /wd4251)
endif()

message(STATUS "[${EDITOR_NAME}] sources: ${EDITOR_SRC}")


# Copy FMOD runtime
add_custom_command(TARGET PulseEditor POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<$<CONFIG:Debug>:${FMOD_API_DIR_FOUND}/api/core/lib/x64/fmodL.dll>"
    "$<$<NOT:$<CONFIG:Debug>>:${FMOD_API_DIR_FOUND}/api/core/lib/x64/fmod.dll>"
    "$<TARGET_FILE_DIR:PulseEditor>"
  COMMAND_EXPAND_LISTS
)


# Make sure PulseEditor waits for ScriptAPI and ManagedScripts to build
add_dependencies(PulseEditor ScriptAPI ManagedScripts)

# Consolidated POST_BUILD: Copy all DLLs in one command
add_custom_command(TARGET PulseEditor POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:PulseEngine>" "$<TARGET_FILE_DIR:PulseEditor>"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:ScriptAPI>" "$<TARGET_FILE_DIR:PulseEditor>"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/extern/dotnet/bin" "$<TARGET_FILE_DIR:PulseEditor>"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_RUNTIME_DLLS:PulseEditor> "$<TARGET_FILE_DIR:PulseEditor>"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_BINARY_DIR}/.bin/$<CONFIG>-x64/ManagedScripts.dll" "$<TARGET_FILE_DIR:PulseEditor>"
  COMMAND_EXPAND_LISTS
  VERBATIM
  COMMENT "Copying PulseEngine/ScriptAPI/ManagedScripts + .NET runtime to PulseEditor output"
)



# Enables multi-processor parallel compilation in Visual Studio
if(MSVC)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

