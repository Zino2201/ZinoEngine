ze_add_module(gfx 
	public/engine/gfx/device.hpp
	public/engine/gfx/shader_format.hpp
	public/engine/gfx/backend.hpp
	public/engine/gfx/gfx_result.hpp
	public/engine/gfx/memory.hpp
	public/engine/gfx/buffer.hpp
	public/engine/gfx/texture.hpp
	public/engine/gfx/backend_device.hpp
	public/engine/gfx/format.hpp
	public/engine/gfx/shader.hpp
	public/engine/gfx/pipeline.hpp
	public/engine/gfx/pipeline_layout.hpp
	public/engine/gfx/gfx_pipeline.hpp
	public/engine/gfx/render_pass.hpp
	public/engine/gfx/device_resource.hpp
	public/engine/gfx/sampler.hpp
	public/engine/gfx/swapchain.hpp
	public/engine/gfx/command.hpp
	public/engine/gfx/sync.hpp
	public/engine/gfx/rect.hpp
	public/engine/gfx/uniform_buffer.hpp
	private/engine/gfx/threaded_command_pool.cpp
	private/engine/gfx/device.cpp)
target_include_directories(gfx PUBLIC public PRIVATE private)
target_link_libraries(gfx PUBLIC core)