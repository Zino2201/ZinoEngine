#pragma once

#include "engine/gfx/Device.hpp"

namespace ze::renderer
{

/**
 * Structure containing everything required to draw a mesh batch for a specific render pass
 */
struct Drawcall
{
	std::array<gfx::Descriptor, gfx::max_descriptor_sets> descriptors;
	uint64_t commands_offset;
};

/**
 * The actual drawcall as stored in the pass drawcalls buffer
 */
struct GPUDrawcall
{
	union
	{
		struct
		{
			uint32_t index_count;	
			uint32_t instance_count;	
			uint32_t first_index;
			int32_t vertex_offset;
			uint32_t first_instance;
		};
#if 0
		struct /** No index buffer bound */
		{
			uint32_t vertex_count;	
			uint32_t instance_count;	
			uint32_t first_vertex;	
			uint32_t first_instance;	
		};
#endif
	};
};

}