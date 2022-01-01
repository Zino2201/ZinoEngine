#include "engine/engine.hpp"
#include "engine/application/application_module.hpp"
#include "engine/application/platform_application.hpp"
#include "engine/application/platform_window.hpp"
#include "engine/gfx/backend.hpp"
#include "engine/gfx/vulkan_backend_module.hpp"
#include "engine/imgui/imgui.hpp"
#include "engine/module/module_manager.hpp"
#include "engine/shadersystem/shader_manager.hpp"

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

	while(running)
	{
		platform->get_application().pump_messages();

		device->new_frame();

		static auto start_time = std::chrono::high_resolution_clock::now();
		auto current_time = std::chrono::high_resolution_clock::now();
		const float delta_time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
		start_time = current_time;

		imgui::new_frame(delta_time, *main_window.get());

		ImGui::NewFrame();
		ImGui::Text("%.0f FPS", 1.f / ImGui::GetIO().DeltaTime, ImGui::GetIO().DeltaTime);
		ImGui::Text("%.2f ms", ImGui::GetIO().DeltaTime * 1000);
		ImGui::Text("Mouse Pos: %f %f", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
		ImGui::SetNextWindowBgAlpha(0.2f);
		ImGui::ShowDemoWindow();
		ImGui::Render();

		ImGui::UpdatePlatformWindows();
		imgui::draw_viewports();
		device->end_frame();

		imgui::present_viewports();
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

void Engine::on_closing_window(platform::Window& in_window)
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
