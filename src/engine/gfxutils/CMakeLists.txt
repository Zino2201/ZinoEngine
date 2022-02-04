ze_add_module(gfxutils
	public/engine/gfx/utils/scatter_upload_buffer.hpp
	public/engine/gfx/utils/gfx_utils_module.hpp
	private/engine/gfx/utils/gfx_utils_module.cpp
	private/engine/gfx/utils/scatter_upload_buffer.cpp)
target_include_directories(gfxutils PUBLIC public PRIVATE private)
target_link_libraries(gfxutils PUBLIC core gfx shadersystem)