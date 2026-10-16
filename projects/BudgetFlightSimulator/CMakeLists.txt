# Set the project name
project(BudgetFlightSimulator)

# Explicitly list all source files for the executable
set(GAME_SOURCES
    ../../src/main.cpp
    src/game/main.cpp
    src/game/BudgetFlightSimulatorScene.cpp
    src/game/Aircraft.cpp
    src/game/FlightCamera.cpp
    src/game/tinyxml2.cpp
    src/game/MainMenuScene.cpp
    src/game/HangarSelectScene.cpp
    src/game/TerrainMap.cpp
    src/game/Ocean.cpp
    src/game/MissionDatabase.cpp
    src/game/MissionManager.cpp
    src/game/MissionSelectScene.cpp
    src/game/LoadingScene.cpp
)

# Create the executable
add_executable(BudgetFlightSimulator_run ${GAME_SOURCES})

# Link against the main engine library
target_link_libraries(BudgetFlightSimulator_run PRIVATE engine)

# Define include directories
target_include_directories(BudgetFlightSimulator_run PUBLIC 
    "include"
    "../../include"
)

# Define resource directories
set(RES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)

# Ensure DATA directories exist in current binary directory before bundling
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/DATA/MISSIONS)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/DATA/SFX)

# --- Asset Bundling (Scoped per Project Target) ---
add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/shaders.bin
        ${CMAKE_CURRENT_BINARY_DIR}/planes.woosh
        ${CMAKE_CURRENT_BINARY_DIR}/city.bin
        ${CMAKE_CURRENT_BINARY_DIR}/houses.bin
        ${CMAKE_CURRENT_BINARY_DIR}/trees.bin
        ${CMAKE_CURRENT_BINARY_DIR}/DATA/MISSIONS/missions.bin
        ${CMAKE_CURRENT_BINARY_DIR}/DATA/SFX/sfx.bin
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/DATA/MISSIONS
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/DATA/SFX
    COMMAND asset_bundler ${CMAKE_SOURCE_DIR}/include/engine/shader ${CMAKE_CURRENT_BINARY_DIR}/shaders.bin
    COMMAND asset_bundler ${RES_DIR}/planes ${CMAKE_CURRENT_BINARY_DIR}/planes.woosh
    COMMAND asset_bundler ${RES_DIR}/city ${CMAKE_CURRENT_BINARY_DIR}/city.bin
    COMMAND asset_bundler ${RES_DIR}/houses ${CMAKE_CURRENT_BINARY_DIR}/houses.bin
    COMMAND asset_bundler ${RES_DIR}/trees ${CMAKE_CURRENT_BINARY_DIR}/trees.bin
    COMMAND asset_bundler ${RES_DIR}/missions ${CMAKE_CURRENT_BINARY_DIR}/DATA/MISSIONS/missions.bin
    COMMAND asset_bundler ${RES_DIR}/sfx ${CMAKE_CURRENT_BINARY_DIR}/DATA/SFX/sfx.bin
    DEPENDS asset_bundler
    COMMENT "Packing BudgetFlightSimulator game assets (shaders, planes, city, houses, trees, missions, sfx)"
)

# Use ${PROJECT_NAME}_BundleAssets to guarantee a globally unique target name
add_custom_target(${PROJECT_NAME}_BundleAssets ALL DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/shaders.bin
    ${CMAKE_CURRENT_BINARY_DIR}/planes.woosh
    ${CMAKE_CURRENT_BINARY_DIR}/city.bin
    ${CMAKE_CURRENT_BINARY_DIR}/houses.bin
    ${CMAKE_CURRENT_BINARY_DIR}/trees.bin
    ${CMAKE_CURRENT_BINARY_DIR}/DATA/MISSIONS/missions.bin
    ${CMAKE_CURRENT_BINARY_DIR}/DATA/SFX/sfx.bin
)

add_dependencies(BudgetFlightSimulator_run ${PROJECT_NAME}_BundleAssets)

# --- Copy assets to build directory ---
add_custom_command(TARGET BudgetFlightSimulator_run POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:BudgetFlightSimulator_run>/DATA/MISSIONS
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:BudgetFlightSimulator_run>/DATA/SFX
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/shaders.bin
        $<TARGET_FILE_DIR:BudgetFlightSimulator_run>/shaders.bin
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/planes.woosh
        $<TARGET_FILE_DIR:BudgetFlightSimulator_run>/planes.woosh
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/city.bin
        $<TARGET_FILE_DIR:BudgetFlightSimulator_run>/city.bin
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/houses.bin
        $<TARGET_FILE_DIR:BudgetFlightSimulator_run>/houses.bin
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/trees.bin
        $<TARGET_FILE_DIR:BudgetFlightSimulator_run>/trees.bin
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/DATA/MISSIONS/missions.bin
        $<TARGET_FILE_DIR:BudgetFlightSimulator_run>/DATA/MISSIONS/missions.bin
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/DATA/SFX/sfx.bin
        $<TARGET_FILE_DIR:BudgetFlightSimulator_run>/DATA/SFX/sfx.bin
    COMMENT "Copying bundled assets to executable directory"
)
