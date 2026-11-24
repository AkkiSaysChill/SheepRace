# Set the project name
project(VolumetricClouds)

# Explicitly list all source files for the executable
set(GAME_SOURCES
    ../../src/main.cpp
    src/game/main.cpp
    src/game/VolumetricCloudsScene.cpp
)

# Create the executable
add_executable(VolumetricClouds_run ${GAME_SOURCES})

# Link against the main engine library
target_link_libraries(VolumetricClouds_run PRIVATE engine)

# Define include directories
target_include_directories(VolumetricClouds_run PUBLIC 
    "include"
    "../../include"
)