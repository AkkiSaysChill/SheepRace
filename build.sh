#!/usr/bin/bash

# This script builds the project

# Exit immediately if a command exits with a non-zero status.
set -e

# Create the build directory
echo "Creating build directory..."
mkdir -p build

# Check if build directory was created successfully
if [ ! -d "build" ]; then
  echo "Error: Failed to create build directory."
  exit 1
fi
echo "Build directory created/exists."

# Configure the project with CMake
echo "Configuring project with CMake..."
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Check if CMake configuration was successful
if [ $? -ne 0 ]; then
  echo "Error: CMake configuration failed."
  exit 1
fi
echo "CMake configuration complete."

# Always repack the assets
echo "Repacking assets from resources/cars/ into build/cars.pack..."
# Ensure the asset_bundler executable exists before trying to run it
if [[ ! -f "./tools/asset_bundler/build/asset_bundler" ]]; then
  echo "Error: Asset bundler executable not found at ./tools/asset_bundler/build/asset_bundler."
  echo "Please ensure the asset_bundler project has been built."
  exit 1
fi
./tools/asset_bundler/build/asset_bundler ./resources/cars ./build/cars.pack

# Check if asset bundling was successful
if [ $? -ne 0 ]; then
  echo "Error: Asset bundling failed."
  exit 1
fi
echo "Asset bundling complete."

# Change to the build directory and compile the project
echo "Changing to build directory and compiling project..."
cd build
sleep 0.6s

make

# Check if compilation was successful
if [ $? -ne 0 ]; then
  echo "Error: Project compilation failed."
  exit 1
fi
echo "Project compiled successfully."

# Provide instructions to run the game
echo ""
echo "----------------------------------------------------"
echo "Build process finished successfully!"
echo "You can now run the game:"

# Check if the executable exists before suggesting to run it
if [[ -f "./sheepRace" ]]; then
  echo "./sheepRace"
  echo ""
  echo "Attempting to run the game now..."
  ./sheepRace
else
  echo "Error: 'sheepRace' executable not found in the build directory."
  echo "Please check your CMakeLists.txt and compilation output."
  exit 1
fi
echo "----------------------------------------------------"
