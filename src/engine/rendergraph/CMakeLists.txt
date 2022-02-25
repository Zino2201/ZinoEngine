ze_add_module(rendergraph
	public/engine/gfx/rendergraph/render_graph.hpp
	public/engine/gfx/rendergraph/resource_registry.hpp
	private/engine/gfx/rendergraph/render_pass.cpp
	private/engine/gfx/rendergraph/resource_registry.cpp
	private/engine/gfx/rendergraph/render_graph.cpp)
target_include_directories(rendergraph PUBLIC public PRIVATE private)
target_link_libraries(rendergraph PUBLIC core gfx)
