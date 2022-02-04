ze_add_module(materialsystem
	public/engine/materialsystem/material.hpp
	private/engine/materialsystem/material.cpp)
target_include_directories(materialsystem PUBLIC public PRIVATE private)
target_link_libraries(materialsystem PUBLIC core gfx shadersystem)
