#!/bin/bash

# --- Script to create a new game project from a template ---

# Check for correct number of arguments
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 ProjectName [2d|3d]"
    exit 1
fi

# --- CONFIGURATION ---
PROJECT_NAME=$1
SCENE_TYPE=$2
PROJECTS_DIR="projects"
TEMPLATE_DIR="tools/project_template"

# --- DERIVED NAMES ---
PROJECT_PATH="$PROJECTS_DIR/$PROJECT_NAME"
SCENE_NAME="${PROJECT_NAME}Scene"
SCENE_HEADER_NAME="game/${SCENE_NAME}.h"
SCENE_SOURCE_NAME="game/${SCENE_NAME}.cpp"
MAIN_CPP_NAME="game/main.cpp" # Changed to main.cpp for consistency

echo "--- Creating New Project: $PROJECT_NAME ---"

# --- VALIDATION ---
# Check if project already exists
if [ -d "$PROJECT_PATH" ]; then
    echo "Error: Project '$PROJECT_NAME' already exists at $PROJECT_PATH"
    exit 1
fi

# Check for valid scene type
if [ "$SCENE_TYPE" != "2d" ] && [ "$SCENE_TYPE" != "3d" ]; then
    echo "Error: Invalid scene type '$SCENE_TYPE'. Must be '2d' or '3d'."
    exit 1
fi

echo "Project Name: $PROJECT_NAME"
echo "Scene Type:   $SCENE_TYPE"
echo "Scene Name:   $SCENE_NAME"
echo "Project Path: $PROJECT_PATH"

# --- DIRECTORY CREATION ---
echo "Creating directory structure..."
mkdir -p "$PROJECT_PATH/include/game"
mkdir -p "$PROJECT_PATH/src/game"
echo "Done."

# --- FILE GENERATION ---
echo "Generating files from templates..."

# Copy and process CMakeLists.txt
cp "$TEMPLATE_DIR/CMakeLists.txt.tpl" "$PROJECT_PATH/CMakeLists.txt"
sed -i "s/%%PROJECT_NAME%%/$PROJECT_NAME/g" "$PROJECT_PATH/CMakeLists.txt"
sed -i "s/%%SCENE_NAME%%/$SCENE_NAME/g" "$PROJECT_PATH/CMakeLists.txt"

# Copy and process main.cpp
cp "$TEMPLATE_DIR/main.cpp.tpl" "$PROJECT_PATH/src/$MAIN_CPP_NAME"
sed -i "s/%%SCENE_NAME%%/$SCENE_NAME/g" "$PROJECT_PATH/src/$MAIN_CPP_NAME"

# Copy and process the correct scene files
if [ "$SCENE_TYPE" == "3d" ]; then
    # Header
    cp "$TEMPLATE_DIR/Scene3D.h.tpl" "$PROJECT_PATH/include/$SCENE_HEADER_NAME"
    sed -i "s/%%SCENE_NAME%%/$SCENE_NAME/g" "$PROJECT_PATH/include/$SCENE_HEADER_NAME"
    # Source
    cp "$TEMPLATE_DIR/Scene3D.cpp.tpl" "$PROJECT_PATH/src/$SCENE_SOURCE_NAME"
    sed -i "s/%%SCENE_NAME%%/$SCENE_NAME/g" "$PROJECT_PATH/src/$SCENE_SOURCE_NAME"
else # 2d
    # Header
    cp "$TEMPLATE_DIR/Scene2D.h.tpl" "$PROJECT_PATH/include/$SCENE_HEADER_NAME"
    sed -i "s/%%SCENE_NAME%%/$SCENE_NAME/g" "$PROJECT_PATH/include/$SCENE_HEADER_NAME"
    # Source
    cp "$TEMPLATE_DIR/Scene2D.cpp.tpl" "$PROJECT_PATH/src/$SCENE_SOURCE_NAME"
    sed -i "s/%%SCENE_NAME%%/$SCENE_NAME/g" "$PROJECT_PATH/src/$SCENE_SOURCE_NAME"
fi

echo "Done."
echo ""
echo "--- SUCCESS! ---"
echo "New project '$PROJECT_NAME' created at '$PROJECT_PATH'."
echo ""
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!! IMPORTANT: Add the following line to your root CMakeLists.txt file:"
echo "!! "
echo "!!     add_subdirectory($PROJECT_PATH)"
echo "!! "
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
