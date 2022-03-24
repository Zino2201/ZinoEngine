find_package(assimp REQUIRED)
find_package(DirectXTex REQUIRED)

ze_add_module(engine 
	public/engine/tinyobjloader.h
	public/engine/engine.hpp
	private/engine/engine.cpp)
target_include_directories(engine PUBLIC public PRIVATE private)
target_link_libraries(engine PUBLIC core application imgui vulkangfx gfxutils shadersystem filesystem materialsystem rendergraph
	assimp::assimp Microsoft::DirectXTex)