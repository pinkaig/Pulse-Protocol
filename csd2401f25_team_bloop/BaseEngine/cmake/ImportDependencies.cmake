set(CMAKE_FOLDER "Libraries")

include(FetchContent)

# Macro to import GLFW
macro(import_glfw)
 if(NOT TARGET glfw)
        set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
        FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.3.8
        )
        # Trim extras
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_DLL ON CACHE BOOL "" FORCE)

        # Works on CMake >=3.14, including 3.29.2 and all 4.x
        FetchContent_MakeAvailable(glfw)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        # no add_subdirectory(), no include_directories()
    endif()
endmacro()

# --- GLEW ---
macro(import_glew)
  if(NOT TARGET GLEW::glew AND NOT TARGET libglew_static AND NOT TARGET libglew_shared)
    FetchContent_Declare(
      glew
      GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
      GIT_TAG d06782b910213d675925e6e51a69ad0fd1fe1f23
    )
    FetchContent_MakeAvailable(glew)

    # Fix CMake "prefixed in the build directory" by using BUILD/INSTALL interface include dirs
    if(TARGET libglew_static)
      set_property(TARGET libglew_static PROPERTY
        INTERFACE_INCLUDE_DIRECTORIES
          "$<BUILD_INTERFACE:${glew_SOURCE_DIR}/include>;$<INSTALL_INTERFACE:include>"
      )
      target_compile_definitions(libglew_static INTERFACE GLEW_STATIC)
      add_library(GLEW::glew ALIAS libglew_static)

    elseif(TARGET libglew_shared)
      set_property(TARGET libglew_shared PROPERTY
        INTERFACE_INCLUDE_DIRECTORIES
          "$<BUILD_INTERFACE:${glew_SOURCE_DIR}/include>;$<INSTALL_INTERFACE:include>"
      )
      add_library(GLEW::glew ALIAS libglew_shared)
    endif()
  endif()
endmacro()

# Macro to import glm
macro(import_glm)
    if(NOT TARGET glm)  # Guard to prevent multiple inclusion
        FetchContent_Declare(
            glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG 1.0.1
        )
        FetchContent_MakeAvailable(glm)
 
        include_directories(${glm_SOURCE_DIR})
    endif()
endmacro()

# Macro to import RapidJSON (header-only, no build needed)
macro(import_json)
    if(NOT TARGET json)  # Guard to prevent multiple inclusion
        # RapidJSON is header-only, so we fetch without building its CMakeLists.txt
        FetchContent_Declare(
            json
            GIT_REPOSITORY "https://github.com/Tencent/rapidjson.git"
            GIT_TAG master
        )
        FetchContent_GetProperties(json)
        if(NOT json_POPULATED)
            FetchContent_Populate(json)
            include_directories(${json_SOURCE_DIR}/include)
        endif()
    endif()
endmacro()

macro(import_freetype)
  if(NOT TARGET Freetype::Freetype)
    message(STATUS "FreeType not found, fetching from source...")
    
    include(FetchContent)
    
    FetchContent_Declare(
      freetype
      GIT_REPOSITORY https://github.com/freetype/freetype.git
      GIT_TAG        VER-2-13-3
      GIT_SHALLOW    TRUE
    )
    
    # Disable extra dependencies
    set(FT_DISABLE_BZIP2     ON  CACHE BOOL "" FORCE)
    set(FT_DISABLE_HARFBUZZ  ON  CACHE BOOL "" FORCE)
    set(FT_DISABLE_ZLIB      ON  CACHE BOOL "" FORCE)
    set(FT_DISABLE_PNG       ON  CACHE BOOL "" FORCE)
    set(FT_DISABLE_BROTLI    ON  CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(freetype)
    
    # Debug output
    message(STATUS "FreeType fetched to: ${freetype_SOURCE_DIR}")
    
    # Fix the include directories using generator expressions
    if(TARGET freetype)
      set(_ft_inc "${freetype_SOURCE_DIR}/include")
      
      # Clear any existing interface include directories that might cause issues
      set_property(TARGET freetype PROPERTY INTERFACE_INCLUDE_DIRECTORIES "")
      
      # Set using BUILD_INTERFACE generator expression
      target_include_directories(freetype PUBLIC
        "$<BUILD_INTERFACE:${_ft_inc}>"
        "$<INSTALL_INTERFACE:include>"
      )
      
      # Create the alias
      if(NOT TARGET Freetype::Freetype)
        add_library(Freetype::Freetype ALIAS freetype)
      endif()
      
      message(STATUS "FreeType include directory: ${_ft_inc}")
      
      # Verify ft2build.h exists
      if(EXISTS "${_ft_inc}/ft2build.h")
        message(STATUS "✓ Found ft2build.h at ${_ft_inc}/ft2build.h")
      else()
        message(WARNING "✗ ft2build.h NOT found at ${_ft_inc}/ft2build.h")
      endif()
    endif()
  else()
    message(STATUS "FreeType target already exists")
  endif()
endmacro()

# macro(import_spdlog)
#     if(NOT TARGET spdlog)  # Guard to prevent multiple inclusion
#         FetchContent_Declare(
#           spdlog
#           GIT_REPOSITORY https://github.com/gabime/spdlog.git
#           GIT_TAG v1.14.1
#         )

#         FetchContent_MakeAvailable(spdlog)

#         include_directories(${spdlog_SOURCE_DIR}/include)
#     endif()
# endmacro()

# Macro to import all dependencies
macro(importDependencies)
    import_glfw()
    import_glew()
    import_glm()
    import_freetype()
    #import_spdlog()
    import_json()
endmacro()