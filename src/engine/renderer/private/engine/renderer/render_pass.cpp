#include "engine/renderer/render_pass.hpp"
#include "engine/renderer/world.hpp"

#if 0
namespace ze::renderer
{


RenderPass::RenderPass(World& in_world) : world(in_world) {}

void RenderPass::rebuild()
{
	drawcalls.clear();
	drawcalls.reserve(world.get_batches().get_size());

	std::vector<GPUDrawcall> temp_gpu_drawcalls;
	temp_gpu_drawcalls.reserve(world.get_batches().get_size());

	/** TODO: Drawcall sorting */
	for(const auto& batch : world.get_batches())
	{
		/** TODO: Flags mask to filter out batches */

		/** Build the GPU drawcalls for this batch */
		for(const auto& instance : batch.instances)
		{
			temp_gpu_drawcalls.emplace_back(instance.index_count,
				1,
				instance.first_index,
				instance.vertex_offset,
				0);
		}
	}
}


}

#endif