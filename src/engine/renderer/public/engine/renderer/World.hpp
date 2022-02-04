#pragma once

#include "engine/containers/SparseArray.hpp"
#include "RenderNode.hpp"
#include "engine/gfx/UniformBuffer.hpp"
#include "engine/gfx/SparseGPUArray.hpp"
#include <boost/dynamic_bitset.hpp>
#include "RenderPass.hpp"
#include "Drawcall.hpp"
#include "engine/containers/sparse_array.hpp"

namespace ze::renderer
{

/**
 * GPU-side informatins about a render node
 */
struct RenderNodeGPUInfo
{
	glm::mat4 world;
};

class WorldView
{
	
};

class World
{
public:
	/**
	 * Add a render node to the world
	 * \return Handle to the node
	 */
	size_t add_node(RenderNode&& in_node);
	void remove_node(size_t in_handle);

	/** Update dirty nodes and upload data to GPU */
	void update_nodes();

	/** Update dirty batches */
	void update_batches();

	const auto& get_batches() const { return batches; }
private:
	void update_gpu_node(const RenderNode& in_node);
	void update_batch(MeshBatch& in_batch);

	/**
	 * Merge or add mesh data to a compatible batch
	 */
	MeshBatch& merge_or_add_to_compatible_batch(const RenderNode::MeshData& in_mesh_data);
private:
	SparseArray<RenderNode> nodes;
	SparseArray<MeshBatch> batches;
	std::vector<size_t> dirty_nodes;
	std::vector<size_t> dirty_batches;
	gfx::SparseGPUArray<RenderNodeGPUInfo> node_gpu_infos;
	robin_hood::unordered_map<RenderPass, std::vector<Drawcall>> drawcalls;
};

}
