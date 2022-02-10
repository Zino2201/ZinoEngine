ze_add_module(materialsystem 
	public/engine/materialsystem/material.hpp
	public/engine/materialsystem/material_compiler.hpp
	private/engine/materialsystem/material_compiler.cpp
	private/engine/materialsystem/material.cpp)
target_include_directories(materialsystem PUBLIC public PRIVATE private)
target_link_libraries(materialsystem PUBLIC core vulkangfx shadersystem filesystem shadersystem)