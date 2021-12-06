CPMAddPackage(
	NAME DirectXShaderCompiler 
	URL https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.6.2106/dxc_2021_07_01.zip)

file(COPY ${DirectXShaderCompiler_SOURCE_DIR}/bin/x64/dxcompiler.dll DESTINATION ${ZE_BIN_DIR})
set(DXC_ROOT_DIR ${DirectXShaderCompiler_SOURCE_DIR} CACHE STRING "" FORCE)
set(DXC_INCLUDE_DIR ${DXC_ROOT_DIR}/inc CACHE STRING "" FORCE)
set(DXC_LIB_DIR ${DXC_ROOT_DIR}/lib/x64 CACHE STRING "" FORCE)