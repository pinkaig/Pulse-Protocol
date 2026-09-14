cmake_minimum_required(VERSION 3.20)

message(STATUS "PulseEngine: FREETYPE_INCLUDE_DIRS='${FREETYPE_INCLUDE_DIRS}'")
message(STATUS "PulseEngine: FREETYPE_INCLUDE_DIR='${FREETYPE_INCLUDE_DIR}'")
# -------------------------------
# PulseEngine (build 2 variants)
# -------------------------------

# Auto-collect sources (keeps Solution Explorer in sync)
file(GLOB_RECURSE ENGINE_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/**/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/**/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/**/*.h"
    "${CMAKE_CURRENT_LIST_DIR}/**/*.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
    "${CMAKE_CURRENT_LIST_DIR}/*.hpp"
)

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${ENGINE_SRC})

# ---- Robust FMOD discovery (once) ----
if (NOT DEFINED FMOD_API_DIR_FOUND)
  set(_FMOD_CANDIDATES
      "${FMOD_API_DIR}"                                   # user-provided -DFMOD_API_DIR=...
      "${CMAKE_CURRENT_LIST_DIR}/FMOD_API"                 # PulseEngine/FMOD_API
      "${CMAKE_SOURCE_DIR}/FMOD_API"                       # top-level / FMOD_API
      "${CMAKE_SOURCE_DIR}/ThirdParty/FMOD_API"            # typical thirdparty layout
  )
  foreach(_cand IN LISTS _FMOD_CANDIDATES)
    if(_cand AND EXISTS "${_cand}/api/core/lib/x64")
      set(FMOD_API_DIR_FOUND "${_cand}" CACHE PATH "FMOD API root" FORCE)
      break()
    endif()
  endforeach()
  if(NOT FMOD_API_DIR_FOUND)
    message(FATAL_ERROR "FMOD_API folder not found. Set -DFMOD_API_DIR=PATH (must contain api/core/lib/x64).")
  endif()
endif()
message(STATUS "Using FMOD at: ${FMOD_API_DIR_FOUND}")

# Helper: create one engine variant
function(add_pulse_engine_variant TARGET_NAME IS_EDITOR)
    # Make SHARED to be DLL instead of lib
    add_library(${TARGET_NAME} SHARED ${ENGINE_SRC})
if(TARGET Freetype::Freetype)
  target_link_libraries(${TARGET_NAME} PUBLIC Freetype::Freetype)
endif()
# --- Fix FreeType include path (ft2build.h) ---
# Make sure the engine target can actually see ft2build.h, even if the package
# only exposes include/freetype2 or otherwise has odd layout.
if(TARGET Freetype::Freetype)
    get_target_property(_ft_inc_dirs Freetype::Freetype INTERFACE_INCLUDE_DIRECTORIES)

    if(_ft_inc_dirs)
        foreach(_dir IN LISTS _ft_inc_dirs)
            # Case 1: dir itself contains ft2build.h
            if(EXISTS "${_dir}/ft2build.h")
                target_include_directories(${TARGET_NAME} PUBLIC "${_dir}")
            endif()

            # Case 2: package gives .../include/freetype2 but ft2build.h is in .../include
            if(_dir MATCHES "freetype2$" AND EXISTS "${_dir}/../ft2build.h")
                get_filename_component(_parent "${_dir}" DIRECTORY)
                target_include_directories(${TARGET_NAME} PUBLIC "${_parent}")
            endif()
        endforeach()
    endif()
endif()
    get_target_property(_FT_INC Freetype::Freetype INTERFACE_INCLUDE_DIRECTORIES)
    if(_FT_INC)
        target_include_directories(${TARGET_NAME} PRIVATE ${_FT_INC})
    endif()

    set_target_properties(${TARGET_NAME} PROPERTIES
        VS_GLOBAL_DisableFastUpToDateCheck true
    )

    # Define DLL export macro
    target_compile_definitions(${TARGET_NAME} PRIVATE DLL_API_EXPORT)

    # Look at what the folder name we are in and make that the RootFolderName
    get_filename_component(RootFolderName "${CMAKE_SOURCE_DIR}" NAME)
    target_compile_definitions(${TARGET_NAME} PRIVATE RootFolderName="${RootFolderName}")

    # IMPORTANT: keep your old behavior:
    # - Editor build: ENABLE_EDITOR is defined
    # - Game build:  ENABLE_EDITOR is NOT defined
    if(${IS_EDITOR})
        target_compile_definitions(${TARGET_NAME} PUBLIC ENABLE_EDITOR)
    endif()

    # C++ standard lib and warnings
    set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 20)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${TARGET_NAME} PRIVATE -Wall)
    elseif (MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX- /wd4251)
    endif()

    # Includes owned by this target
    target_include_directories(${TARGET_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}                 # Engine headers
        "${CMAKE_SOURCE_DIR}/extern/dotnet/include" # .NET Core hosting headers
        "${FMOD_API_DIR_FOUND}/api/core/inc"
    )
    # ---- FreeType headers (ft2build.h) ----
    # Some FreeType packages don't populate FREETYPE_INCLUDE_DIR(S) variables (CONFIG mode) but *do* provide a target.
    # Use the target's include dirs when available, and fall back to common FetchContent layouts.
    if (TARGET Freetype::Freetype)
        target_include_directories(${TARGET_NAME} PUBLIC
            "$<TARGET_PROPERTY:Freetype::Freetype,INTERFACE_INCLUDE_DIRECTORIES>"
        )
    endif()

    set(_FT_FALLBACK_INC "")
    foreach(_p
        "${CMAKE_BINARY_DIR}/_deps/freetype_src-src/include"
        "${CMAKE_BINARY_DIR}/_deps/freetype-src/include"
        "${CMAKE_BINARY_DIR}/_deps/freetype-build/include"
    )
        if (EXISTS "${_p}/ft2build.h")
            list(APPEND _FT_FALLBACK_INC "${_p}")
        endif()
    endforeach()

    if (_FT_FALLBACK_INC)
        target_include_directories(${TARGET_NAME} PUBLIC ${_FT_FALLBACK_INC})
    elseif (FREETYPE_INCLUDE_DIRS)
        target_include_directories(${TARGET_NAME} PUBLIC ${FREETYPE_INCLUDE_DIRS})
    elseif (FREETYPE_INCLUDE_DIR)
        target_include_directories(${TARGET_NAME} PUBLIC ${FREETYPE_INCLUDE_DIR})
    endif()


    # Core deps (PUBLIC so apps inherit them via engine link)
    target_link_libraries(${TARGET_NAME} PUBLIC
        glfw
        libglew_static
        imgui
        imgui_backend
        Freetype::Freetype
    )
    # FMOD libraries (multi-config safe for MSVC)
    target_link_libraries(${TARGET_NAME} PUBLIC
        debug     "${FMOD_API_DIR_FOUND}/api/core/lib/x64/fmodL_vc.lib"
        optimized "${FMOD_API_DIR_FOUND}/api/core/lib/x64/fmod_vc.lib"
        debug     "${FMOD_API_DIR_FOUND}/api/studio/lib/x64/fmodstudioL_vc.lib"
        optimized "${FMOD_API_DIR_FOUND}/api/studio/lib/x64/fmodstudio_vc.lib"
    )

    # Copy the correct FMOD DLLs next to the ENGINE DLL
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FMOD_API_DIR_FOUND}/api/core/lib/x64/$<IF:$<CONFIG:Debug>,fmodL.dll,fmod.dll>"
        $<TARGET_FILE_DIR:${TARGET_NAME}>
    )
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FMOD_API_DIR_FOUND}/api/studio/lib/x64/$<IF:$<CONFIG:Debug>,fmodstudioL.dll,fmodstudio.dll>"
        $<TARGET_FILE_DIR:${TARGET_NAME}>
    )

    # Note: game JSON and Assets now live in PulseProtocol/ and are copied by its CMakeLists.

    # Copy .NET Runtime DLLs into the engine output directory (fixed: no hardcoded PulseEngine target)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/extern/dotnet/bin"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>"
        COMMENT "Copying .NET Runtime DLLs to engine output directory"
    )
endfunction()

# Build single engine (shared by game + editor)
add_pulse_engine_variant(PulseEngine ON)

target_link_libraries(PulseEngine PRIVATE
  GLEW::glew
  Freetype::Freetype
)


# Enables multi-processor parallel compilation in Visual Studio
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()