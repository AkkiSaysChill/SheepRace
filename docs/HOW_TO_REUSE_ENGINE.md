# How to Reuse the Game Engine

This document explains the current architecture of the engine and how to reuse it to create new games or scenes, following the recent refactoring.

## Engine Architecture Overview

The engine is now designed as a standalone static library, decoupled from any specific game logic. Game projects are now separate entities that link against this engine library.

**Key Changes:**
*   The core engine (located in `src/engine/` and `include/engine/`) is compiled into a library named `engine`.
*   Game-specific code and assets are housed within individual directories under `projects/`.
*   A game project is responsible for providing its own `main` function (which will reside in the generic `src/main.cpp` now included in each game executable) and implementing a `RegisterGameScenes` function to initialize its scenes.

## Directory Structure (Example with SheepRace)

```
/
├── include/
│   └── engine/        # Core engine headers
├── src/
│   └── engine/        # Core engine source files
│   └── main.cpp       # Generic engine entry point (will be linked by game projects)
├── libs/              # External dependencies (raylib, bullet3, imgui, etc.)
├── projects/
│   └── SheepRace/     # Example game project
│       ├── include/
│       │   └── game/  # SheepRace-specific headers (e.g., scenes, components)
│       ├── src/
│       │   └── game/  # SheepRace-specific source files (including game.cpp)
│       └── resources/ # SheepRace-specific assets
└── CMakeLists.txt     # Main CMake configuration (builds engine library, adds projects)
```

## Creating a New Game Project

To create a new game using this engine:

### 1. Create Project Directories

Create a new directory for your game under `projects/`. For example, `projects/MyNewGame`. Inside this directory, create `src/`, `include/`, and `resources/` subdirectories:

```bash
mkdir -p projects/MyNewGame/src/game
mkdir -p projects/MyNewGame/include/game
mkdir projects/MyNewGame/resources
```

### 2. Create a CMakeLists.txt for Your Game

Create a `projects/MyNewGame/CMakeLists.txt` file. This file will define your game's executable, link it to the engine, and manage its assets. You can use `projects/SheepRace/CMakeLists.txt` as a template.

**Example `projects/MyNewGame/CMakeLists.txt`:**

```cmake
cmake_minimum_required(VERSION 3.12)
project(MyNewGame)

# --- Game Executable ---
set(GAME_SOURCES
    ../../src/main.cpp             # Link the generic main entry point
    src/game/MyNewGameEntry.cpp    # Your game's scene registration implementation
    # Add other game-specific source files here (e.g., src/game/MyScene.cpp)
)

add_executable(my_new_game ${GAME_SOURCES})

# --- Include Directories ---
target_include_directories(my_new_game PRIVATE 
    include                         # For your game's own headers
    ../../include                   # For engine headers
)

# --- Link against the engine ---
target_link_libraries(my_new_game PRIVATE engine)

# --- Asset Bundling (Example - adjust for your game's assets) ---
# Add custom commands to run asset_bundler for your game's specific resources.
# Example:
# add_custom_command(
#     OUTPUT ${CMAKE_BINARY_DIR}/my_game_assets.pack
#     COMMAND asset_bundler ${CMAKE_CURRENT_SOURCE_DIR}/resources/my_assets ${CMAKE_BINARY_DIR}/my_game_assets.pack
#     DEPENDS asset_bundler
#     COMMENT "Packing MyNewGame assets"
# )
# add_custom_target(BundleMyGameAssets ALL DEPENDS ${CMAKE_BINARY_DIR}/my_game_assets.pack)
# add_dependencies(my_new_game BundleMyGameAssets)

# --- Copy assets to build directory (Windows-specific example) ---
if(WIN32)
    add_custom_command(TARGET my_new_game POST_BUILD
        # COMMAND ${CMAKE_COMMAND} -E copy_if_different
        #     ${CMAKE_BINARY_DIR}/my_game_assets.pack
        #     $<TARGET_FILE_DIR:my_new_game>/my_game_assets.pack
        COMMENT "Copying bundled assets to executable directory for MyNewGame"
    )
endif()
```

### 3. Implement `RegisterGameScenes`

Your game project must implement the `RegisterGameScenes` function, which will be called by the generic `main.cpp` to set up your game's scenes.

Create a source file (e.g., `projects/MyNewGame/src/game/MyNewGameEntry.cpp`) and its corresponding header (e.g., `projects/MyNewGame/include/game/MyNewGameEntry.h`).

**`projects/MyNewGame/include/game/MyNewGameEntry.h` (Example):**
```cpp
#pragma once
#include "engine/Engine.h" // Include engine header for Engine class

// This function will be called by the main entry point to register your game's scenes.
void RegisterGameScenes(Engine& engine);
```

**`projects/MyNewGame/src/game/MyNewGameEntry.cpp` (Example):**
```cpp
#include "game/MyNewGameEntry.h"
#include "engine/SceneManager.h"
#include "engine/asset_loader.hpp"
// Include your game's specific scene headers here
// #include "game/MyMainMenuScene.h" 
// #include "game/MyGameLevelScene.h"

void RegisterGameScenes(Engine& engine) {
    auto& sceneManager = engine.GetSceneManager();
    auto& assetLoader = engine.GetAssetLoader();

    // Register your game's scenes
    // sceneManager.RegisterScene("MainMenu", std::make_unique<MyMainMenuScene>());
    // sceneManager.RegisterScene("Level1", std::make_unique<MyGameLevelScene>(assetLoader));

    // Load the initial scene
    // sceneManager.LoadScene("MainMenu"); 
    
    // For a new project, you might want to initially do nothing or load a placeholder scene.
    // For now, let's load a default scene from the engine if available, or do nothing.
    // engine.GetSceneManager().LoadScene("DefaultEngineScene"); // if you have one
}
```
Remember to include your custom scene headers and define your scenes within this file.

### 4. Add Your New Project to the Root CMakeLists.txt

Finally, open the root `CMakeLists.txt` and add your new game project as a subdirectory:

```cmake
# --- Game Projects ---
add_subdirectory(projects/SheepRace)
add_subdirectory(projects/MyNewGame) # Add this line
```

After these steps, you can configure your CMake project and build your new game executable.
