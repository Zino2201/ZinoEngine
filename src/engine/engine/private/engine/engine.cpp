#include "engine/engine.hpp"
#if ZE_FEATURE(PROFILING)
#include <Tracy.hpp>
#endif
#include "engine/application/application_module.hpp"
#include "engine/application/platform_application.hpp"
#include "engine/application/platform_window.hpp"
#include "engine/gfx/backend.hpp"
#include "engine/gfx/vulkan_backend_module.hpp"
#include "engine/imgui/imgui.hpp"
#include "engine/module/module_manager.hpp"
#include "engine/shadersystem/shader_manager.hpp"
#include "engine/filesystem/filesystem.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "engine/gfx/utils/gfx_utils_module.hpp"
#include "engine/gfx/rendergraph/render_graph.hpp"
#include "engine/gfx/rendergraph/resource_registry.hpp"
#include "engine/gfx/utils/mipmap_generation.hpp"
#include "DirectXTex.h"
#undef near
#undef far

namespace ze
{

Engine::Engine() : running(true)
{
#if ZE_BUILD(IS_DEBUG)
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

	auto gfx_utils_module = get_module<gfx::GfxUtilsModule>("gfxutils");
	gfx_utils_module->initialize_shaders(*shader_manager);

	image_available_semaphore = gfx::UniqueSemaphore(device->create_semaphore({}).get_value());
	render_finished_semaphore = gfx::UniqueSemaphore(device->create_semaphore({}).get_value());
}

Engine::~Engine()
{
	get_module<gfx::GfxUtilsModule>("gfxutils")->destroy_resources();
}

void Engine::run()
{
	const auto platform = get_module<platform::ApplicationModule>("Application");
	platform->get_application().set_message_handler(this);
	main_window = platform->get_application().create_window(
		"ZinoEngine",
		1280,
		720,
		0,
		0,
		platform::WindowFlags(platform::WindowFlagBits::Centered | platform::WindowFlagBits::Maximized | platform::WindowFlagBits::Resizable));

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
	
	std::array<gfx::rendergraph::PhysicalResourceRegistry, gfx::Device::max_frames_in_flight> registry;

	auto previous = std::chrono::high_resolution_clock::now();

	while(running)
	{
		mouse_delta = {};
		platform->get_application().pump_messages();

		device->new_frame();

		auto current = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> delta_time_ms = current - previous;
		previous = current;

		const float delta_time = static_cast<float>(delta_time_ms.count()) * 0.001f;

		imgui::new_frame(delta_time, *main_window);

		ImGui::NewFrame();
		ImGui::Text("%.0f FPS", 1.f / ImGui::GetIO().DeltaTime, ImGui::GetIO().DeltaTime);
		ImGui::Text("%.2f ms", ImGui::GetIO().DeltaTime * 1000);
		ImGui::ShowDemoWindow();
		ImGui::Render();

		using namespace gfx;
		rendergraph::RenderGraph render_graph(registry[get_device()->get_current_frame_idx()]);

		if (get_device()->acquire_swapchain_texture(swapchain.get(), image_available_semaphore.get()) != GfxResult::Success)
			continue;

		ImGui::UpdatePlatformWindows();

		imgui::draw_viewport(ImGui::GetMainViewport(), render_graph, false);
		imgui::draw_viewports(render_graph);

		render_graph.set_backbuffer_attachment("backbuffer", 
			get_device()->get_swapchain_backbuffer_view(swapchain.get()),
			main_window->get_width(),
			main_window->get_height());

		{
			ZoneScopedN("Render graph compilation");
			render_graph.compile();
		}

		const auto list = device->allocate_cmd_list(gfx::QueueType::Gfx);
		{
			ZoneScopedN("Execute render graph");
			render_graph.execute(list);
		}
		
		{
			ZoneScopedN("Submit");

			std::array submit_wait_semaphores = { image_available_semaphore.get() };
			std::array submit_signal_semaphores = { render_finished_semaphore.get() };

			device->submit(list, submit_wait_semaphores, submit_signal_semaphores);
		}

		device->end_frame();

		imgui::present_viewports();

		std::array present_wait_semaphores = { render_finished_semaphore.get() };
		device->present(swapchain.get(), present_wait_semaphores);

#if ZE_FEATURE(PROFILING)
		FrameMark;
#endif

		/** FPS Limiter */
		if(1) {
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

void Engine::on_mouse_double_click(platform::Window& in_window, platform::MouseButton in_button, const glm::ivec2& in_mouse_pos)
{
	imgui::on_mouse_double_click(in_window, in_button, in_mouse_pos);
}

void Engine::on_cursor_set()
{
	imgui::on_cursor_set();
}

void Engine::on_key_down(const platform::KeyCode in_key_code, const uint32_t in_character_code, const bool in_repeat)
{
	imgui::on_key_down(in_key_code, in_character_code, in_repeat);
}

void Engine::on_key_up(const platform::KeyCode in_key_code, const uint32_t in_character_code, const bool in_repeat)
{
	imgui::on_key_up(in_key_code, in_character_code, in_repeat);
}

void Engine::on_key_char(const char in_char)
{
	imgui::on_key_char(in_char);
}

void Engine::on_mouse_move(const glm::ivec2& in_delta)
{
	mouse_delta += in_delta;
}


}
