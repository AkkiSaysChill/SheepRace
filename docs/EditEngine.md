# Editing and Extending the Game Engine

This document provides instructions on how to edit and extend the game engine.

## Introduction

The engine is designed to be modular, allowing you to modify existing systems or add new ones. The core engine logic resides in the `src/engine/` and `include/engine/` directories.

## Editing the Renderer

The `Renderer` class (`src/engine/Renderer.cpp` and `include/engine/Renderer.h`) is responsible for all rendering in the engine.

The main rendering logic is in the `Renderer::Render()` method. This method orchestrates the different rendering passes, such as shadow mapping, skybox rendering, and the main scene rendering.

To add a new rendering pass, you can modify the `Renderer::Render()` method. For example, to add a debug rendering pass, you could add a call to a `DebugDraw()` method at the end of the `Render()` method.

## Adding Post-Processing Effects

The `PostProcessing` class (`src/engine/PostProcessing.cpp` and `include/engine/PostProcessing.h`) manages the post-processing effects.

To add a new post-processing effect:

1.  **Create the Shader:**
    *   Create your new fragment shader file (e.g., `my_effect.fs`) and place it in the `include/shader/glsl330/` directory.

2.  **Update the Asset Bundler:**
    *   The asset bundler is configured in the main `CMakeLists.txt` file. The shaders are packed into `shaders.bin`. The bundler automatically finds all files in the `include/shader` directory, so you don't need to manually add your new shader to the list.

3.  **Update `PostProcessing.h`:**
    *   Open `include/engine/PostProcessing.h`.
    *   Add your new effect to the `PostproShader` enum.
    *   Add the name of your effect to the `postproShaderText` array.

    ```cpp
    typedef enum {
        // ... existing effects
        FX_MY_EFFECT,
    } PostproShader;

    static const char *postproShaderText[] = {
        // ... existing effect names
        "MY_EFFECT",
    };
    ```

4.  **Update `PostProcessing.cpp`:**
    *   Open `src/engine/PostProcessing.cpp`.
    *   In the `PostProcessing` constructor, add your new shader to the `shader_names` array and increase the size of the `shader` array.

    ```cpp
    PostProcessing::PostProcessing(AssetPacking::AssetLoader& assetLoader) {
        // ...
        const char *shader_names[] = {
            // ... existing shader names
            "glsl330/my_effect.fs"
        };

        // Increase the loop size to include your new shader
        for (int i = 0; i < 14; i++) { // 13 + 1 new shader
            // ...
        }
        // ...
    }
    ```

## Normal Shader Loading

Not all shaders need to be part of the post-processing pipeline. For example, the `sky.fs` shader is loaded directly by the `Renderer`.

The `sky.fs` shader is located in the `include/shader/sky/` directory. It is packed into `skybox.pack` by the asset bundler in `CMakeLists.txt`.

The `Renderer` then loads this shader from the `skybox.pack` file using the `AssetLoader`.

**`CMakeLists.txt`**
```cmake
COMMAND asset_bundler ${CMAKE_CURRENT_SOURCE_DIR}/resources/skybox ${CMAKE_BINARY_DIR}/skybox.pack
```

**`src/engine/Renderer.cpp`**
```cpp
Renderer::Renderer(int screenWidth, int screenHeight, AssetPacking::AssetLoader& assetLoader) {
    // ...
    std::vector<char> skyShaderFS = assetLoader.getAssetData("sky/sky.fs");
    skyShaderFS.push_back('\0');
    m_skyShader = LoadShaderFromMemory(0, skyShaderFS.data());
    // ...
}
```

You can use this method to load any shader at any time, as long as it has been packed by the asset bundler.

## Adding New Engine Systems

To add a new engine system (e.g., an `AudioSystem`):

1.  **Create the System:**
    *   Create `AudioSystem.h` in `include/engine/` and `AudioSystem.cpp` in `src/engine/`.
    *   Implement your system's logic in this class.

2.  **Integrate with the `Engine`:**
    *   Open `include/engine/Engine.h` and add a member variable for your new system (e.g., `AudioSystem* m_audioSystem;`).
    *   In the `Engine`'s constructor, initialize your system.
    *   In `Engine::Initialize()`, call your system's `Initialize()` method.
    *   In `Engine::Run()`, call your system's `Update()` method in the main loop.
    *   In `Engine::Shutdown()`, call your system's `Shutdown()` method.

3.  **Update `CMakeLists.txt`:**
    *   Add your new `AudioSystem.cpp` to the `ENGINE_SOURCES` list in `CMakeLists.txt`.
