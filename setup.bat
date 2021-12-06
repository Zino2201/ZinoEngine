@echo off
echo Cloning vcpkg
git clone https://github.com/microsoft/vcpkg.git --branch 2021.05.12 src/thirdparty/vcpkg
cd src/thirdparty/vcpkg
bootstrap-vcpkg.bat