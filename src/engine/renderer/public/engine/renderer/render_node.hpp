#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "engine/gfx/device.hpp"
#include "render_pass.hpp"

namespace ze { class Material; }

namespace ze::renderer
{

/**
 * A batch of node's mesh with the same material/buffers
 */
struct MeshBatch
{
	struct Instance
	{
		size_t node_idx;
		uint32_t index_count;
		uint32_t first_index;
		int32_t vertex_offset;
	};

	uint64_t key;

	Material* material;
	gfx::BufferHandle vertex_buffer;

	/** Vertex buffer used to index to the global node SSBO */
	gfx::UniqueBuffer nodes_vertex_buffer;
	gfx::BufferHandle index_buffer;

	std::vector<Instance> instances;

	MeshBatch(uint64_t in_key,
		Material* in_material,
		gfx::BufferHandle in_vertex_buffer,
		gfx::BufferHandle in_index_buffer) : key(in_key), material(in_material),
		vertex_buffer(in_vertex_buffer),
		index_buffer(in_index_buffer) {}
};

/**
 * A simple renderable object in a graph
 */
struct RenderNode
{
	/** Mesh informations about a render node, this data will then be used to create/merge to mesh batches */
	struct MeshData
	{
		Material& material;
		gfx::BufferHandle vertex_buffer;
		gfx::BufferHandle index_buffer;
		uint32_t index_count;
		uint32_t instance_count;
		uint32_t first_index;
		int32_t vertex_offset;
		uint32_t first_instance;
	};

	glm::dvec3 position;
	glm::quat rotation;
	glm::vec3 scale;

	std::vector<MeshData> mesh_data;

	/** Data set by the world */
	size_t node_index;

	/** Associed drawcall per render pass */
	robin_hood::unordered_map<RenderPass, size_t> drawcalls;
};

}

namespace std
{

template<>
struct hash<ze::renderer::RenderNode::MeshData>
{
	uint64_t operator()(const ze::renderer::RenderNode::MeshData&) const noexcept
	{
		uint64_t hash = 0;
		//ze::hash_combine(hash, in_mesh_data.vertex_buffer);
		//ze::hash_combine(hash, in_mesh_data.index_buffer);
		//ze::hash_combine(hash, in_mesh_data.material);
		return hash;
	}
};

}