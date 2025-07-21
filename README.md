~~# Sheep Racing (cant think of a better name)~~
# SHEEPER BUILDS (think of a better name)

- made in c++ using raylib imgui 

- uses raylib for graphics and imgui for GUI 


* to bundle the resources 
```
./tools/asset_bundler/build/asset_bundler resources/ build/asset.pack 
```

- to build the asset_bundler tool for windows 
```
mkdir tools/asset_bundler/build
cd tools/asset_bundler/build
cmake .. -G "Visual Studio 17 2022" -A x64 
MSBuild asset_bundler.sln /p:Configuration=Release
```

- the asset_bundler for linux is prebuilt and included in the repository, so you can just run it directly from the tools/asset_bundler/build directory.

## build on LINUX/OSX 
- uses cmake for build system

```
mkdir build
cd build
cmake ..
make 
``` 

- now there should be a sheepRace executable in the build directory

## to build on windows

- install visual studio 2022 with c++ build tools 

```
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
MSBuild sheepRace.sln /p:Configuration=Release
```

- now there should be a sheepRace.exe in the build directory 


* the windows build is not tested, 

