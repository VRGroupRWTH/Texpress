#!/bin/bash

if [ ! -d "build" ]; then mkdir build; fi
cd build
if [ ! -d "vcpkg" ]; then git clone https://github.com/Microsoft/vcpkg.git; fi
cd vcpkg
if [ ! -f "vcpkg" ]; then ./bootstrap-vcpkg.sh; fi

VCPKG_DEFAULT_TRIPLET=x64-linux
# Add your library ports here. 
vcpkg install --recurse boost-multi-array boost-signals2 catch2 glfw3 globjects stb imgui[glfw-binding,opengl3-binding] spdlog fp16 highfive[boost] ktx
cd ..

cmake -Ax64 -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake ..
cmake --build . --target ALL_BUILD --config Release
cd ..