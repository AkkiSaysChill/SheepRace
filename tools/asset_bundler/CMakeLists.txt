cmake_minimum_required(VERSION 3.10)
project(asset_bundler LANGUAGES CXX)

# Ensure C++17 standard is used for std::filesystem
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Optional: Ensures strict C++ standard adherence

add_executable(asset_bundler main.cpp)

# The 'stdc++fs' library is specific to GCC/Clang and not needed for MSVC.
# MSVC integrates std::filesystem directly when compiling with C++17 or newer.
# Therefore, we remove the target_link_libraries line for stdc++fs.
# target_link_libraries(asset_bundler PRIVATE stdc++fs) # REMOVED

install(TARGETS asset_bundler DESTINATION bin)
