ze_add_module(shadersystem
	public/engine/shadersystem/shader_permutation_id.hpp
	public/engine/shadersystem/shader_declaration.hpp
	public/engine/shadersystem/shader_manager.hpp
	public/engine/shadersystem/shader.hpp
	private/engine/shadersystem/shader_declaration.cpp
	private/engine/shadersystem/shader_manager.cpp
	private/engine/shadersystem/shader_permutation.cpp
	private/engine/shadersystem/shader_instance.cpp
	private/engine/shadersystem/zeshader_compiler.hpp
	private/engine/shadersystem/zeshader_compiler.cpp
	private/engine/shadersystem/shader.cpp)
target_include_directories(shadersystem PUBLIC public PRIVATE private)
target_link_libraries(shadersystem PUBLIC core gfx jobsystem PRIVATE shadercompiler filesystem)