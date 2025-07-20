#!/usr/bin/bash

# This script builds the project
mkdir -p build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
