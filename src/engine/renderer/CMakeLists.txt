ze_add_module(renderer
	public/engine/renderer/render_node.hpp
	public/engine/renderer/world.hpp
	public/engine/renderer/render_pass.hpp
	public/engine/renderer/drawcall.hpp
	private/engine/renderer/render_node.cpp
	private/engine/renderer/render_pass.cpp
	private/engine/renderer/world.cpp)
target_include_directories(renderer PUBLIC public PRIVATE private)
target_link_libraries(renderer PUBLIC core gfx)
