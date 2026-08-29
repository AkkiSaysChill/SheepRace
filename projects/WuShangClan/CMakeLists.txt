# Set the project name
project(WuShangClan)

# Explicitly list all source files for the executable
set(GAME_SOURCES
    ../../src/main.cpp
    src/game/main.cpp
    src/game/weapon.cpp
    src/game/WuShangClanScene.cpp
    src/game/enemy.cpp
    src/game/ChatSystem.cpp
)

# Create the executable
add_executable(WuShangClan_run ${GAME_SOURCES})

# Link against the main engine library
target_link_libraries(WuShangClan_run PRIVATE engine)

# Define include directories
target_include_directories(WuShangClan_run PUBLIC 
    "include"
    "../../include"
)

# --- Asset Bundling ---
add_custom_command(
    OUTPUT
        ${CMAKE_BINARY_DIR}/shaders.bin
        ${CMAKE_BINARY_DIR}/hud.pack
        ${CMAKE_BINARY_DIR}/levelDAT
        ${CMAKE_BINARY_DIR}/level001
        ${CMAKE_BINARY_DIR}/level002
    COMMAND asset_bundler ${CMAKE_SOURCE_DIR}/include/engine/shader ${CMAKE_BINARY_DIR}/shaders.bin
    COMMAND asset_bundler ${CMAKE_SOURCE_DIR}/projects/WuShangClan/resources/hud ${CMAKE_BINARY_DIR}/hud.pack
    COMMAND asset_bundler ${CMAKE_SOURCE_DIR}/projects/WuShangClan/resources/arena ${CMAKE_BINARY_DIR}/levelDAT
    COMMAND asset_bundler ${CMAKE_SOURCE_DIR}/projects/WuShangClan/resources/bosses ${CMAKE_BINARY_DIR}/level001
    COMMAND asset_bundler ${CMAKE_SOURCE_DIR}/projects/WuShangClan/resources/weapon ${CMAKE_BINARY_DIR}/level002
    DEPENDS asset_bundler
    COMMENT "Packing game assets (shaders, ui, skybox and maps)"
)

add_custom_target(BundleGameAsset ALL DEPENDS
    ${CMAKE_BINARY_DIR}/shaders.bin
    ${CMAKE_BINARY_DIR}/hud.pack
    ${CMAKE_BINARY_DIR}/levelDAT
    ${CMAKE_BINARY_DIR}/level001
    ${CMAKE_BINARY_DIR}/level002
)

add_dependencies(WuShangClan_run BundleGameAsset)

# --- Copy assets to build directory ---
add_custom_command(TARGET WuShangClan_run POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/shaders.bin
            $<TARGET_FILE_DIR:WuShangClan_run>/shaders.bin
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/hud.pack
            $<TARGET_FILE_DIR:WuShangClan_run>/hud.pack
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/levelDAT
            $<TARGET_FILE_DIR:WuShangClan_run>/levelDAT
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/level001
            $<TARGET_FILE_DIR:WuShangClan_run>/level001
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/level002
            $<TARGET_FILE_DIR:WuShangClan_run>/level002





       COMMENT "Copying bundled assets to executable directory"
    )

# Copy assets (if any) - you can add more files here
# file(COPY resources/ DESTINATION ${CMAKE_BINARY_DIR}/)
