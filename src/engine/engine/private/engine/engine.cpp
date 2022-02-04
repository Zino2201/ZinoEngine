#include "engine/engine.hpp"
#include "engine/application/application_module.hpp"
#include "engine/application/platform_application.hpp"
#include "engine/application/platform_window.hpp"
#include "engine/gfx/backend.hpp"
#include "engine/gfx/vulkan_backend_module.hpp"
#include "engine/imgui/imgui.hpp"
#include "engine/module/module_manager.hpp"
#include "engine/shadersystem/shader_manager.hpp"
#include "engine/filesystem/filesystem.hpp"
#include "engine/filesystem/filesystem_module.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define TINYOBJLOADER_IMPLEMENTATION
#include "engine/tinyobjloader.h"

namespace ze
{

Engine::Engine() : running(true)
{
#if ZE_BUILD(DEBUG)
	auto result = get_module<gfx::VulkanBackendModule>("vulkangfx")
		->create_vulkan_backend(gfx::BackendFlags(gfx::BackendFlagBits::DebugLayers));
#else
	auto result = get_module<gfx::VulkanBackendModule>("vulkangfx")->create_vulkan_backend(gfx::BackendFlags());
#endif
	if (!result)
		logger::fatal("Failed to create backend: {}", result.get_error());

	backend = std::move(result.get_value());

	auto backend_device = backend->create_device(gfx::ShaderModel::SM6_0);
	if (!backend_device)
		logger::fatal("Failed to create device: {}", backend_device.get_error());

	device = std::make_unique<gfx::Device>(*result.get_value().get(), std::move(backend_device.get_value()));
	shader_manager = std::make_unique<shadersystem::ShaderManager>(*device);
	shader_manager->add_shader_directory("assets/shaders");
	shader_manager->set_shader_format(gfx::ShaderFormat(gfx::ShaderModel::SM6_0, backend->get_shader_language()));
}

Engine::~Engine() = default;

void Engine::run()
{
	//auto cmd_list_test = device->allocate_cmd_list(gfx::QueueType::Compute);
	//gfx::ScatterUploadBuffer<int> coucou;
	//coucou.emplace(0, 20);
	//coucou.emplace(50, 2201);

	const auto platform = get_module<platform::ApplicationModule>("Application");
	platform->get_application().set_message_handler(this);
	main_window = platform->get_application().create_window(
		"ZinoEngine",
		1280,
		720,
		0,
		0,
		platform::WindowFlags(platform::WindowFlagBits::Centered | platform::WindowFlagBits::Maximized));

	ImGui::SetCurrentContext(ImGui::CreateContext());
	imgui::initialize(*shader_manager);

	/** Default ZE editor style */
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding = 0.f;
		style.FrameRounding = 3.f;
		style.TabRounding = 0.f;
		style.ScrollbarRounding = 0.f;
		style.WindowMenuButtonPosition = ImGuiDir_Right;
		style.TabMinWidthForCloseButton = 0.f;
		style.ItemSpacing = ImVec2(8, 4);
		style.WindowBorderSize = 0.f;
		style.FrameBorderSize = 0.f;
		style.PopupBorderSize = 1.f;
		style.TabBorderSize = 1.f;

		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(0.79f, 0.79f, 0.79f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.22f, 0.22f, 0.22f, 0.94f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.09f, 0.09f, 0.09f, 0.50f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.16f, 0.54f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.30f, 0.30f, 0.40f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.33f, 0.33f, 0.33f, 0.67f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.71f, 0.71f, 0.71f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.29f, 0.29f, 0.29f, 0.40f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.11f, 0.11f, 0.11f, 0.31f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.13f, 0.13f, 0.13f, 0.80f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.15f, 0.14f, 0.16f, 0.50f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.15f, 0.14f, 0.16f, 1.00f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.14f, 0.13f, 0.16f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.11f, 0.11f, 0.11f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.95f);
		colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.16f, 0.86f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.29f, 0.29f, 0.29f, 0.80f);
		colors[ImGuiCol_TabActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.24f, 0.24f, 0.24f, 0.97f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
		colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	}

	create_swapchain(gfx::UniqueSwapchain());

	imgui::initialize_main_viewport(*main_window, swapchain.get());


	struct vertexdata
	{
		glm::vec3 position;
		glm::vec3 normal;
	};

	struct ubodata
	{
		glm::mat4 world;
		glm::mat4 view;
		glm::mat4 proj;
	};
	gfx::UniformBuffer<ubodata> wvp_ubo;
	gfx::UniqueBuffer vbo;

	std::vector<vertexdata> positions;

	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		tinyobj::LoadObj(&attrib, &shapes, nullptr, nullptr, nullptr, "assets/cube.obj");


		for(const auto& shape : shapes)
		{
			for(const auto& index : shape.mesh.indices)
			{
				auto& pos = positions.emplace_back();
				pos.position.x = attrib.vertices[3 * index.vertex_index + 0];
				pos.position.y = attrib.vertices[3 * index.vertex_index + 1];
				pos.position.z = attrib.vertices[3 * index.vertex_index + 2];

				pos.normal.x = attrib.normals[3 * index.normal_index + 0];
				pos.normal.y = attrib.normals[3 * index.normal_index + 1];
				pos.normal.z = attrib.normals[3 * index.normal_index + 2];
			}
		}

		vbo = gfx::UniqueBuffer(device->create_buffer(gfx::BufferInfo(
			gfx::BufferCreateInfo(positions.size() * sizeof(vertexdata),
				gfx::MemoryUsage::GpuOnly,
				gfx::BufferUsageFlags(gfx::BufferUsageFlagBits::VertexBuffer)),
			{ (uint8_t*) positions.data(), positions.size() * sizeof(vertexdata) })).get_value());
	}

	std::unique_ptr<shadersystem::ShaderInstance> shader_instance;
	gfx::PipelineMaterialState material_state;
	gfx::PipelineRenderPassState render_pass_state;
	std::vector<gfx::PipelineShaderStage> shader_stages;

	/** Get shaders */
	{
		shader_instance = shader_manager->get_shader("Cube")->instantiate({});
		const auto& shader_map = shader_instance->get_permutation().get_shader_map();
		ZE_ASSERTF(shader_map.size() == 2, "Failed to create ImGui shaders, see log. Exiting.");
		auto vertex_shader = shader_map.find(gfx::ShaderStageFlagBits::Vertex);
		auto fragment_shader = shader_map.find(gfx::ShaderStageFlagBits::Fragment);
		shader_stages.emplace_back(gfx::ShaderStageFlagBits::Vertex, gfx::Device::get_backend_shader(*vertex_shader->second), "main");
		shader_stages.emplace_back(gfx::ShaderStageFlagBits::Fragment, gfx::Device::get_backend_shader(*fragment_shader->second), "main");
	}

	/** Setup material state */
	material_state.vertex_input.input_binding_descriptions =
	{
		gfx::VertexInputBindingDescription(0,
			sizeof(vertexdata),
			gfx::VertexInputRate::Vertex)
	};
	material_state.vertex_input.input_attribute_descriptions =
	{
		gfx::VertexInputAttributeDescription(0, 0,
			gfx::Format::R32G32B32Sfloat,
			0),
		gfx::VertexInputAttributeDescription(1, 0,
			gfx::Format::R32G32B32Sfloat,
			offsetof(vertexdata, normal)),
	};
	material_state.stages = shader_stages;
	std::array test = { gfx::PipelineColorBlendAttachmentState() };
	render_pass_state.color_blend.attachments = test;

	material_state.rasterizer.cull_mode = gfx::CullMode::Back;

	auto previous = std::chrono::high_resolution_clock::now();

	float i = 0;
	while(running)
	{
		i += 0.05f;
		platform->get_application().pump_messages();

		device->new_frame();

		auto current = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> delta_time_ms = current - previous;
		previous = current;

		glm::vec3 cam_pos = { 2, 0, 0 };
		glm::vec3 fwd = { 1, 0, 0 };

		glm::mat4 view = glm::lookAtLH(cam_pos,
			cam_pos + fwd,
			glm::vec3(0, 0, 1.f));

		glm::mat4 model = glm::scale(glm::mat4(1.f), glm::vec3(0.5f)) *
			glm::rotate(glm::mat4(1.f), i, glm::vec3(0, 0, 1)) *
			glm::rotate(glm::mat4(1.f), i, glm::vec3(0, 1, 0));;
		glm::mat4 proj = glm::perspective(glm::radians(90.f),
			(float)main_window->get_width() / main_window->get_height(),
			0.01f,
			100000.f);
		proj[1][1] *= -1;

		ubodata u;
		u.world = model;
		u.view = view;
		u.proj = proj;
		wvp_ubo.update(u);

		const float delta_time = static_cast<float>(delta_time_ms.count()) * 0.001f;

		imgui::new_frame(delta_time, *main_window);

		ImGui::NewFrame();
		ImGui::Text("%.0f FPS", 1.f / ImGui::GetIO().DeltaTime, ImGui::GetIO().DeltaTime);
		ImGui::Text("%.2f ms", ImGui::GetIO().DeltaTime * 1000);
		ImGui::Text("Mouse Pos: %f %f", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
		ImGui::ShowDemoWindow();
		ImGui::Render();

		ImGui::UpdatePlatformWindows();
		imgui::draw_viewports();

		if(false) {
			using namespace gfx;
			const auto list = device->allocate_cmd_list(gfx::QueueType::Gfx);
			std::array clear_values = { ClearValue(ClearColorValue({0, 0, 0, 1})),
				ClearValue(ClearDepthStencilValue(1.f, 0)) };
			std::array color_attachments = { get_device()->get_swapchain_backbuffer_view(swapchain.get()) };
			RenderPassInfo render_pass_info;
			render_pass_info.render_area = Rect2D(0, 0,
				static_cast<uint32_t>(main_window->get_width()), static_cast<uint32_t>(main_window->get_height()));
			render_pass_info.color_attachments = color_attachments;
			render_pass_info.clear_attachment_flags = 1 << 0;
			render_pass_info.store_attachment_flags = 1 << 0;
			render_pass_info.clear_values = clear_values;

			std::array color_attachments_refs = { 0Ui32 };
			std::array subpasses = { RenderPassInfo::Subpass(color_attachments_refs,
				{},
				{},
				RenderPassInfo::DepthStencilMode::ReadOnly) };
			render_pass_info.subpasses = subpasses;

			device->cmd_begin_render_pass(list, render_pass_info);
			device->cmd_bind_vertex_buffer(list, vbo.get(), 0);
			device->cmd_set_render_pass_state(list, render_pass_state);
			device->cmd_set_material_state(list, material_state);
			device->cmd_bind_ubo(list, 0, 0, wvp_ubo.get_handle());
			device->cmd_bind_pipeline_layout(list, shader_instance->get_permutation().get_pipeline_layout());
			device->cmd_draw(list, positions.size(), 1, 0, 0);
			device->cmd_end_render_pass(list);

			device->submit(list);
		}


		device->end_frame();

		imgui::present_viewports();

		/** FPS Limiter */
		{
			using namespace std::chrono_literals;
			constexpr std::chrono::duration<double, std::milli> min_ms(1000.0 / 60.0);
			const auto target_sleep_time = current + min_ms;
			std::this_thread::sleep_until(target_sleep_time - 2ms);
			while (std::chrono::high_resolution_clock::now() < target_sleep_time) {}
		}
	}

	device->wait_idle();

	ImGui::DestroyContext();
	imgui::destroy();
}

void Engine::create_swapchain(const gfx::UniqueSwapchain& old_swapchain)
{
	device->wait_idle();
	swapchain = gfx::UniqueSwapchain(device->create_swapchain(gfx::SwapChainCreateInfo(
		main_window->get_handle(),
		main_window->get_width(),
		main_window->get_height(),
		old_swapchain.is_valid() ? device->get_swapchain_backend_handle(old_swapchain.get()) : gfx::null_backend_resource)).get_value());

	if(old_swapchain.is_valid())
		imgui::update_main_viewport(*main_window, swapchain.get());
}

void Engine::on_resized_window(platform::Window& in_window, uint32_t in_width, uint32_t in_height)
{
	if(&in_window == main_window.get())
		create_swapchain(gfx::UniqueSwapchain(swapchain.free()));

	imgui::on_resized_window(in_window, in_width, in_height);
}

void Engine::on_closing_window(platform::Window&)
{
	running = false;
}

void Engine::on_mouse_down(platform::Window& in_window, platform::MouseButton in_button, const glm::ivec2& in_mouse_pos)
{
	imgui::on_mouse_down(in_window, in_button, in_mouse_pos);
}

void Engine::on_mouse_up(platform::Window& in_window, platform::MouseButton in_button, const glm::ivec2& in_mouse_pos)
{
	imgui::on_mouse_up(in_window, in_button, in_mouse_pos);
}

void Engine::on_mouse_wheel(platform::Window& in_window, const float in_delta, const glm::ivec2& in_mouse_pos)
{
	imgui::on_mouse_wheel(in_window, in_delta, in_mouse_pos);
}

}
