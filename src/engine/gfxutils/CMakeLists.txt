ze_add_module(gfxutils
	public/engine/gfx/utils/fullscreen_quad.hpp
	public/engine/gfx/utils/scatter_upload_buffer.hpp
	public/engine/gfx/utils/gfx_utils_module.hpp
	public/engine/gfx/utils/mipmap_generation.hpp
	private/engine/gfx/utils/gfx_utils_module.cpp
	private/engine/gfx/utils/fullscreen_quad.cpp
	private/engine/gfx/utils/mipmap_generation.cpp
	private/engine/gfx/utils/scatter_upload_buffer.cpp)
target_include_directories(gfxutils PUBLIC public PRIVATE private ${FIDELITY_FX_SPD_ROOT_DIR})
target_link_libraries(gfxutils PUBLIC core gfx shadersystem)
file(COPY ${FIDELITY_FX_SPD_ROOT_DIR}/ffx-spd/ffx_spd.h DESTINATION ${ZE_ASSETS_DIR}/shaders/utils)
file(COPY ${FIDELITY_FX_SPD_ROOT_DIR}/ffx-spd/ffx_a.h DESTINATION ${ZE_ASSETS_DIR}/shaders/utils)
