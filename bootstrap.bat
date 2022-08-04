

if not exist "build" mkdir build
cd build
if not exist "vcpkg" git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
if not exist "vcpkg.exe" call bootstrap-vcpkg.bat

set VCPKG_DEFAULT_TRIPLET=x64-windows
rem Add your library ports here.
vcpkg install --recurse boost-multi-array boost-signals2 catch2 glfw3 globjects stb imgui[glfw-binding,opengl3-binding] spdlog highfive[boost] ktx tinygltf tinyexr
cd ..

cmake -Ax64 -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake ..
cmake --build . --target ALL_BUILD --config Release
cd ..