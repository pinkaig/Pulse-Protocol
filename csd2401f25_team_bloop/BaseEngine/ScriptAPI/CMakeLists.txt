cmake_minimum_required(VERSION 3.20)

# ScriptAPI - C++/CLI bridge between C++ and C#
project(ScriptAPI LANGUAGES CXX)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Auto-collect sources (keeps Solution Explorer in sync)
file(GLOB_RECURSE SCRIPTAPI_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
)

# Create library with placeholder
add_library(ScriptAPI SHARED ${SCRIPTAPI_SRC})

# IMPORTANT: Add dependency to ensure PulseEngine is built first
add_dependencies(ScriptAPI PulseEngine)

target_link_libraries(ScriptAPI PRIVATE PulseEngine)

# Explicitly inherit PulseEngine's include directories
target_include_directories(ScriptAPI PRIVATE
    ${CMAKE_SOURCE_DIR}/PulseEngine
    ${CMAKE_SOURCE_DIR}/PulseEngine/include
)

# ---- GLM (header-only) ----
# Don't gate this on EXISTS(). On fresh configs, FetchContent may populate later,
# and missing include dirs causes glm/glm.hpp to fail in the Engine PCH includes.
target_include_directories(ScriptAPI PRIVATE
    "${CMAKE_BINARY_DIR}/_deps/glm-src"
    "${CMAKE_BINARY_DIR}/libraries/glm-src"
)
message(STATUS "ScriptAPI: GLM include candidates: "
    "${CMAKE_BINARY_DIR}/_deps/glm-src;${CMAKE_BINARY_DIR}/libraries/glm-src"
)

# Add RapidJSON include directory
if(DEFINED json_SOURCE_DIR)
    target_include_directories(ScriptAPI PRIVATE "${json_SOURCE_DIR}/include")
    message(STATUS "ScriptAPI: Found RapidJSON at ${json_SOURCE_DIR}/include")
else()
    # Fallback: try common FetchContent locations
    set(_json_paths
        "${CMAKE_BINARY_DIR}/_deps/json-src/include"
        "${CMAKE_BINARY_DIR}/libraries/json-src/include"
    )
    foreach(_path IN LISTS _json_paths)
        if(EXISTS "${_path}/rapidjson/document.h")
            target_include_directories(ScriptAPI PRIVATE "${_path}")
            message(STATUS "ScriptAPI: Found RapidJSON at ${_path}")
            break()
        endif()
    endforeach()
endif()

# Enable C++/CLI (Common Language Runtime)
set_target_properties(ScriptAPI PROPERTIES
    # COMMON_LANGUAGE_RUNTIME ""
    # VS_GLOBAL_CLRSupport "true"
    VS_GLOBAL_CLRSupport "NetCore"
    VS_GLOBAL_TargetFramework "net8.0"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    #VS_DOTNET_REFERENCES "System;System.Core"
)
if(MSVC)
    # Add .NET reference assemblies path
    set(DOTNET_REF_PATH
        "C:/Program Files/dotnet/packs/Microsoft.NETCore.App.Ref/8.0.0/ref/net8.0"
    )

    if(EXISTS "${DOTNET_REF_PATH}")
        target_compile_options(ScriptAPI PRIVATE
            /AI"${DOTNET_REF_PATH}"
        )
        message(STATUS "ScriptAPI: Using .NET ref assemblies at ${DOTNET_REF_PATH}")
    else()
        message(WARNING "ScriptAPI: .NET reference assemblies not found at ${DOTNET_REF_PATH}")
    endif()
endif()

# C++/CLI requires /EHa instead of /EHs
if(MSVC)
    # Remove any inherited /EHs and use /EHa for C++/CLI
    target_compile_options(ScriptAPI PRIVATE 
        /clr:netcore
        /EHa  # Async exception handling for C++/CLI
    )
    # Remove /EHs if it was inherited
    string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "/EHs" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

# Output to .bin directory
set_target_properties(ScriptAPI PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/.bin/$<CONFIG>-x64"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/.bin/$<CONFIG>-x64"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/.bin/$<CONFIG>-x64"
)
    
# Set folder organization for Visual Studio
set_property(TARGET ScriptAPI PROPERTY FOLDER "")

# Enables multi-processor parallel compilation in Visual Studio
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()