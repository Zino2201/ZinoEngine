ze_add_module(shadercompiler
	public/engine/shadercompiler/shader_compiler.hpp
	public/engine/shadercompiler/shader_compiler_module.hpp
	private/engine/shadercompiler/shader_compiler.cpp)
target_include_directories(shadercompiler PUBLIC public PRIVATE private)
target_link_libraries(shadercompiler PUBLIC core gfx)
