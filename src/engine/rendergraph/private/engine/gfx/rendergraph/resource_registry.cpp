#include "engine/gfx/rendergraph/resource_registry.hpp"
#include "engine/gfx/device.hpp"

namespace ze::gfx::rendergraph
{

std::pair<TextureHandle, TextureViewHandle> PhysicalResourceRegistry::get_texture_handles(std::string in_name, TextureInfo in_info)
{
	auto it = textures.find({ in_name, in_info });
	if (it != textures.end())
		return { it->second.texture.get(), it->second.view.get() };

	auto texture_result = get_device()->create_texture(in_info);
	ZE_CHECKF(texture_result.has_value(), "Failed to create render graph texture: {}", std::to_string(texture_result.get_error()));

	TextureAspectFlags aspect_flags = TextureAspectFlagBits::Color;
	if (in_info.info.format == Format::D32Sfloat
		|| in_info.info.format == Format::D24UnormS8Uint || in_info.info.format == Format::D32SfloatS8Uint)
		aspect_flags = TextureAspectFlagBits::Depth;

	auto texture_view_result = get_device()->create_texture_view(TextureViewInfo
		{
			TextureViewType::Tex2D,
			texture_result.get_value(),
			in_info.info.format,
			TextureSubresourceRange(aspect_flags, 0, 1, 0, 1)
		}.set_debug_name(fmt::format("{} default view", in_name)));
	ZE_CHECKF(texture_view_result.has_value(), "Failed to create render graph texture view: {}", std::to_string(texture_view_result.get_error()));

	textures.insert({ TextureKey { in_name, in_info },
		TextureEntry
		{
			UniqueTexture(texture_result.get_value()),
			UniqueTextureView(texture_view_result.get_value())
		} });

	return { texture_result.get_value(), texture_view_result.get_value() };
}

}
