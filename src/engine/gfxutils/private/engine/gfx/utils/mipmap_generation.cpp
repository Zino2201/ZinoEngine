#include "engine/gfx/utils/mipmap_generation.hpp"
#define A_CPU
#include "ffx-spd/ffx_a.h"
#include "ffx-spd/ffx_spd.h"
#include "engine/shadersystem/shader_manager.hpp"

namespace ze::gfx
{
	
void generate_mipmaps(shadersystem::ShaderManager& in_shader_manager, 
	TextureHandle in_texture,
	Format in_format,
	uint32_t in_width, 
	uint32_t in_height, 
	uint32_t in_layer_count)
{
	ZE_CHECK(in_layer_count == 1);

	if (const auto instance = in_shader_manager.get_shader("SPDMipmapsGen")->instantiate({}))
	{
		varAU2(dispatch_thread_group_count);
		varAU2(work_group_offset);
		varAU2(work_group_and_mip_count);
		varAU4(rect_info) = initAU4(0, 0, in_width, in_height);
		SpdSetup(dispatch_thread_group_count, work_group_offset, work_group_and_mip_count, rect_info);

		constexpr uint32_t initial_value = 0;
		UniqueBuffer global_atomic_counter = UniqueBuffer(get_device()->create_buffer(
			BufferInfo::make_ssbo(sizeof(uint32_t))// { reinterpret_cast<const uint8_t*>(&initial_value), sizeof(initial_value)}
				.set_debug_name("Mipmap Gen Global Atomic Counter")).get_value());

		std::vector<UniqueTextureView> mips;
		for (size_t i = 0; i < work_group_and_mip_count[1]; ++i)
		{
			mips.emplace_back(UniqueTextureView(get_device()->create_texture_view(
				TextureViewInfo::make_2d(in_texture, in_format, TextureSubresourceRange(
					TextureAspectFlagBits::Color, i, 1, 0, 1))).get_value()));
		}

		UniqueSampler sampler = UniqueSampler(get_device()->create_sampler(
			SamplerInfo()).get_value());

		const auto list = get_device()->allocate_cmd_list(QueueType::Gfx); // todo: async compute
		get_device()->cmd_begin_region(list, "Mipmap Generation", { 1, 0, 0, 1});

		const uint32_t dispatch_x = dispatch_thread_group_count[0];
		const uint32_t dispatch_y = dispatch_thread_group_count[1];
		const uint32_t dispatch_z = in_layer_count;

		instance->set_parameter("work_group_count", work_group_and_mip_count[0]);
		instance->set_parameter("mip_count", work_group_and_mip_count[1]);
		instance->set_parameter("work_group_offset", { work_group_offset[0], work_group_offset[1] });
		instance->set_parameter("global_atomic_counter", global_atomic_counter.get());
		instance->set_parameter("src_texture", mips[0].get());
		instance->set_parameter_uav("mips", std::span { mips.begin() + 1, mips.end() });
		instance->set_parameter("sampler", sampler.get());
		instance->set_parameter("inv_input_size", 
			glm::vec2 
			{
				1.f / static_cast<float>(in_width),
				1.f / static_cast<float>(in_height),
			});

		instance->bind(list);
		get_device()->cmd_dispatch(list, dispatch_x, dispatch_y, dispatch_z);
		get_device()->cmd_end_region(list);
		get_device()->submit(list);
	}
}

}