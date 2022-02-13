find_package(spirv_cross_core REQUIRED)
ze_add_module(vulkanshadercompiler
	private/vulkan_shader_compiler.cpp)
target_include_directories(vulkanshadercompiler PUBLIC public PRIVATE private ${DXC_INCLUDE_DIR})
target_link_directories(vulkanshadercompiler PRIVATE ${DXC_LIB_DIR} ${VCPKG_ROOT}/installed/${TRIPLET}/lib)
target_link_libraries(vulkanshadercompiler PUBLIC shadercompiler PRIVATE filesystem dxcompiler spirv-cross-core)
