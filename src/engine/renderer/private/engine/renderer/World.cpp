#include "engine/renderer/World.hpp"

namespace ze::renderer
{

#if 0

size_t World::add_node(RenderNode&& in_node)
{
	size_t node = nodes.add(std::move(in_node));
	size_t gpu_node = node_gpu_infos.add(RenderNodeGPUInfo());
	ZE_CHECK(node == gpu_node);

	/** Setup initial data, world matrix will be set by update_nodes() */
	nodes[node].node_index = node;

	dirty_nodes.emplace_back(node);

	for(const auto& mesh_data : nodes[node].mesh_data)
	{
		auto& batch = merge_or_add_to_compatible_batch(mesh_data);
		batch.instances.emplace_back(node, mesh_data.index_count, mesh_data.first_index, mesh_data.vertex_offset);
	}

	return node;
}

void World::remove_node(size_t in_handle)
{
	nodes.remove(in_handle);
	node_gpu_infos.remove(in_handle);
}

void World::update_nodes()
{
	if(!dirty_nodes.empty())
	{
		for(const auto& dirty_node : dirty_nodes)
			update_gpu_node(nodes[dirty_node]);

		dirty_nodes.clear();
		dirty_nodes.clear();
	}
}

void World::update_batches()
{
	if(!dirty_batches.empty())
	{
		for(const auto& batch : dirty_batches)
		{
			update_batch(batches[batch]);		
		}

		dirty_batches.clear();
	}
}

void World::update_gpu_node(const RenderNode& in_node)
{
	node_gpu_infos[in_node.node_index].world = glm::scale(glm::mat4(1.f), in_node.scale) *
		glm::mat4_cast(in_node.rotation) *
		glm::translate(glm::mat4(1.f), in_node.position);
}

void World::update_batch(MeshBatch& in_batch)
{
	using namespace gfx;

	in_batch.nodes_vertex_buffer.reset();

	auto result = get_device()->create_buffer(BufferInfo(BufferCreateInfo(
		in_batch.instances.size() * sizeof(uint32_t),
		MemoryUsage::GpuOnly,
		BufferUsageFlags(BufferUsageFlagBits::VertexBuffer)),
		{ reinterpret_cast<uint8_t*>(in_batch.instances.data()),
			reinterpret_cast<uint8_t*>(in_batch.instances.data() + in_batch.instances.size())}));
	if(result)
	{
		in_batch.nodes_vertex_buffer = UniqueBuffer(result.get_value());	
	}
}

MeshBatch& World::merge_or_add_to_compatible_batch(const RenderNode::MeshData& in_mesh_data)
{
	const uint64_t hash = std::hash()(in_mesh_data);

	size_t idx = 0;
	bool found_compatible_batch = false;
	for(auto& batch : batches)
	{
		if(hash == batch.key)
		{
			batch.key = hash;
			found_compatible_batch = true;
			break;
		}

		idx++;
	}

	if(!found_compatible_batch)
	{
		idx = batches.add(MeshBatch(hash, 
			in_mesh_data.material, 
			in_mesh_data.vertex_buffer, 
			in_mesh_data.index_buffer));
	}

	dirty_batches.emplace_back(idx);

	return batches[idx];
}

#endif

}