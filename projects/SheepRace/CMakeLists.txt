cmake_minimum_required(VERSION 3.12)
project(SheepRace)

# --- Game Executable ---
set(GAME_SOURCES
    ../../src/main.cpp
    src/game/game.cpp
    src/game/playerSheep.cpp
    src/game/SheepPhysicsComponent.cpp
    src/game/MainMenuScene.cpp
    src/game/GameScene.cpp
)

add_executable(sheep_race ${GAME_SOURCES})

# --- Include Directories ---
target_include_directories(sheep_race PRIVATE 
    include
    ../../include
)

# --- Link against the engine ---
target_link_libraries(sheep_race PRIVATE engine)

# --- Asset Bundling ---
add_custom_command(
    OUTPUT
        ${CMAKE_BINARY_DIR}/shaders.bin
        ${CMAKE_BINARY_DIR}/cars.pack
        ${CMAKE_BINARY_DIR}/skybox.pack
        ${CMAKE_BINARY_DIR}/levels.map
        ${CMAKE_BINARY_DIR}/effects.dyum
    COMMAND asset_bundler ${CMAKE_SOURCE_DIR}/include/engine/shader ${CMAKE_BINARY_DIR}/shaders.bin
    COMMAND asset_bundler ${CMAKE_SOURCE_DIR}/projects/SheepRace/resources/maps ${CMAKE_BINARY_DIR}/levels.map
    COMMAND asset_bundler ${CMAKE_SOURCE_DIR}/projects/SheepRace/resources/cars ${CMAKE_BINARY_DIR}/cars.pack
    COMMAND asset_bundler ${CMAKE_SOURCE_DIR}/projects/SheepRace/resources/skybox ${CMAKE_BINARY_DIR}/skybox.pack
    COMMAND asset_bundler ${CMAKE_SOURCE_DIR}/projects/SheepRace/resources/effectTex ${CMAKE_BINARY_DIR}/effects.dyum 
    DEPENDS asset_bundler
    COMMENT "Packing game assets (shaders, cars, skybox and maps)"
)

add_custom_target(BundleGameAssets ALL DEPENDS
    ${CMAKE_BINARY_DIR}/shaders.bin
    ${CMAKE_BINARY_DIR}/cars.pack
    ${CMAKE_BINARY_DIR}/levels.map
    ${CMAKE_BINARY_DIR}/skybox.pack
)

add_dependencies(sheep_race BundleGameAssets)

# --- Copy assets to build directory ---
    add_custom_command(TARGET sheep_race POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/shaders.bin
            $<TARGET_FILE_DIR:sheep_race>/shaders.bin
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/cars.pack
            $<TARGET_FILE_DIR:sheep_race>/cars.pack
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/skybox.pack
            $<TARGET_FILE_DIR:sheep_race>/skybox.pack
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/levels.map
            $<TARGET_FILE_DIR:sheep_race>/levels.map
        COMMENT "Copying bundled assets to executable directory"
    )
