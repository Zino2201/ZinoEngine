include(FindLua)
find_package(Lua REQUIRED)
find_package(sol2 CONFIG REQUIRED)
ze_add_module(shadersystem
	public/engine/shadersystem/shader_declaration.hpp
	public/engine/shadersystem/shader_manager.hpp
	public/engine/shadersystem/shader.hpp
	private/engine/shadersystem/shader_declaration.cpp
	private/engine/shadersystem/shader_manager.cpp
	private/engine/shadersystem/shader_permutation.cpp
	private/engine/shadersystem/shader_instance.cpp
	private/engine/shadersystem/shaderbuilder.cpp
	private/engine/shadersystem/shader.cpp)
target_include_directories(shadersystem PUBLIC public PRIVATE private ${LUA_INCLUDE_DIR})
target_link_libraries(shadersystem PUBLIC core gfx jobsystem PRIVATE ${LUA_LIBRARIES} sol2::sol2 shadercompiler filesystem)
target_compile_definitions(shadersystem PRIVATE SOL_ALL_SAFETIES_ON=1 SOL_NO_EXCEPTIONS=1)