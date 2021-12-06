if(0)
ze_add_module(renderer
	public/engine/renderer/RenderNode.hpp
	public/engine/renderer/World.hpp
	public/engine/renderer/Material.hpp
	public/engine/renderer/RenderPass.hpp
	public/engine/renderer/Drawcall.hpp
	private/engine/renderer/RenderNode.cpp
	private/engine/renderer/RenderPass.cpp
	private/engine/renderer/World.cpp)
target_include_directories(renderer PUBLIC public PRIVATE private)
target_link_libraries(renderer PUBLIC core gfx)
endif()