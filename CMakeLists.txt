cmake_minimum_required(VERSION 3.12)
project(sheepRace VERSION 0.1.0)

# Set a default build type if none is specified (e.g. Release, Debug)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Release, Debug, RelWithDebInfo, MinSizeRel)." FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Note: Target-specific include directories are added after add_executable()

# --- Local Libraries Setup ---
# Ensure raylib, imgui, and rlimgui sources are placed in the 'libs' directory.

# --- Raylib (from local source) ---
# Assumes raylib source code (with its own CMakeLists.txt) is in project_name/libs/raylib
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build raylib as a static library" FORCE)
set(RAYLIB_EXAMPLES OFF CACHE BOOL "Disable raylib examples" FORCE) # For raylib 5.0+
# set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # For older raylib versions if needed
add_subdirectory(libs/raylib) # This will create a 'raylib' target

# --- Dear ImGui & rlImGui (source files to be compiled) ---
# Assumes Dear ImGui source code is in project_name/libs/imgui
# Assumes rlImGui source code is in project_name/libs/rlimgui
list(APPEND LOCAL_LIB_SOURCES
    # Dear ImGui core files
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    # libs/imgui/imgui_demo.cpp # Uncomment to include the ImGui demo

    # rlImGui (provides the Raylib backend for ImGui)
    libs/rlimgui/rlImGui.cpp
)

# --- Main Project Executable ---
file(GLOB MAIN_SOURCES "src/*.cpp") # User's main source files
add_executable(${PROJECT_NAME} ${MAIN_SOURCES} ${LOCAL_LIB_SOURCES})

# --- Target Specific Properties for ${PROJECT_NAME} ---
# These MUST come AFTER add_executable(${PROJECT_NAME} ...)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include  # For project's own headers in project_name/include
    libs/imgui                           # For ImGui headers
    libs/rlimgui                         # For rlImGui headers
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib opengl32 gdi32 winmm)
    # Optional: Add -static for MinGW static linking
    # if(CMAKE_CXX_COMPILER_ID MATCHES "GNU") # Check for MinGW
    #     target_link_options(${PROJECT_NAME} PRIVATE "-static")
    # endif()
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib) # Linux/other platforms
endif()

# --- Copy Resources to Build Directory ---
file(COPY resources DESTINATION ${CMAKE_BINARY_DIR})
