#pragma once

#include "texture.hpp"

namespace ze::gfx
{

enum class QueueType
{
	Gfx,
	Compute,
	Transfer,
	Present
};

struct CommandPoolCreateInfo
{
	QueueType queue_type;

	CommandPoolCreateInfo(QueueType in_queue_type) : queue_type(in_queue_type) {}
};

struct BufferCopyRegion
{
	uint64_t src_offset;
	uint64_t dst_offset;
	uint64_t size;

	BufferCopyRegion(const uint64_t in_src_offset,
		const uint64_t in_dst_offset,
		const uint64_t in_size) : src_offset(in_src_offset),
		dst_offset(in_dst_offset),
		size(in_size) {}
};

struct Offset3D
{
	int32_t x;	
	int32_t y;	
	int32_t z;

	Offset3D(const int32_t in_x = 0,
		const int32_t in_y = 0,
		const int32_t in_z = 0) : x(in_x), y(in_y), z(in_z) {}
};

struct Extent3D
{
	uint32_t width;
	uint32_t height;
	uint32_t depth;

	Extent3D(const int32_t in_width = 0,
		const int32_t in_height = 0,
		const int32_t in_depth = 0) : width(in_width), height(in_height), depth(in_depth) {}
};

struct BufferTextureCopyRegion
{
	uint64_t buffer_offset;
	TextureSubresourceLayers texture_subresource;
	Offset3D texture_offset;
	Extent3D texture_extent;

	BufferTextureCopyRegion(const uint64_t in_buffer_offset,
		const TextureSubresourceLayers in_layers,
		const Offset3D in_offset,
		const Extent3D in_extent) : buffer_offset(in_buffer_offset),
		texture_subresource(in_layers),
		texture_offset(in_offset),
		texture_extent(in_extent) {}
};

enum class IndexType
{
	Uint16,
	Uint32
};

}