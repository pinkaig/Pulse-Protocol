cmake_minimum_required(VERSION 3.20)

# Grab the game name from the current folder name(it should be same name as when u add a new game in editor)
get_filename_component(GAME_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(GAME_ROOT ${CMAKE_CURRENT_LIST_DIR})

# Grab all cpp/h in this folder
file(GLOB_RECURSE GAME_SRC CONFIGURE_DEPENDS
    "${GAME_ROOT}/*.cpp"
    "${GAME_ROOT}/*.h"
) 

add_executable(${GAME_NAME} ${GAME_SRC})

# Hide console in Release (GUI subsystem), keep console in Debug
if (WIN32)
  set_target_properties(${GAME_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)

  target_link_options(${GAME_NAME} PRIVATE
    $<$<CONFIG:Debug>:/SUBSYSTEM:CONSOLE>
    $<$<NOT:$<CONFIG:Debug>>:/SUBSYSTEM:WINDOWS>
    $<$<NOT:$<CONFIG:Debug>>:/ENTRY:mainCRTStartup>
  )
endif()

# Makes the game fking dynamic instead of hardcoded.
add_dependencies(${GAME_NAME} PulseEngine)

# Ensures POST_BUILD copy commands run
set_target_properties(${GAME_NAME} PROPERTIES 
    VS_GLOBAL_DisableFastUpToDateCheck true
)

# Use the engine library
target_link_libraries(${GAME_NAME} PRIVATE PulseEngine)

# So we can do #include "pch/pch.h" and other engine headers
target_include_directories(${GAME_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/PulseEngine
)

add_dependencies(${GAME_NAME} ScriptAPI)

# Copy PulseEngine.dll
add_custom_command(TARGET ${GAME_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:PulseEngine>" "$<TARGET_FILE_DIR:${GAME_NAME}>"
  COMMENT "Copying PulseEngine.dll to PulseProtocol"
  VERBATIM
)

# Copy all runtime DLLs (glfw3.dll, etc.)
add_custom_command(TARGET ${GAME_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:${GAME_NAME}> "$<TARGET_FILE_DIR:${GAME_NAME}>"
    COMMAND_EXPAND_LISTS
    COMMENT "Copying DLL Dependencies like glfw3.dll to PulseProtocol executable directory."
    VERBATIM
)

# Copy ScriptAPI.dll to PulseProtocol output
add_custom_command(TARGET ${GAME_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:ScriptAPI>"
        "$<TARGET_FILE_DIR:${GAME_NAME}>"
    COMMENT "Copying ScriptAPI.dll to PulseProtocol"
)

# Copy .NET Runtime DLLs for C# scripting
add_custom_command(TARGET ${GAME_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/extern/dotnet/bin"
        "$<TARGET_FILE_DIR:${GAME_NAME}>"
    COMMENT "Copying .NET Runtime DLLs to PulseProtocol output directory"
)

# Copy JSON (from game project folder, not engine)
add_custom_command(TARGET ${GAME_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_LIST_DIR}/JSON
        $<TARGET_FILE_DIR:${GAME_NAME}>/JSON
    COMMENT "Copying JSON files to PulseProtocol"
)

# Copy Assets (from game project folder, not engine)
add_custom_command(TARGET ${GAME_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_LIST_DIR}/Assets
        $<TARGET_FILE_DIR:${GAME_NAME}>/Assets
    COMMENT "Copying Assets to PulseProtocol"
)

# Copy JSON to shared build/PulseEngine folder
#add_custom_command(TARGET ${GAME_NAME} POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_directory
#        ${CMAKE_SOURCE_DIR}/PulseEngine/JSON
#        $<TARGET_FILE_DIR:${GAME_NAME}>/../../PulseEngine/JSON
#    COMMENT "Copying JSON files to shared PulseEngine folder"
#)

# Copy Assets to shared build/PulseEngine folder
#add_custom_command(TARGET ${GAME_NAME} POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_directory
#        ${CMAKE_SOURCE_DIR}/PulseEngine/Assets
#        $<TARGET_FILE_DIR:${GAME_NAME}>/../../PulseEngine/Assets
#    COMMENT "Copying Assets to shared PulseEngine folder"
#)
# Copy engine shaders to the release layout PulseProtocol expects
add_custom_command(TARGET ${GAME_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/PulseEngine/Graphics/shader"
        "$<TARGET_FILE_DIR:${GAME_NAME}>/PulseEngine/Graphics/shader"
    COMMENT "Copying engine shaders to PulseProtocol output"
)

# Copy ManagedScripts.dll next to the game executable
add_custom_command(TARGET ${GAME_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/.bin/$<CONFIG>-x64/ManagedScripts.dll"
        "$<TARGET_FILE_DIR:${GAME_NAME}>"
    COMMENT "Copying ManagedScripts.dll to ${GAME_NAME}"
)

# Must build after ManagedScripts is compiled
add_dependencies(${GAME_NAME} ManagedScripts)

# Bundle MSVC runtime DLLs (VCRUNTIME140.dll, MSVCP140.dll, etc.) for self-contained distribution
set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
include(InstallRequiredSystemLibraries)
if(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
    add_custom_command(TARGET ${GAME_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
            "$<TARGET_FILE_DIR:${GAME_NAME}>"
        COMMENT "Copying MSVC runtime DLLs to PulseProtocol output"
    )
endif()

# Enables multi-processor parallel compilation in Visual Studio
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    
    # --- FMOD DLL copy for PulseProtocol (Debug/Release aware) ---
    if (FMOD_API_DIR_FOUND)
        set(FMOD_CORE_DEBUG     "${FMOD_API_DIR_FOUND}/api/core/lib/x64/fmodL.dll")
        set(FMOD_CORE_RELEASE   "${FMOD_API_DIR_FOUND}/api/core/lib/x64/fmod.dll")

        set(FMOD_STUDIO_DEBUG   "${FMOD_API_DIR_FOUND}/api/studio/lib/x64/fmodstudioL.dll")
        set(FMOD_STUDIO_RELEASE "${FMOD_API_DIR_FOUND}/api/studio/lib/x64/fmodstudio.dll")

        add_custom_command(TARGET ${GAME_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<IF:$<CONFIG:Debug>,${FMOD_CORE_DEBUG},${FMOD_CORE_RELEASE}>"
                $<TARGET_FILE_DIR:${GAME_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<IF:$<CONFIG:Debug>,${FMOD_STUDIO_DEBUG},${FMOD_STUDIO_RELEASE}>"
                $<TARGET_FILE_DIR:${GAME_NAME}>
        )
    endif()
endif()