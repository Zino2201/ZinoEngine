#pragma once

#include "engine/hash.hpp"
#include "texture.hpp"
#include <array>
#include <variant>
#include <algorithm>

namespace ze::gfx
{

/** Render pass/framebuffer */
enum class AttachmentLoadOp
{
	Load,
	Clear,
	DontCare
};

enum class AttachmentStoreOp
{
	Store,
	DontCare
};

/** Description of an attachment */
struct AttachmentDescription
{
	Format format;
	SampleCountFlagBits samples;
	AttachmentLoadOp load_op;
	AttachmentStoreOp store_op;
	AttachmentLoadOp stencil_load_op;
	AttachmentStoreOp stencil_store_op;
	TextureLayout initial_layout;
	TextureLayout final_layout;

	AttachmentDescription(const Format& in_format,
		const SampleCountFlagBits& in_samples,
		const AttachmentLoadOp& in_load_op,
		const AttachmentStoreOp& in_store_op,
		const AttachmentLoadOp& in_stencil_load_op,
		const AttachmentStoreOp& in_stencil_store_op,
		const TextureLayout& in_initial_layout,
		const TextureLayout& in_final_layout) : format(in_format),
		samples(in_samples), load_op(in_load_op), store_op(in_store_op),
		stencil_load_op(in_stencil_load_op), stencil_store_op(in_stencil_store_op),
		initial_layout(in_initial_layout), final_layout(in_final_layout) {}

	bool operator==(const AttachmentDescription& in_desc) const
	{
		return format == in_desc.format &&
			samples == in_desc.samples &&
			load_op == in_desc.load_op &&
			store_op == in_desc.store_op &&
			stencil_load_op == in_desc.stencil_load_op &&
			stencil_store_op == in_desc.stencil_store_op &&
			initial_layout == in_desc.initial_layout &&
			final_layout == in_desc.final_layout;
	}
};

/**
 * Reference to an attachment
 */
struct AttachmentReference
{
	static constexpr uint32_t unused_attachment = ~0Ui32;

	/** Index of the attachment (mirror RenderPassCreateInfo::attachments) */
	uint32_t attachment;

	/** Attachment layout during the subpass */
	TextureLayout layout;

	AttachmentReference() : attachment(unused_attachment), layout(TextureLayout::Undefined) {}

	AttachmentReference(const uint32_t in_attachment,
		const TextureLayout in_layout) : attachment(in_attachment), layout(in_layout) {}

	bool operator==(const AttachmentReference& in_ref) const
	{
		return attachment == in_ref.attachment &&
			layout == in_ref.layout;
	}
};

struct SubpassDescription
{
	std::vector<AttachmentReference> input_attachments;
	std::vector<AttachmentReference> color_attachments;
	std::vector<AttachmentReference> resolve_attachments;
	AttachmentReference depth_stencil_attachment;
	std::vector<uint32_t> preserve_attachments;

	SubpassDescription(const std::vector<AttachmentReference>& in_input_attachments = {},
		const std::vector<AttachmentReference>& in_color_attachments = {},
		const std::vector<AttachmentReference>& in_resolve_attachments = {},
		const AttachmentReference& in_depth_stencil_attachment = AttachmentReference(),
		const std::vector<uint32_t>& in_preserve_attachments = {}) :
		input_attachments(in_input_attachments), color_attachments(in_color_attachments),
		resolve_attachments(in_resolve_attachments), depth_stencil_attachment(in_depth_stencil_attachment),
		preserve_attachments(in_preserve_attachments) {}

	bool operator==(const SubpassDescription& in_desc) const
	{
		return input_attachments == in_desc.input_attachments &&
			color_attachments == in_desc.color_attachments &&
			resolve_attachments == in_desc.resolve_attachments &&
			depth_stencil_attachment == in_desc.depth_stencil_attachment &&
			preserve_attachments == in_desc.preserve_attachments;
	}
};

struct RenderPassCreateInfo
{
	std::vector<AttachmentDescription> attachments;
	std::vector<SubpassDescription> subpasses;

	RenderPassCreateInfo(const std::vector<AttachmentDescription>& in_attachments,
		const std::vector<SubpassDescription>& in_subpasses) : attachments(in_attachments),
		subpasses(in_subpasses) {}

	bool operator==(const RenderPassCreateInfo& in_info) const
	{
		return attachments == in_info.attachments &&
			subpasses == in_info.subpasses;
	}
};

union ClearColorValue
{
	std::array<float, 4> float32;

	ClearColorValue(const std::array<float, 4>& in_float) : float32(in_float) {}
};

struct ClearDepthStencilValue
{
	float depth;
	uint32_t stencil;

	ClearDepthStencilValue(const float in_depth,
		const uint32_t in_stencil) : depth(in_depth), stencil(in_stencil) {}
};

union ClearValue
{
	ClearColorValue clear_color;
	ClearDepthStencilValue clear_depth_stencil_value;

	explicit ClearValue(const ClearColorValue& in_clear_color)
		: clear_color(in_clear_color) {}
	explicit ClearValue(const ClearDepthStencilValue& in_clear_depth_stencil_value)
		: clear_depth_stencil_value(in_clear_depth_stencil_value) {}
};

/**
 * A framebuffer (containers of texture views)
 */
struct Framebuffer
{
	std::vector<BackendDeviceResource> attachments;
	uint32_t width;
	uint32_t height;
	uint32_t layers;

	Framebuffer() : width(0), height(0), layers(1) {}

	bool operator==(const Framebuffer& other) const
	{
		const size_t count = std::min<size_t>(attachments.size(), other.attachments.size());
		for(size_t i = 0; i < count; ++i)
		{
			if(attachments[i] != other.attachments[i])
				return false;
		}
		
		return width == other.width &&
			height == other.height;
	}
};
	
}

namespace std
{


template<> struct hash<ze::gfx::AttachmentDescription>
{
	uint64_t operator()(const ze::gfx::AttachmentDescription& in_description) const noexcept
	{
		uint64_t hash = 0;
		ze::hash_combine(hash, in_description.format);
		ze::hash_combine(hash, in_description.samples);
		ze::hash_combine(hash, in_description.load_op);
		ze::hash_combine(hash, in_description.store_op);
		ze::hash_combine(hash, in_description.stencil_load_op);
		ze::hash_combine(hash, in_description.stencil_store_op);
		ze::hash_combine(hash, in_description.initial_layout);
		ze::hash_combine(hash, in_description.final_layout);
		return hash;
	}
};

template<> struct hash<ze::gfx::AttachmentReference>
{
	uint64_t operator()(const ze::gfx::AttachmentReference& in_ref) const noexcept
	{
		uint64_t hash = 0;
		ze::hash_combine(hash, in_ref.attachment);
		ze::hash_combine(hash, in_ref.layout);
		return hash;
	}
};

template<> struct hash<ze::gfx::SubpassDescription>
{
	uint64_t operator()(const ze::gfx::SubpassDescription& in_subpass) const noexcept
	{
		uint64_t hash = 0;
		for(const auto& attachment : in_subpass.input_attachments)
			ze::hash_combine(hash, attachment);

		for(const auto& attachment : in_subpass.color_attachments)
			ze::hash_combine(hash, attachment);

		for(const auto& attachment : in_subpass.resolve_attachments)
			ze::hash_combine(hash, attachment);

		ze::hash_combine(hash, in_subpass.depth_stencil_attachment);

		for(const auto& attachment : in_subpass.resolve_attachments)
			ze::hash_combine(hash, attachment);

		return hash;
	}
};

template<> struct hash<ze::gfx::RenderPassCreateInfo>
{
	uint64_t operator()(const ze::gfx::RenderPassCreateInfo& in_create_info) const noexcept
	{
		uint64_t hash = 0;
		for(const auto& attachment : in_create_info.attachments)
			ze::hash_combine(hash, attachment);

		for(const auto& subpass : in_create_info.subpasses)
			ze::hash_combine(hash, subpass);
		
		return hash;
	}
};

template<> struct hash<ze::gfx::Framebuffer>
{
	uint64_t operator()(const ze::gfx::Framebuffer& in_framebuffer) const noexcept
	{
		uint64_t hash = 0;

		for(const auto& attachment : in_framebuffer.attachments)
			ze::hash_combine(hash, attachment);

		ze::hash_combine(hash, in_framebuffer.width);
		ze::hash_combine(hash, in_framebuffer.height);
		ze::hash_combine(hash, in_framebuffer.layers);

		return hash;
	}
};

}