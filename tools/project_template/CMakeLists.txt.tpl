# Set the project name
project(%%PROJECT_NAME%%)

# Explicitly list all source files for the executable
set(GAME_SOURCES
    ../../src/main.cpp
    src/game/main.cpp
    src/game/%%SCENE_NAME%%.cpp
)

# Create the executable
add_executable(%%PROJECT_NAME%%_run ${GAME_SOURCES})

# Link against the main engine library
target_link_libraries(%%PROJECT_NAME%%_run PRIVATE engine)

# Define include directories
target_include_directories(%%PROJECT_NAME%%_run PUBLIC 
    "include"
    "../../include"
)

# Copy assets (if any) - you can add more files here
# file(COPY resources/ DESTINATION ${CMAKE_BINARY_DIR}/)
