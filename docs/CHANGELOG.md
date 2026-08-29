# Engine Refactoring Changelog

This document outlines the significant changes made to the project structure and build system to decouple the game engine from the "SheepRace" game, making the engine reusable for other projects.

## Summary of Changes

The primary goal was to transform the existing monolithic project into a modular structure where the core engine is a standalone library, and individual games (like "SheepRace") are separate projects that link against this engine.

### 1. Directory Structure Changes

*   **New `projects/` Directory:** A top-level `projects/` directory was created to house individual game projects.
*   **`SheepRace` Project Creation:** The game-specific headers (`include/game/`), source files (`src/game/`), and all assets (`resources/`) were moved into a new `projects/SheepRace/` directory.
    *   `include/game/` -> `projects/SheepRace/include/game/`
    *   `src/game/` -> `projects/SheepRace/src/game/`
    *   `resources/` -> `projects/SheepRace/resources/` (The original `resources/` directory was moved entirely).
*   **Engine Code Location:** The core engine files remain in `include/engine/` and `src/engine/`.

### 2. `CMakeLists.txt` (Root) Changes

*   **Project Name Change:** The main project name was changed from `sheepRace` to `GameEngine`.
*   **Engine Library Creation:** The `ENGINE_SOURCES` and `IMGUI_SOURCES` (ImGui, rlImGui, ImGuizmo) are now compiled into a static library named `engine`.
*   **Removed Game-Specific Logic:** All `GAME_SOURCES`, `MAIN_SOURCE`, asset copying, asset bundling custom commands, and Windows-specific post-build copy commands related to `SheepRace` were removed from the root `CMakeLists.txt`.
*   **Removed `launcher` Executable:** The `launcher` executable, which previously built `src/main.cpp`, was removed. The responsibility for creating executables now lies with individual game projects.
*   **`add_subdirectory(projects/SheepRace)`:** The `projects/SheepRace` directory is now added as a subdirectory, allowing its `CMakeLists.txt` to be processed.
*   **Engine Linkage:** The engine library's external dependencies (raylib, Bullet) are now explicitly linked to the `engine` library itself, ensuring that any project linking to `engine` automatically gets these dependencies.

### 3. `projects/SheepRace/CMakeLists.txt` (New File)

*   **New Game Project:** A new `CMakeLists.txt` was created at `projects/SheepRace/CMakeLists.txt`.
*   **`sheep_race` Executable:** This file defines the `sheep_race` executable.
*   **Includes Generic `main.cpp`:** The executable now explicitly includes `../../src/main.cpp` (the generic engine entry point) as one of its source files.
*   **Includes New `game.cpp`:** A new `src/game/game.cpp` (containing `RegisterGameScenes`) is included in the build.
*   **Game-Specific Source Files:** The `SheepRace`-specific source files (`playerSheep.cpp`, `SheepPhysicsComponent.cpp`, `MainMenuScene.cpp`, `GameScene.cpp`) are included.
*   **Include Directories:** Correct include directories (`include`, `../../include`) are set for the `sheep_race` target.
*   **Links to `engine` Library:** The `sheep_race` executable now links against the newly created `engine` static library.
*   **Asset Bundling:** All asset bundling logic (using `asset_bundler`) and asset copying (for Windows) specific to `SheepRace` was moved into this `CMakeLists.txt` file.

### 4. `src/main.cpp` Changes

*   **Generic Entry Point:** `src/main.cpp` was refactored to be a generic engine entry point.
*   **Removed Game-Specific Includes:** Includes for `game/MainMenuScene.h` and `game/GameScene.h` were removed.
*   **Removed Direct Scene Registration:** The direct instantiation and registration of `MainMenuScene` and `GameScene` were removed.
*   **Introduced `RegisterGameScenes`:** The `main` function now calls a `void RegisterGameScenes(Engine& engine)` function. This function is expected to be implemented by the specific game project.

### 5. `projects/SheepRace/src/game/game.cpp` (New File)

*   **Implementation of `RegisterGameScenes`:** This new file provides the concrete implementation of `void RegisterGameScenes(Engine& engine)` for the "SheepRace" game.
*   **Game-Specific Scene Logic:** It contains the original scene instantiation and registration logic (for `MainMenuScene` and `GameScene`) that was previously present in `src/main.cpp`.
*   **Initial Scene Loading:** It also handles loading the initial scene for "SheepRace" (`MainMenu`).

These changes ensure a clear separation of concerns, allowing the engine to be developed independently and reused across multiple game projects more easily.
