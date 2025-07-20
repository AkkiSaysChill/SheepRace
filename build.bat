@echo off
REM This script builds the project for Windows using CMake and MSBuild.

REM Enable delayed expansion for robust error checking.
SETLOCAL ENABLEDELAYEDEXPANSION

REM --- Create the build directory ---
echo Creating build directory...
mkdir build
REM Check if build directory was created successfully
IF NOT EXIST build\ (
  echo Error: Failed to create build directory.
  EXIT /B 1
)
echo Build directory created/exists.

REM --- Configure the project with CMake ---
echo Configuring project with CMake...
REM Specify a Visual Studio generator. Adjust "Visual Studio 17 2022" if you have a different version.
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022"
REM Check if CMake configuration was successful
IF %ERRORLEVEL% NEQ 0 (
  echo Error: CMake configuration failed.
  EXIT /B 1
)
echo CMake configuration complete.

REM --- Always repack the assets ---
echo Repacking assets from resources\cars\ into build\cars.pack...
REM Ensure the asset_bundler executable exists before trying to run it
IF NOT EXIST ".\tools\asset_bundler\build\asset_bundler.exe" (
  echo Error: Asset bundler executable not found at .\tools\asset_bundler\build\asset_bundler.exe.
  echo Please ensure the asset_bundler project has been built.
  EXIT /B 1
)
".\tools\asset_bundler\build\asset_bundler.exe" ".\resources\cars" ".\build\Release\cars.pack"
REM Check if asset bundling was successful
IF %ERRORLEVEL% NEQ 0 (
  echo Error: Asset bundling failed.
  EXIT /B 1
)
echo Asset bundling complete.

REM --- Change to the build directory and compile the project ---
echo Changing to build directory and compiling project...
cd build
REM Pause for a short duration (equivalent to sleep 0.6s)
timeout /t 0.6 /nobreak >nul

echo Compiling project with MSBuild...
REM Assuming CMake generated a Visual Studio project file like ALL_BUILD.vcxproj
REM If CMake generates a .sln file (e.g., sheepRace.sln), you might need to use:
REM msbuild sheepRace.sln /p:Configuration=Release
msbuild ALL_BUILD.vcxproj /p:Configuration=Release
REM Check if compilation was successful
IF %ERRORLEVEL% NEQ 0 (
  echo Error: Project compilation failed.
  EXIT /B 1
)
echo Project compiled successfully.

REM --- Provide instructions to run the game ---
echo.
echo ----------------------------------------------------
echo Build process finished successfully!
echo You can now run the game:

REM Check if the executable exists before suggesting to run it
REM Changing directory
cd Release
IF EXIST ".\sheepRace.exe" (
  echo .\sheepRace.exe
  echo.
  echo Attempting to run the game now...
  .\sheepRace.exe
) ELSE (
  echo Error: 'sheepRace.exe' executable not found in the build directory.
  echo Please check your CMakeLists.txt and compilation output.
  EXIT /B 1
)
echo ----------------------------------------------------

REM End local environment changes
ENDLOCAL
