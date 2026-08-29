# Log of Engine Refactoring Session

This document summarizes the conversation and actions taken to refactor the game engine, decoupling it from the "SheepRace" game logic to make it reusable.

## Objective

The initial request was to understand how to reuse the game engine for new projects, as the existing documentation was outdated. The goal was to separate the generic engine code from the game-specific "SheepRace" code.

## Refactoring Process

The following steps were taken to restructure the project:

1.  **Directory Restructuring**:
    *   A new top-level `projects/` directory was created.
    *   All game-specific code and assets were moved into `projects/SheepRace/`. This included:
        *   `include/game/` -> `projects/SheepRace/include/game/`
        *   `src/game/` -> `projects/SheepRace/src/game/`
        *   The entire `resources/` directory -> `projects/SheepRace/resources/`

2.  **CMake Build System Overhaul**:
    *   **Root `CMakeLists.txt`**:
        *   Modified to define the engine as a static library named `engine`.
        *   All game-specific build logic (`sheepRace` executable, asset bundling) was removed.
        *   An `add_subdirectory(projects/SheepRace)` command was added to include the game project in the build.
    *   **New `projects/SheepRace/CMakeLists.txt`**:
        *   A new `CMakeLists.txt` was created for the game.
        *   It defines the `sheep_race` executable, including the now-generic `../../src/main.cpp`.
        *   It links the executable against the `engine` library.
        *   It contains all asset bundling and copying logic specific to SheepRace.

3.  **Code Decoupling**:
    *   **`src/main.cpp`**: Was made generic by removing all direct references to `GameScene` and `MainMenuScene`. It now calls an external `RegisterGameScenes(Engine& engine)` function.
    *   **`projects/SheepRace/src/game/game.cpp`**: A new file was created to implement the `RegisterGameScenes` function specifically for SheepRace, containing the scene registration logic that was originally in `main.cpp`.

## Bugs Encountered and Solutions

We worked through several build errors after the refactoring:

1.  **Error: `No rule to make target 'asset_bundler'`**
    *   **Cause**: This error appeared when trying to build the `sheep_race` target. The `asset_bundler` executable was not being built before it was needed. The root cause was identified as running `cmake` from within a project subdirectory (`projects/SheepRace/build`), which meant the root `CMakeLists.txt` (where the `asset_bundler` tool is defined) was never processed.
    *   **Solution**: The correct build procedure was established: create a `build` directory at the project root, `cd` into it, and run `cmake ..` from there.

2.  **Error: `fatal error: game/playerSheep.h: No such file or directory`**
    *   **Cause**: A core engine file, `include/engine/Renderer.h`, was including a game-specific file. This broke the engine/game separation.
    *   **Solution**: The `#include "game/playerSheep.h"` was found to be an unnecessary remnant of old code. It was removed from both `Renderer.h` and `Renderer.cpp`, completely decoupling the renderer from the game.

3.  **Error: `Source directory ".../projects/SheepRace/include/engine/shader" does not exist`**
    *   **Cause**: The asset bundling commands in `projects/SheepRace/CMakeLists.txt` were using the variable `CMAKE_CURRENT_SOURCE_DIR`, which resolved to the wrong path after the files were moved.
    *   **Solution**: Replaced all instances of `CMAKE_CURRENT_SOURCE_DIR` with `CMAKE_SOURCE_DIR` in the asset bundling commands. This variable always points to the project's top-level directory, ensuring the correct paths are used.

4.  **Issue: Assets not copied to the executable directory on Linux.**
    *   **Cause**: The game compiled, but the bundled asset files (`.pack`, `.bin`) were not next to the `sheep_race` executable. The `add_custom_command` responsible for copying these files was wrapped in an `if(WIN32)` block, so it never ran on Linux.
    *   **Solution**: The `if(WIN32)` condition was removed, making the asset-copying `POST_BUILD` command platform-agnostic.

## Final Outcome & Documentation

*   The engine is now a self-contained static library (`libengine.a`).
*   "SheepRace" is a standalone project that links against the engine.
*   The build system is centralized and handles multiple game projects.
*   New documentation was created:
    *   `HOW_TO_REUSE_ENGINE.md`: Explains how to create a new game project.
    *   `HOW_TO_CREATE_A_WORLD.md`: A guide with template files for creating a new 3D scene from scratch.
    *   `CHANGELOG.md`: Details the refactoring changes.
*   This file, `REFACTORING_LOG.md`, was created to log this session.
