@echo off
echo Cloning vcpkg
git clone https://github.com/microsoft/vcpkg.git src/thirdparty/vcpkg
cd src/thirdparty/vcpkg
bootstrap-vcpkg.bat