#include "engine/imgui/imgui.hpp"
#include <Unknwn.h>
#include <fstream>
#include <glm/glm.hpp>
#include "engine/shadersystem/shader_manager.hpp"
#include "engine/application/application_module.hpp"
#include "engine/application/platform_application.hpp"
#include "engine/module/module_manager.hpp"

namespace ze::imgui
{

using namespace gfx;

SamplerHandle sampler;
TextureHandle font_texture;
TextureViewHandle font_texture_view;
std::unique_ptr<shadersystem::ShaderInstance> shader_instance;
std::vector<PipelineShaderStage> shader_stages;
PipelineRenderPassState render_pass_state;
PipelineMaterialState material_state;
std::vector<std::unique_ptr<platform::Cursor>> mouse_cursors;
ImGuiMouseCursor last_mouse_cursor;

std::array color_blend_states = { PipelineColorBlendAttachmentState(
	true,
	BlendFactor::SrcAlpha,
	BlendFactor::OneMinusSrcAlpha,
	BlendOp::Add,
	BlendFactor::OneMinusSrcAlpha,
	BlendFactor::Zero,
	BlendOp::Add) };

std::vector<uint8_t> read_text_file(const std::string& in_name)
{
	std::ifstream file(in_name, std::ios::ate | std::ios::binary);

	const size_t file_size = file.tellg();

	std::vector<uint8_t> buffer(file_size);
	file.seekg(0);
	file.read((char*)buffer.data(), file_size);
	file.close();

	return buffer;
}

void initialize(shadersystem::ShaderManager& in_shader_manager)
{
	IMGUI_CHECKVERSION();

	const auto platform = get_module<platform::ApplicationModule>("Application");

	ImGuiIO& io = ImGui::GetIO();
	io.BackendPlatformName = "zinoengine_imgui_application";
	io.BackendRendererName = "zinoengine_imgui_renderer";
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

	/** Cursors */
	mouse_cursors.resize(ImGuiMouseCursor_COUNT);
	mouse_cursors[ImGuiMouseCursor_Arrow] = platform->get_application().create_system_cursor(platform::SystemCursor::Arrow);
	mouse_cursors[ImGuiMouseCursor_TextInput] = platform->get_application().create_system_cursor(platform::SystemCursor::Ibeam);
	mouse_cursors[ImGuiMouseCursor_ResizeAll] = platform->get_application().create_system_cursor(platform::SystemCursor::SizeAll);
	mouse_cursors[ImGuiMouseCursor_ResizeNS] = platform->get_application().create_system_cursor(platform::SystemCursor::SizeNorthSouth);
	mouse_cursors[ImGuiMouseCursor_ResizeEW] = platform->get_application().create_system_cursor(platform::SystemCursor::SizeWestEast);
	mouse_cursors[ImGuiMouseCursor_ResizeNESW] = platform->get_application().create_system_cursor(platform::SystemCursor::SizeNorthEastSouthWest);
	mouse_cursors[ImGuiMouseCursor_ResizeNWSE] = platform->get_application().create_system_cursor(platform::SystemCursor::SizeNorthWestSouthEast);
	mouse_cursors[ImGuiMouseCursor_Hand] = platform->get_application().create_system_cursor(platform::SystemCursor::Hand);
	mouse_cursors[ImGuiMouseCursor_NotAllowed] = platform->get_application().create_system_cursor(platform::SystemCursor::No);

	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
	platform_io.Platform_CreateWindow = [](ImGuiViewport* viewport)
	{
		const auto platform = get_module<platform::ApplicationModule>("Application");

		auto* platform_data = new ViewportPlatformData;
		viewport->PlatformUserData = platform_data;

		platform_data->window = platform->get_application().create_window("ImGui Viewport Window",
			static_cast<uint32_t>(viewport->Size.x),
			static_cast<uint32_t>(viewport->Size.y),
			static_cast<uint32_t>(viewport->Pos.x),
			static_cast<uint32_t>(viewport->Pos.y),
			platform::WindowFlags(platform::WindowFlagBits::Borderless));

		viewport->PlatformHandleRaw = platform_data->get_window()->get_handle();
	};

	platform_io.Platform_GetWindowSize = [](ImGuiViewport*) -> ImVec2
	{
		const auto* platform_data = new ViewportPlatformData;
		return
		{
			static_cast<float>(platform_data->get_window()->get_width()),
			static_cast<float>(platform_data->get_window()->get_height())
		};
	};

	platform_io.Platform_GetWindowPos = [](ImGuiViewport* viewport) -> ImVec2
	{
		const auto* platform_data = static_cast<ViewportPlatformData*>(viewport->PlatformUserData);
		return ImVec2(static_cast<float>(platform_data->get_window()->get_position().x),
			static_cast<float>(platform_data->get_window()->get_position().y));
	};

	platform_io.Platform_SetWindowPos = [](ImGuiViewport* viewport, ImVec2 size)
	{
		const auto* platform_data = static_cast<ViewportPlatformData*>(viewport->PlatformUserData);
		platform_data->get_window()->set_position({ size.x, size.y });
	};

	platform_io.Platform_SetWindowSize = [](ImGuiViewport* viewport, ImVec2 size)
	{
		const auto* platform_data = static_cast<ViewportPlatformData*>(viewport->PlatformUserData);
		platform_data->get_window()->set_size({ size.x, size.y });
	};

	platform_io.Platform_SetWindowTitle = [](ImGuiViewport* viewport, const char* title)
	{
		const auto* platform_data = static_cast<ViewportPlatformData*>(viewport->PlatformUserData);
		platform_data->get_window()->set_title(title);
	};

	platform_io.Platform_ShowWindow = [](ImGuiViewport* viewport)
	{
		const auto* platform_data = static_cast<ViewportPlatformData*>(viewport->PlatformUserData);
		platform_data->get_window()->show();
	};

	platform_io.Platform_SetWindowAlpha = [](ImGuiViewport* viewport, float alpha)
	{
		const auto* platform_data = static_cast<ViewportPlatformData*>(viewport->PlatformUserData);
		platform_data->get_window()->set_opacity(alpha);
	};

	platform_io.Platform_DestroyWindow = [](ImGuiViewport* viewport)
	{
		delete static_cast<ViewportPlatformData*>(viewport->PlatformUserData);
		viewport->PlatformUserData = nullptr;
	};

	platform_io.Renderer_CreateWindow = [](ImGuiViewport* viewport)
	{
		auto* renderer_data = new ViewportRendererData;
		viewport->RendererUserData = renderer_data;

		renderer_data->window.swapchain = UniqueSwapchain(get_device()->create_swapchain(SwapChainCreateInfo(
			viewport->PlatformHandleRaw,
			static_cast<uint32_t>(viewport->Size.x),
			static_cast<uint32_t>(viewport->Size.y))).get_value());
	};

	platform_io.Renderer_DestroyWindow = [](ImGuiViewport* viewport)
	{
		delete static_cast<ViewportRendererData*>(viewport->RendererUserData);
		viewport->RendererUserData = nullptr;
	};

	platform_io.Renderer_SetWindowSize = [](ImGuiViewport* viewport, ImVec2)
	{
		if (viewport == ImGui::GetMainViewport())
			return;
		
		get_device()->wait_idle();

		logger::verbose("Resizing swapchain for imgui window to {}x{}", viewport->Size.x, viewport->Size.y);
		auto* renderer_data = static_cast<ViewportRendererData*>(viewport->RendererUserData);
		const auto old_swapchain = UniqueSwapchain(std::get<0>(renderer_data->window.swapchain).free());
		renderer_data->window.swapchain = UniqueSwapchain(get_device()->create_swapchain(SwapChainCreateInfo(
			viewport->PlatformHandleRaw,
			static_cast<uint32_t>(viewport->Size.x),
			static_cast<uint32_t>(viewport->Size.y),
			get_device()->get_swapchain_backend_handle(old_swapchain.get()))).get_value());
	};

	platform_io.Renderer_SwapBuffers = [](ImGuiViewport* viewport, void*)
	{
		swap_buffers(viewport);
	};

	platform_io.Renderer_RenderWindow = [](ImGuiViewport* viewport, void*)
	{
		auto* renderer_data = static_cast<ViewportRendererData*>(viewport->RendererUserData);
		renderer_data->has_submitted_work = false;

		if (get_device()->acquire_swapchain_texture(renderer_data->window.get_swapchain(), 
			renderer_data->image_available_semaphore.get()) == GfxResult::Success)
			draw_viewport(viewport);
	};

	io.Fonts->AddFontFromFileTTF("assets/fonts/ReadexPro-Light.ttf", 18.f);

	{
		auto result = get_device()->create_sampler(SamplerInfo());
		ZE_ASSERTF(result.has_value(), "Failed to create ImGui UBO: {}", std::to_string(result.get_error()));
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
			Format::R8G8B8A8Srgb,
			1,
			TextureUsageFlags(TextureUsageFlagBits::Sampled),
			{ data, data + (width * height * 4)}).set_debug_name("ImGui Font Texture"));
		ZE_ASSERTF(tex_result.has_value(), "Failed to create ImGui font texture: {}", std::to_string(tex_result.get_error()));

		font_texture = tex_result.get_value();

		auto view_result = get_device()->create_texture_view(TextureViewInfo::make_2d(
			font_texture,
			Format::R8G8B8A8Srgb));
		ZE_ASSERTF(view_result.has_value(), "Failed to create ImGui font texture view: {}", std::to_string(view_result.get_error()));
		font_texture_view = view_result.get_value();
	}

	/** Get shaders */
	{
		shader_instance = in_shader_manager.get_shader("ImGui")->instantiate({});
		const auto& shader_map = shader_instance->get_permutation().get_shader_map();
		ZE_ASSERTF(shader_map.size() == 2, "Failed to create ImGui shaders, see log. Exiting.");
		auto vertex_shader = shader_map.find(ShaderStageFlagBits::Vertex);
		auto fragment_shader = shader_map.find(ShaderStageFlagBits::Fragment);
		shader_stages.emplace_back(ShaderStageFlagBits::Vertex, Device::get_backend_shader(*vertex_shader->second), "main");
		shader_stages.emplace_back(ShaderStageFlagBits::Fragment, Device::get_backend_shader(*fragment_shader->second), "main");
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

	update_monitors();
}

void initialize_main_viewport(platform::Window& in_window, SwapchainHandle in_swapchain)
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	auto* platform_data = new ViewportPlatformData;
	auto* renderer_data = new ViewportRendererData;

	platform_data->window = &in_window;

	viewport->PlatformHandle = &in_window;
	viewport->PlatformHandleRaw = in_window.get_handle();
	viewport->PlatformUserData = platform_data;
	viewport->RendererUserData = renderer_data;
	renderer_data->window.swapchain = in_swapchain;
}

void update_main_viewport(platform::Window&, SwapchainHandle in_swapchain)
{
	const auto* viewport = ImGui::GetMainViewport();
	auto* renderer_data = reinterpret_cast<ViewportRendererData*>(viewport->RendererUserData);
	renderer_data->window.swapchain = in_swapchain;
}

void new_frame(float in_delta_time, platform::Window& in_main_window)
{
	const auto platform = get_module<platform::ApplicationModule>("Application");

	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = in_delta_time;

	ZE_CHECK(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable);

	/** Update cursor & mouse pos */
	const auto mouse_pos = platform->get_application().get_mouse_pos();
	io.MousePos = ImVec2(static_cast<float>(mouse_pos.x), static_cast<float>(mouse_pos.y));
	io.DisplaySize = ImVec2(static_cast<float>(in_main_window.get_width()), 
		static_cast<float>(in_main_window.get_height()));

	const ImGuiMouseCursor cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
	if (last_mouse_cursor != cursor)
	{
		update_mouse_cursor();
	}

	/** Acquire main window image as soon as possible */
	{
		auto* renderer_data = static_cast<ViewportRendererData*>(ImGui::GetMainViewport()->RendererUserData);
		renderer_data->has_submitted_work = false;
		get_device()->acquire_swapchain_texture(renderer_data->window.get_swapchain(),
			renderer_data->image_available_semaphore.get());
	}
}

void update_mouse_cursor()
{
	ImGuiIO& io = ImGui::GetIO();
	const auto platform = get_module<platform::ApplicationModule>("Application");

	const ImGuiMouseCursor cursor = ImGui::GetMouseCursor();
	last_mouse_cursor = cursor;

	if (io.MouseDrawCursor || cursor == ImGuiMouseCursor_None)
	{
		platform->get_application().set_cursor(nullptr);
	}
	else
	{
		platform->get_application().set_cursor(mouse_cursors[cursor] ? mouse_cursors[cursor].get() : mouse_cursors[ImGuiMouseCursor_Arrow].get());
	}
}

void draw_viewport(ImGuiViewport* viewport)
{
	auto* renderer_data = static_cast<ViewportRendererData*>(viewport->RendererUserData);
	auto update_viewport_buffers = [&](ImDrawData* draw_data, ViewportDrawData& vp_draw_data)
	{
		if (draw_data->TotalVtxCount == 0)
			return;

		const uint64_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
		const uint64_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

		/** Create/resize buffers */
		if (!vp_draw_data.vertex_buffer || vp_draw_data.vertex_buffer_size < vertex_size)
		{
			vp_draw_data.vertex_buffer = UniqueBuffer(get_device()->create_buffer(
				gfx::BufferInfo::make_vertex_buffer_cpu_visible(
					vertex_size)).get_value());
			if (!vp_draw_data.vertex_buffer)
			{
				ze::logger::error("Failed to create ImGui vertex buffer");
				return;
			}

			vp_draw_data.vertex_buffer_size = vertex_size;
		}

		if (!vp_draw_data.index_buffer || vp_draw_data.index_buffer_size < index_size)
		{
			vp_draw_data.index_buffer = UniqueBuffer(get_device()->create_buffer(
				gfx::BufferInfo::make_index_buffer_cpu_visible(
					index_size)).get_value());
			if (!vp_draw_data.index_buffer)
			{
				ze::logger::error("Failed to create ImGui index buffer");
				return;
			}

			vp_draw_data.index_buffer_size = index_size;
		}

		/** Write data to both buffers */
		auto vertex_map = get_device()->map_buffer(*vp_draw_data.vertex_buffer);
		auto index_map = get_device()->map_buffer(*vp_draw_data.index_buffer);

		auto* vertex_data = static_cast<ImDrawVert*>(vertex_map.get_value());
		auto* index_data = static_cast<ImDrawIdx*>(index_map.get_value());

		for (int i = 0; i < draw_data->CmdListsCount; i++)
		{
			const auto* cmd_list = draw_data->CmdLists[i];
			memcpy(vertex_data, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(index_data, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			vertex_data += cmd_list->VtxBuffer.Size;
			index_data += cmd_list->IdxBuffer.Size;
		}

		get_device()->unmap_buffer(*vp_draw_data.vertex_buffer);
		get_device()->unmap_buffer(*vp_draw_data.index_buffer);

		/** Global data */
		ViewportDrawData::GlobalData global_data;
		global_data.scale = { 2.f / draw_data->DisplaySize.x, 2.f / draw_data->DisplaySize.y };
		global_data.translate.x = -1.f - draw_data->DisplayPos.x * global_data.scale.x;
		global_data.translate.y = -1.f - draw_data->DisplayPos.y * global_data.scale.y;
		vp_draw_data.global_data.update(global_data);
	};

	update_viewport_buffers(viewport->DrawData, renderer_data->draw_data);

	const auto list = get_device()->allocate_cmd_list(QueueType::Gfx);

	std::array clear_values = { ClearValue(ClearColorValue({0, 0, 0, 1})),
			ClearValue(ClearDepthStencilValue(1.f, 0)) };
	std::array color_attachments = { get_device()->get_swapchain_backbuffer_view(renderer_data->window.get_swapchain()) };
	RenderPassInfo render_pass_info;
	render_pass_info.render_area = Rect2D(0, 0, 
		static_cast<uint32_t>(viewport->Size.x), static_cast<uint32_t>(viewport->Size.y));
	render_pass_info.color_attachments = color_attachments;

	if(viewport == ImGui::GetMainViewport())
		render_pass_info.load_attachment_flags = 1 << 0;
	else
		render_pass_info.clear_attachment_flags = 1 << 0;
	render_pass_info.store_attachment_flags = 1 << 0;
	render_pass_info.clear_values = clear_values;

	std::array color_attachments_refs = { 0Ui32 };
	std::array subpasses = { RenderPassInfo::Subpass(color_attachments_refs,
		{},
		{},
		RenderPassInfo::DepthStencilMode::ReadOnly) };
	render_pass_info.subpasses = subpasses;

	get_device()->cmd_begin_render_pass(list, render_pass_info);

	get_device()->cmd_bind_pipeline_layout(list, shader_instance->get_permutation().get_pipeline_layout());
	get_device()->cmd_set_render_pass_state(list, render_pass_state);
	get_device()->cmd_set_material_state(list, material_state);

	get_device()->cmd_bind_ubo(list, 0, 0, renderer_data->draw_data.global_data.get_handle());
	get_device()->cmd_bind_sampler(list, 0, 1, sampler);

	const ImDrawData* draw_data = viewport->DrawData;

	if (draw_data->CmdListsCount > 0)
	{
		get_device()->cmd_bind_vertex_buffer(list, renderer_data->draw_data.vertex_buffer.get(), 0);
		get_device()->cmd_bind_index_buffer(list, renderer_data->draw_data.index_buffer.get(), 0, IndexType::Uint16);

		uint32_t vertex_offset = 0;
		uint32_t index_offset = 0;

		for (int32_t i = 0; i < draw_data->CmdListsCount; ++i)
		{
			ImDrawList* draw_list = draw_data->CmdLists[i];
			for (int32_t j = 0; j < draw_list->CmdBuffer.Size; ++j)
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

				if (!cmd.TextureId)
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

	get_device()->cmd_end_render_pass(list);

	std::array wait_semaphores = { renderer_data->image_available_semaphore.get() };
	std::array signal_semaphores = { renderer_data->render_finished_semaphore.get() };
	get_device()->submit(list, wait_semaphores, signal_semaphores);
	renderer_data->has_submitted_work = true;
}

void swap_buffers(ImGuiViewport* viewport)
{
	const auto* renderer_data = static_cast<ViewportRendererData*>(viewport->RendererUserData);
	if (renderer_data->has_submitted_work)
	{
		std::array render_finished_semaphores = { renderer_data->render_finished_semaphore.get() };
		get_device()->present(renderer_data->window.get_swapchain(),
			render_finished_semaphores);
	}
}

void draw_viewports()
{
	draw_viewport(ImGui::GetMainViewport());

	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
	for(int i = 1; i < platform_io.Viewports.Size; i++)
	{
		ImGuiViewport* viewport = platform_io.Viewports[i];
		if (viewport->Flags & ImGuiViewportFlags_Minimized)
			continue;

		platform_io.Renderer_RenderWindow(viewport, nullptr);
	}
}

void present_viewports()
{
	/** TODO: Batch presents in one present call */

	swap_buffers(ImGui::GetMainViewport());

	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
	for (int i = 1; i < platform_io.Viewports.Size; i++)
	{
		ImGuiViewport* viewport = platform_io.Viewports[i];
		if (viewport->Flags & ImGuiViewportFlags_Minimized)
			continue;

		platform_io.Renderer_SwapBuffers(viewport, nullptr);
	}
}

void update_monitors()
{
	const auto platform = get_module<platform::ApplicationModule>("Application");
	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
	platform_io.Monitors.resize(0);
	for (uint32_t i = 0; i < platform->get_application().get_num_monitors(); ++i)
	{
		const auto& monitor = platform->get_application().get_monitor_info(i);

		ImGuiPlatformMonitor imgui_monitor;
		imgui_monitor.MainPos = ImVec2(static_cast<float>(monitor.bounds.x), static_cast<float>(monitor.bounds.y));
		imgui_monitor.MainSize = ImVec2(static_cast<float>(monitor.bounds.z), static_cast<float>(monitor.bounds.w));
		imgui_monitor.WorkPos = ImVec2(static_cast<float>(monitor.work_bounds.x), static_cast<float>(monitor.work_bounds.y));
		imgui_monitor.WorkSize = ImVec2(static_cast<float>(monitor.work_bounds.z), static_cast<float>(monitor.work_bounds.w));
		imgui_monitor.DpiScale = monitor.dpi / 96.f;
		platform_io.Monitors.push_back(imgui_monitor);
	}
}

void destroy()
{
	mouse_cursors.clear();

	shader_instance.reset();

	get_device()->destroy_texture(font_texture);
	get_device()->destroy_texture_view(font_texture_view);

	get_device()->destroy_sampler(sampler);
}

void on_mouse_down(platform::Window&, platform::MouseButton in_button, const glm::ivec2&)
{
	ImGui::GetIO().MouseDown[static_cast<size_t>(in_button)] = true;
}

void on_mouse_double_click(platform::Window&, platform::MouseButton in_button, const glm::ivec2&)
{
	ImGui::GetIO().MouseDown[static_cast<size_t>(in_button)] = true;
}

void on_mouse_up(platform::Window&, platform::MouseButton in_button, const glm::ivec2&)
{
	ImGui::GetIO().MouseDown[static_cast<size_t>(in_button)] = false;
}

void on_mouse_wheel(platform::Window&, const float in_delta, const glm::ivec2&)
{
	ImGui::GetIO().MouseWheel += in_delta;
}

void on_resized_window(platform::Window& in_window, uint32_t in_width, uint32_t in_height)
{
	if (ImGui::GetCurrentContext())
	{
		ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		if (viewport->PlatformHandleRaw == in_window.get_handle())
		{
			platform_io.Renderer_SetWindowSize(ImGui::GetMainViewport(),
				ImVec2(static_cast<float>(in_width), static_cast<float>(in_height)));
		}
	}
}

void on_cursor_set()
{
	update_mouse_cursor();
}

static ImGuiKey virtual_key_to_imgui_key(platform::KeyCode in_key_code)
{
	switch (in_key_code)
	{
	case platform::KeyCode::Num0: return ImGuiKey_0;
	case platform::KeyCode::Num1: return ImGuiKey_1;
	case platform::KeyCode::Num2: return ImGuiKey_2;
	case platform::KeyCode::Num3: return ImGuiKey_3;
	case platform::KeyCode::Num4: return ImGuiKey_4;
	case platform::KeyCode::Num5: return ImGuiKey_5;
	case platform::KeyCode::Num6: return ImGuiKey_6;
	case platform::KeyCode::Num7: return ImGuiKey_7;
	case platform::KeyCode::Num8: return ImGuiKey_8;
	case platform::KeyCode::Num9: return ImGuiKey_9;
	case platform::KeyCode::A: return ImGuiKey_A;
	case platform::KeyCode::B: return ImGuiKey_B;
	case platform::KeyCode::C: return ImGuiKey_C;
	case platform::KeyCode::D: return ImGuiKey_D;
	case platform::KeyCode::E: return ImGuiKey_E;
	case platform::KeyCode::F: return ImGuiKey_F;
	case platform::KeyCode::G: return ImGuiKey_G;
	case platform::KeyCode::H: return ImGuiKey_H;
	case platform::KeyCode::I: return ImGuiKey_I;
	case platform::KeyCode::J: return ImGuiKey_J;
	case platform::KeyCode::K: return ImGuiKey_K;
	case platform::KeyCode::L: return ImGuiKey_L;
	case platform::KeyCode::M: return ImGuiKey_M;
	case platform::KeyCode::N: return ImGuiKey_N;
	case platform::KeyCode::O: return ImGuiKey_O;
	case platform::KeyCode::P: return ImGuiKey_P;
	case platform::KeyCode::Q: return ImGuiKey_Q;
	case platform::KeyCode::R: return ImGuiKey_R;
	case platform::KeyCode::S: return ImGuiKey_S;
	case platform::KeyCode::T: return ImGuiKey_T;
	case platform::KeyCode::U: return ImGuiKey_U;
	case platform::KeyCode::V: return ImGuiKey_V;
	case platform::KeyCode::W: return ImGuiKey_W;
	case platform::KeyCode::X: return ImGuiKey_X;
	case platform::KeyCode::Y: return ImGuiKey_Y;
	case platform::KeyCode::Z: return ImGuiKey_Z;
	case platform::KeyCode::Numpad0: return ImGuiKey_Keypad0;
	case platform::KeyCode::Numpad1: return ImGuiKey_Keypad1;
	case platform::KeyCode::Numpad2: return ImGuiKey_Keypad2;
	case platform::KeyCode::Numpad3: return ImGuiKey_Keypad3;
	case platform::KeyCode::Numpad4: return ImGuiKey_Keypad4;
	case platform::KeyCode::Numpad5: return ImGuiKey_Keypad5;
	case platform::KeyCode::Numpad6: return ImGuiKey_Keypad6;
	case platform::KeyCode::Numpad7: return ImGuiKey_Keypad7;
	case platform::KeyCode::Numpad8: return ImGuiKey_Keypad8;
	case platform::KeyCode::Numpad9: return ImGuiKey_Keypad9;
	case platform::KeyCode::Escape: return ImGuiKey_Escape;
	case platform::KeyCode::LeftControl: return ImGuiKey_LeftCtrl;
	case platform::KeyCode::RightControl: return ImGuiKey_RightCtrl;
	case platform::KeyCode::LeftAlt: return ImGuiKey_LeftAlt;
	case platform::KeyCode::RightAlt: return ImGuiKey_RightAlt;
	case platform::KeyCode::LeftShift: return ImGuiKey_LeftShift;
	case platform::KeyCode::RightShift: return ImGuiKey_RightShift;
	default: return ImGuiKey_None;
	}
}

void on_key_down(const platform::KeyCode in_key_code, const uint32_t in_character_code, const bool in_repeat)
{
	ImGui::GetIO().AddKeyEvent(virtual_key_to_imgui_key(in_key_code), true);
	UnusedParameters{ in_character_code, in_repeat };
}

void on_key_up(const platform::KeyCode in_key_code, const uint32_t in_character_code, const bool in_repeat)
{
	ImGui::GetIO().AddKeyEvent(virtual_key_to_imgui_key(in_key_code), false);
	UnusedParameters{ in_character_code, in_repeat };
}

}
