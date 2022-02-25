#pragma once

#include <unordered_map>
#include "engine/gfx/device.hpp"

namespace ze::gfx::rendergraph
{

/**
 * The physical resource registry allocate and holds all actual physical resources used by a render graph
 */
class PhysicalResourceRegistry
{
public:
	struct TextureKey
	{
		std::string name;
		TextureInfo create_info;

		bool operator==(const TextureKey& in_other) const
		{
			return name == in_other.name &&
				create_info.info == in_other.create_info.info;
		}
	};

	struct TextureKeyHash
	{
		uint64_t operator()(const TextureKey& in_key) const noexcept
		{
			uint64_t hash = 0;

			hash_combine(hash, in_key.name);
			hash_combine(hash, in_key.create_info.info.mem_usage);
			hash_combine(hash, in_key.create_info.info.width);
			hash_combine(hash, in_key.create_info.info.height);
			hash_combine(hash, in_key.create_info.info.depth);
			hash_combine(hash, in_key.create_info.info.format);
			hash_combine(hash, in_key.create_info.info.usage_flags);

			return hash;
		}
	};

	struct TextureEntry
	{
		UniqueTexture texture;
		UniqueTextureView view;
	};

	std::pair<TextureHandle, TextureViewHandle> get_texture_handles(std::string in_name, TextureInfo in_info);
private:
	robin_hood::unordered_map<TextureKey, TextureEntry, TextureKeyHash> textures;
};

}