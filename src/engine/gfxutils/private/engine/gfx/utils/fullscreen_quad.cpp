#include "engine/gfx/device.hpp"
#include "engine/gfx/utils/gfx_utils_module.hpp"
#include "glm/vec2.hpp"

namespace ze::gfx
{

void draw_fullscreen_quad(CommandListHandle in_list)
{
	struct Vertex
	{
		glm::vec2 position;
		glm::vec2 texcoord;
	};

	PipelineVertexInputStateCreateInfo vertex_input;
	vertex_input.input_binding_descriptions =
	{
		VertexInputBindingDescription(0,
			sizeof(Vertex),
			VertexInputRate::Vertex)
	};
	vertex_input.input_attribute_descriptions =
	{
		VertexInputAttributeDescription(0, 0,
			Format::R32G32Sfloat,
			offsetof(Vertex, position)),
		VertexInputAttributeDescription(1, 0,
			Format::R32G32Sfloat,
			offsetof(Vertex, texcoord)),
	};

	get_device()->cmd_set_vertex_input_state(in_list, vertex_input);
	get_device()->cmd_bind_vertex_buffer(in_list, GfxUtilsModule::get_quad_vertex_buffer(), 0);
	get_device()->cmd_bind_index_buffer(in_list, GfxUtilsModule::get_quad_index_buffer(), 0, IndexType::Uint16);
	get_device()->cmd_draw_indexed(in_list, 6, 1, 0, 0, 0);
}

}
