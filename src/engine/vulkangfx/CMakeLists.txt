find_package(Vulkan REQUIRED)
find_package(unofficial-vulkan-memory-allocator CONFIG REQUIRED)

ze_add_module(vulkangfx
	public/engine/gfx/vulkan_backend_module.hpp
	private/engine/gfx/vulkan_backend.hpp
	private/engine/gfx/vulkan.hpp
	private/engine/gfx/vulkan_device.hpp
	private/engine/gfx/vulkan_swapchain.hpp
	private/engine/gfx/vulkan_shader.hpp
	private/engine/gfx/vulkan_buffer.hpp
	private/engine/gfx/vulkan_pipeline.hpp
	private/engine/gfx/vulkan_texture.hpp
	private/engine/gfx/vulkan_backend_module.cpp
	private/engine/gfx/vulkan_pipeline_layout.cpp
	private/engine/gfx/vulkan_pipeline_layout.hpp
	private/engine/gfx/vulkan_renderpass.hpp
	private/engine/gfx/vulkan_command_pool.hpp
	private/engine/gfx/vulkan_descriptor_manager.cpp
	private/engine/gfx/vulkan_descriptor_manager.hpp
	private/engine/gfx/vulkan_command_list.hpp
	private/engine/gfx/vulkan_texture_view.hpp
	private/engine/gfx/vulkan_sync.hpp
	private/engine/gfx/vulkan_sampler.hpp
	private/engine/gfx/vulkan_device.cpp
	private/engine/gfx/vulkan_memory_allocator.cpp
	private/engine/gfx/vulkan_backend.cpp
	private/engine/gfx/vulkan_buffer.cpp
	private/engine/gfx/vulkan_swapchain.cpp)
target_include_directories(vulkangfx PUBLIC public PRIVATE private)
target_link_libraries(vulkangfx PUBLIC core gfx 
	PRIVATE Vulkan::Vulkan vk-bootstrap::vk-bootstrap unofficial::vulkan-memory-allocator::vulkan-memory-allocator)
