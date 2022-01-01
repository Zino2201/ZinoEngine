@echo off
set VCPKG_ROOT=%CD%/src/thirdparty/vcpkg
cd %VCPKG_ROOT%

vcpkg update

echo Upgrading installed libs
vcpkg upgrade

echo Installing libs for x64-windows
vcpkg install fmt:x64-windows
vcpkg install stb:x64-windows
vcpkg install glm:x64-windows
vcpkg install vulkan-memory-allocator:x64-windows
vcpkg install robin-hood-hashing:x64-windows
vcpkg install gtest:x64-windows
vcpkg install imgui:x64-windows
vcpkg install tbb:x64-windows
vcpkg install boost-dynamic-bitset:x64-windows
vcpkg install boost-locale:x64-windows
vcpkg install freetype:x64-windows
vcpkg install sol2:x64-windows
