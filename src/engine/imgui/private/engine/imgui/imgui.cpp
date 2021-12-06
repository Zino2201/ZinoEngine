#include "engine/imgui/imgui.hpp"
#include <Unknwn.h>
#include <dxcapi.h>
#include <fstream>
#if ZE_PLATFORM(WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <glm/glm.hpp>

#include "engine/shadercompiler/shader_compiler.hpp"

namespace ze::ui
{

using namespace gfx;

struct GlobalData
{
	glm::vec2 translate;
	glm::vec2 scale;
};

BufferHandle global_data_ubo;
void* global_data_ubo_data = nullptr;
UniqueBuffer vertex_buffer;
UniqueBuffer index_buffer;
uint64_t vertex_buffer_size;
uint64_t index_buffer_size;
SamplerHandle sampler;
TextureHandle font_texture;
TextureViewHandle font_texture_view;
ShaderHandle vertex_shader;
ShaderHandle fragment_shader;
PipelineLayoutHandle pipeline_layout;
std::vector<PipelineShaderStage> shader_stages;
PipelineRenderPassState render_pass_state;
PipelineMaterialState material_state;
std::array color_blend_states = { PipelineColorBlendAttachmentState(
	true,
	BlendFactor::SrcAlpha,
	BlendFactor::OneMinusSrcAlpha,
	BlendOp::Add,
	BlendFactor::OneMinusSrcAlpha,
	BlendFactor::Zero,
	BlendOp::Add) };
ImDrawData* draw_data = nullptr;

std::vector<uint8_t> read_text_file(const std::string& in_name)
{
	std::ifstream file(in_name, std::ios::ate | std::ios::binary);

	const size_t file_size = file.tellg();
	
	std::vector<uint8_t> buffer(file_size);
	file.seekg(0);
	file.read((char*) buffer.data(), file_size);
	file.close();

	return buffer;
}

void initialize_imgui()
{
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontDefault();

	{
		auto result = get_device()->create_buffer(BufferInfo::make_ubo(sizeof(GlobalData)));
		ZE_ASSERTF(result.has_value(), "Failed to create ImGui UBO: {}", result.get_error());
		global_data_ubo = result.get_value();

		auto map = get_device()->map_buffer(global_data_ubo);
		global_data_ubo_data = map.get_value();
	}

	{
		auto result = get_device()->create_sampler(SamplerInfo());
		ZE_ASSERTF(result.has_value(), "Failed to create ImGui UBO: {}", result.get_error());
		sampler = result.get_value();
	}

	/** Build font texture */
	{
		uint8_t* data = nullptr;
		int width = 0;
		int height = 0;
		io.Fonts->GetTexDataAsRGBA32(&data,
			&width,
			&height);

		auto tex_result = get_device()->create_texture(TextureInfo::make_immutable_2d(width,
			height,
			Format::R8G8B8A8Unorm,
			1,
			TextureUsageFlags(TextureUsageFlagBits::Sampled),
			{ data, data + (width * height * 4)}).set_debug_name("ImGui Font Texture"));
		ZE_ASSERTF(tex_result.has_value(), "Failed to create ImGui font texture: {}", tex_result.get_error());

		font_texture = tex_result.get_value();

		auto view_result = get_device()->create_texture_view(TextureViewInfo::make_2d(
			font_texture,
			Format::R8G8B8A8Unorm));
		ZE_ASSERTF(view_result.has_value(), "Failed to create ImGui font texture view: {}", view_result.get_error());
		font_texture_view = view_result.get_value();
	}

	/** Create shaders */
	{
		auto vertex_src = read_text_file("assets/shaders/imgui_vs.hlsl");
		auto fragment_src = read_text_file("assets/shaders/imgui_fs.hlsl");

		ShaderCompilerInput vertex_input;
		vertex_input.name = "ImGui VS";
		vertex_input.code = vertex_src;
		vertex_input.entry_point = "main";
		vertex_input.stage = ShaderStageFlagBits::Vertex;
		vertex_input.target_format = ShaderFormat(ShaderModel::SM6_0, ShaderLanguage::VK_SPIRV);

		ShaderCompilerInput fragment_input;
		fragment_input.name = "ImGui FS";
		fragment_input.code = fragment_src;
		fragment_input.entry_point = "main";
		fragment_input.stage = ShaderStageFlagBits::Fragment;
		fragment_input.target_format = ShaderFormat(ShaderModel::SM6_0, ShaderLanguage::VK_SPIRV);

		auto vert_data = gfx::compile_shader(vertex_input);
		auto frag_data = gfx::compile_shader(fragment_input);

		auto vert_result = get_device()->create_shader(ShaderInfo::make({(uint32_t*)vert_data.bytecode.data(),
			(uint32_t*)vert_data.bytecode.data() + vert_data.bytecode.size() }));
		auto frag_result = get_device()->create_shader(ShaderInfo::make({ (uint32_t*)frag_data.bytecode.data(),
			(uint32_t*)frag_data.bytecode.data() + frag_data.bytecode.size() }));

		vertex_shader = vert_result.get_value();
		fragment_shader = frag_result.get_value();

		shader_stages.emplace_back(ShaderStageFlagBits::Vertex, Device::get_backend_shader(vertex_shader), "main");
		shader_stages.emplace_back(ShaderStageFlagBits::Fragment, Device::get_backend_shader(fragment_shader), "main");
	}

	/** Create pipeline layout */
	{
		std::array bindings = {
			DescriptorSetLayoutBinding(0, 
				DescriptorType::UniformBuffer, 
				1, 
				ShaderStageFlags(ShaderStageFlagBits::Vertex)),
			DescriptorSetLayoutBinding(1, 
				DescriptorType::Sampler, 
				1, 
				ShaderStageFlags(ShaderStageFlagBits::Fragment)),
			DescriptorSetLayoutBinding(2, 
				DescriptorType::SampledTexture, 
				1, 
				ShaderStageFlags(ShaderStageFlagBits::Fragment)),
		};
		std::array set_layouts = { DescriptorSetLayoutCreateInfo(bindings) };
		auto result = get_device()->create_pipeline_layout(PipelineLayoutInfo({ set_layouts, {} }));
		ZE_ASSERTF(result.has_value(), "Failed to create ImGui pipeline layout: {}", result.get_error());
		pipeline_layout = result.get_value();
	}

	/** Setup render pass state */
	render_pass_state.color_blend = PipelineColorBlendStateCreateInfo(false,
			LogicOp::NoOp,
			color_blend_states);

	/** Setup material state */
	material_state.vertex_input.input_binding_descriptions =
	{
		VertexInputBindingDescription(0,
			sizeof(ImDrawVert),
			VertexInputRate::Vertex)
	};
	material_state.vertex_input.input_attribute_descriptions =
	{
		VertexInputAttributeDescription(0, 0,
			Format::R32G32Sfloat, 
			offsetof(ImDrawVert, pos)),
		VertexInputAttributeDescription(1, 0,
			Format::R32G32Sfloat, 
			offsetof(ImDrawVert, uv)),
		VertexInputAttributeDescription(2, 0,
			Format::R8G8B8A8Unorm, 
			offsetof(ImDrawVert, col)),
	};
	material_state.stages = shader_stages;
}

void update_draw_data()
{
	if(!draw_data)
		draw_data = ImGui::GetDrawData();

	if(draw_data->TotalVtxCount == 0)
		return;

	uint64_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
	uint64_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

	if(!vertex_buffer || vertex_buffer_size < vertex_size)
	{
		vertex_buffer = UniqueBuffer(get_device()->create_buffer(
			BufferInfo::make_vertex_buffer_cpu_visible(vertex_size).set_debug_name("ImGui Vertex Buffer")).get_value());
		vertex_buffer_size = vertex_size;
	}

	if(!index_buffer || index_buffer_size < index_size)
	{
		index_buffer = UniqueBuffer(get_device()->create_buffer(
			BufferInfo::make_index_buffer_cpu_visible(index_size).set_debug_name("ImGui Index Buffer")).get_value());
		index_buffer_size = index_size;
	}

	/** Write to buffers */
	auto vertex_map = get_device()->map_buffer(vertex_buffer.get());
	auto index_map = get_device()->map_buffer(index_buffer.get());

	auto vertex_data = static_cast<ImDrawVert*>(vertex_map.get_value());
	auto index_data = static_cast<ImDrawIdx*>(index_map.get_value());

	for(int i = 0; i < draw_data->CmdListsCount; i++)
	{
		ImDrawList* draw_list = draw_data->CmdLists[i];
		memcpy(vertex_data, draw_list->VtxBuffer.Data, draw_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(index_data, draw_list->IdxBuffer.Data, draw_list->IdxBuffer.Size * sizeof(ImDrawIdx));

		vertex_data += draw_list->VtxBuffer.Size;
		index_data += draw_list->IdxBuffer.Size;
	}

	get_device()->unmap_buffer(index_buffer.get());
	get_device()->unmap_buffer(vertex_buffer.get());

	/** Update global data */
	GlobalData global_data;
	global_data.scale = { 2.f / draw_data->DisplaySize.x, 2.f / draw_data->DisplaySize.y };
	global_data.translate = { -1.f - draw_data->DisplayPos.x * global_data.scale.x, -1.f - draw_data->DisplayPos.y * global_data.scale.y };
	memcpy(global_data_ubo_data, &global_data, sizeof(global_data));
}

void draw_imgui(gfx::CommandListHandle list)
{
	update_draw_data();

	ZE_CHECK(draw_data);

	if(draw_data->CmdListsCount > 0)
	{
		get_device()->cmd_bind_pipeline_layout(list, pipeline_layout);
		get_device()->cmd_set_render_pass_state(list, render_pass_state);
		get_device()->cmd_set_material_state(list, material_state);

		get_device()->cmd_bind_ubo(list, 0, 0, global_data_ubo);
		get_device()->cmd_bind_sampler(list, 0, 1, sampler);

		get_device()->cmd_bind_vertex_buffer(list, vertex_buffer.get(), 0);
		get_device()->cmd_bind_index_buffer(list, index_buffer.get(), 0, IndexType::Uint16);

		uint32_t vertex_offset = 0;
		uint32_t index_offset = 0;

		for(int32_t i = 0; i < draw_data->CmdListsCount; ++i)
		{
			ImDrawList* draw_list = draw_data->CmdLists[i];
			for(int32_t j = 0; j < draw_list->CmdBuffer.Size; ++j)
			{
				const ImDrawCmd& cmd = draw_list->CmdBuffer[j];

				const ImVec2 clip_off = draw_data->DisplayPos;
				const ImVec2 clip_scale = draw_data->FramebufferScale;

				ImVec4 clip_rect;
				clip_rect.x = std::max<float>((cmd.ClipRect.x - clip_off.x) * clip_scale.x, 0);
				clip_rect.y = std::max<float>((cmd.ClipRect.y - clip_off.y) * clip_scale.y, 0);
				clip_rect.z = (cmd.ClipRect.z - clip_off.x) * clip_scale.x;
				clip_rect.w = (cmd.ClipRect.w - clip_off.y) * clip_scale.y;

				get_device()->cmd_set_scissor(list, Rect2D(
					static_cast<int32_t>(clip_rect.x), 
					static_cast<int32_t>(clip_rect.y),
					static_cast<uint32_t>(clip_rect.z - clip_rect.x),
					static_cast<uint32_t>(clip_rect.w - clip_rect.y)));

				// TODO: Implement texture
				ZE_CHECK(!cmd.TextureId);
				if(!cmd.TextureId)
					get_device()->cmd_bind_texture_view(list, 0, 2, font_texture_view);

				get_device()->cmd_draw_indexed(list,
					cmd.ElemCount,
					1,
					cmd.IdxOffset + index_offset,
					static_cast<int32_t>(cmd.VtxOffset) + vertex_offset,
					0);
			}

			vertex_offset += draw_list->VtxBuffer.Size;
			index_offset += draw_list->IdxBuffer.Size;
		}
	}
}

void destroy_imgui()
{
	get_device()->destroy_pipeline_layout(pipeline_layout);

	get_device()->destroy_shader(vertex_shader);
	get_device()->destroy_shader(fragment_shader);

	get_device()->destroy_texture(font_texture);
	get_device()->destroy_texture_view(font_texture_view);

	get_device()->destroy_sampler(sampler);

	get_device()->unmap_buffer(global_data_ubo);
	get_device()->destroy_buffer(global_data_ubo);

	vertex_buffer.reset();
	index_buffer.reset();
}

}
