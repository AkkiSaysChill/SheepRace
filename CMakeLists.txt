cmake_minimum_required(VERSION 3.12)
project(GameEngine VERSION 0.1.0)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    
    set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
    )
    
    foreach(CompilerFlag ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()
    
    set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "Use static runtime" FORCE)
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Release, Debug, RelWithDebInfo, MinSizeRel)." FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Unit Testing ---
# Download and configure Google Test framework
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler options.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing for the entire project
enable_testing()
# Add the directory containing the project's tests
add_subdirectory(tests)


# --- External Libraries ---
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build libraries as static" FORCE)
set(RAYLIB_EXAMPLES OFF CACHE BOOL "Disable raylib examples" FORCE)
add_subdirectory(libs/raylib)

set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/bullet3)

# --- Engine Library ---
set(ENGINE_SOURCES
    src/engine/asset_loader.cpp
    src/engine/CollisionManager.cpp
    src/engine/DrawableComponent.cpp
    src/engine/Engine.cpp
    src/engine/GameObjectManager.cpp
    src/engine/PhysicsComponent.cpp
    src/engine/PhysicsSystem.cpp
    src/engine/PostProcessing.cpp
    src/engine/Renderer.cpp
    src/engine/SceneManager.cpp
    src/engine/skybox.cpp
    src/engine/editorUi.cpp
    src/engine/particleSystem.cpp
    src/engine/PlayerProgress.cpp
    src/engine/AnimationComponent.cpp
    src/engine/Scene3D.cpp
    src/engine/Scene2D.cpp
    src/engine/levelEditor.cpp
    src/engine/TextManager.cpp
    src/engine/VisualLogicEditor.cpp
)

set(IMGUI_SOURCES
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    libs/rlimgui/rlImGui.cpp
    libs/imguizmo/ImGuizmo.cpp
    libs/imguizmo/GraphEditor.cpp
)

add_library(engine STATIC ${ENGINE_SOURCES} ${IMGUI_SOURCES})

target_include_directories(engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    libs/imgui
    libs/rlimgui
    libs/imguizmo
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/bullet3/src
)

# --- Asset Bundler ---
add_subdirectory(tools/asset_bundler)

# --- Game Projects ---
# add_subdirectory(projects/SheepRace)
# add_subdirectory(projects/WuShangClan)
# add_subdirectory(projects/TheFresnelEffect)
add_subdirectory(projects/BudgetFlightSimulator)

if(WIN32)
    target_link_libraries(engine PUBLIC raylib opengl32 gdi32 winmm BulletDynamics BulletCollision LinearMath)
else()
    target_link_libraries(engine PUBLIC raylib BulletDynamics BulletCollision LinearMath)
endif()
