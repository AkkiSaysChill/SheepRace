# Set the project namh
project(TheFresnelEffect)

# Explicitly list all source files for the executable
set(GAME_SOURCES
    ../../src/main.cpp
    src/game/main.cpp
    src/game/TheFresnelEffectScene.cpp
    src/game/Water.cpp
)

# Create the executable
add_executable(TheFresnelEffect_run ${GAME_SOURCES})

# Link against the main engine library
target_link_libraries(TheFresnelEffect_run PRIVATE engine)

# Define include directories
target_include_directories(TheFresnelEffect_run PUBLIC 
    "include"
    "../../include"
)



# --- Asset Bundling ---
add_custom_command(
    OUTPUT
        ${CMAKE_BINARY_DIR}/shaders.bin
        ${CMAKE_BINARY_DIR}/map.pack
    COMMAND asset_bundler ${CMAKE_SOURCE_DIR}/include/engine/shader ${CMAKE_BINARY_DIR}/shaders.bin
    COMMAND asset_bundler ${CMAKE_SOURCE_DIR}/projects/TheFresnelEffect/resources/map ${CMAKE_BINARY_DIR}/map.pack
    DEPENDS asset_bundler
    COMMENT "Packing game assets (shaders, ui, skybox and maps)"
)

add_custom_target(BundleGameAssetss ALL DEPENDS
    ${CMAKE_BINARY_DIR}/shaders.bin
)

add_dependencies( TheFresnelEffect_run BundleGameAssetss)

# --- Copy assets to build directory ---
add_custom_command(TARGET  TheFresnelEffect_run POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/shaders.bin
            $<TARGET_FILE_DIR:WuShangClan_run>/shaders.bin
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/map.pack
            $<TARGET_FILE_DIR:TheFresnelEffect_run>/map.pack

       COMMENT "Copying bundled assets to executable directory"
    )


