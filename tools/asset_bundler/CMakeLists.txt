cmake_minimum_required(VERSION 3.10)
project(asset_bundler CXX)

add_executable(asset_bundler main.cpp)

target_link_libraries(asset_bundler PRIVATE stdc++fs)

install(TARGETS asset_bundler DESTINATION bin)