#include "engine/gfx/vulkan_backend_module.hpp"
#include "engine/gfx/device.hpp"
#include "engine/logger/logger.hpp"
#include "engine/logger/sinks/stdout_sink.hpp"
#include "tinyobjloader.h"
#include "engine/imgui/imgui.hpp"
#include "engine/module/module_manager.hpp"
#include "engine/gfx/vulkan_backend_module.hpp"
#include <tbb/parallel_for.h>
#include "boost/locale/generator.hpp"
#include "boost/locale/localization_backend.hpp"
#include "engine/application/application_module.hpp"
#include "engine/application/message_handler.hpp"
#include "engine/application/platform_application.hpp"

int main()
{
	using namespace ze;

	logger::set_pattern("[{time}] [{severity}] ({category}) {message}");
	logger::add_sink(std::make_unique<logger::StdoutSink>());

	{

	const boost::locale::generator generator;
	const std::locale locale = generator.generate("");
	std::locale::global(locale);

	auto platform = get_module<platform::ApplicationModule>("Application");

	class Santor : public platform::ApplicationMessageHandler
	{
	public:
		void on_resized_window(platform::Window& in_window, uint32_t in_width, uint32_t in_height) override
		{
			if(win)
				win(in_window, in_width, in_height);

			imgui::on_resized_window(in_window, in_width, in_height);
		}

		void on_closing_window(platform::Window& in_window) override
		{
			running = false;
		}

		void on_mouse_down(platform::Window& in_window, platform::MouseButton in_button, const glm::ivec2& in_mouse_pos) override
		{
			imgui::on_mouse_down(in_window, in_button, in_mouse_pos);
		}

		void on_mouse_up(platform::Window& in_window, platform::MouseButton in_button, const glm::ivec2& in_mouse_pos) override
		{
			imgui::on_mouse_up(in_window, in_button, in_mouse_pos);
		}

		void on_mouse_wheel(platform::Window& in_window, const float in_delta, const glm::ivec2& in_mouse_pos) override
		{
			imgui::on_mouse_wheel(in_window, in_delta, in_mouse_pos);
		}
	public:
		std::function<void(platform::Window&, uint32_t, uint32_t)> win;
		bool running = true;
	};

	Santor s;
	platform->get_application().set_message_handler(&s);

	std::unique_ptr<platform::Window> win = platform->get_application().create_window(
		"ZinoEngine", 
		1280, 
		720, 
		0, 
		0,
		platform::WindowFlags(platform::WindowFlagBits::Centered));

#if ZE_BUILD(DEBUG)
	auto result = get_module<gfx::VulkanBackendModule>("vulkangfx")
		->create_vulkan_backend(gfx::BackendFlags(gfx::BackendFlagBits::DebugLayers));
#else
	auto result = get_module<gfx::VulkanBackendModule>("vulkangfx")->create_vulkan_backend(gfx::BackendFlags());
#endif

	if (!result)
	{
		logger::fatal("Failed to create backend: {}", result.get_error());
		return -1;
	}

	auto backend_device = result.get_value()->create_device(gfx::ShaderModel::SM6_0);
	if (!backend_device)
	{
		logger::fatal("Failed to create device: {}", backend_device.get_error());
		return -1;
	}

	auto device = std::make_unique<gfx::Device>(*result.get_value().get(), std::move(backend_device.get_value()));

	using namespace gfx;

	UniqueSwapchain swapchain(device->create_swapchain(gfx::SwapChainCreateInfo(
		win->get_handle(),
		win->get_width(),
		win->get_height())).get_value());

	UniqueSemaphore image_available_semaphore(device->create_semaphore(SemaphoreInfo()).get_value());
	UniqueSemaphore render_finished_semaphore(device->create_semaphore(SemaphoreInfo()).get_value());
	std::array render_wait_semaphores = { image_available_semaphore.get() };
	std::array present_wait_semaphores = { render_finished_semaphore.get() };
	std::array render_finished_semaphores = { render_finished_semaphore.get() };

	std::array<DescriptorSetLayoutCreateInfo, 1> layouts;
	std::vector<DescriptorSetLayoutBinding> bindings =
	{
		DescriptorSetLayoutBinding(0, DescriptorType::UniformBuffer, 1, ShaderStageFlags(ShaderStageFlagBits::Vertex | ShaderStageFlagBits::Fragment)),
		DescriptorSetLayoutBinding(1, DescriptorType::Sampler, 1, ShaderStageFlags(ShaderStageFlagBits::Fragment)),
		DescriptorSetLayoutBinding(2, DescriptorType::SampledTexture, 1, ShaderStageFlags(ShaderStageFlagBits::Fragment)),
		DescriptorSetLayoutBinding(3, DescriptorType::SampledTexture, 1, ShaderStageFlags(ShaderStageFlagBits::Fragment)),
	};

	layouts[0].bindings = bindings;
	UniquePipelineLayout pipeline_layout(device->create_pipeline_layout(PipelineLayoutCreateInfo(layouts)).get_value());

	UniqueTexture depth_texture(device->create_texture(TextureInfo::make_depth_stencil_attachment(
		win->get_width(), win->get_height(), Format::D24UnormS8Uint).set_debug_name("Depth Buffer Texture")).get_value());

	UniqueTextureView depth_texture_view(device->create_texture_view(TextureViewInfo::make_depth(depth_texture.get(),
		Format::D24UnormS8Uint).set_debug_name("Depth Buffer View")).get_value());

	s.win = [&](platform::Window& my_win, uint32_t width, uint32_t height)
	{
		if (win.get() != &my_win)
			return;

			logger::verbose("Resizing swapchain and recreating resources...");

			UniqueSwapchain old_swapchain(swapchain.free());

			device->wait_idle();
			swapchain = UniqueSwapchain(device->create_swapchain(gfx::SwapChainCreateInfo(
				my_win.get_handle(),
				width,
				height,
				device->get_swapchain_backend_handle(old_swapchain.get()))).get_value());
			imgui::update_main_viewport(my_win, swapchain.get());

			depth_texture = UniqueTexture(device->create_texture(TextureInfo::make_depth_stencil_attachment(
				width, height, Format::D24UnormS8Uint).set_debug_name("Depth Buffer Texture")).get_value());

			depth_texture_view = UniqueTextureView(device->create_texture_view(TextureViewInfo::make_depth(depth_texture.get(),
				Format::D24UnormS8Uint).set_debug_name("Depth Buffer View")).get_value());
		};

	float cam_pitch = 0.f, cam_yaw = 0.f;

	ImGui::SetCurrentContext(ImGui::CreateContext());
	imgui::initialize();
	imgui::initialize_main_viewport(*win.get(), swapchain.get());

	while (s.running)
	{
		platform->get_application().pump_messages();

		device->new_frame();

		static auto start_time = std::chrono::high_resolution_clock::now();
		auto current_time = std::chrono::high_resolution_clock::now();
		float delta_time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
		start_time = current_time;

		imgui::new_frame(delta_time, *win.get());

		ImGui::NewFrame();
		ImGui::Text("%s (Shader Model: %s, Shader Format: %s)", result.get_value()->get_name().data(),
			std::to_string(ShaderModel::SM6_0).c_str(),
			std::to_string(result.get_value()->get_shader_language()).c_str());
		ImGui::Text("%.0f FPS", 1.f / ImGui::GetIO().DeltaTime, ImGui::GetIO().DeltaTime);
		ImGui::Text("%.2f ms", ImGui::GetIO().DeltaTime * 1000);
		ImGui::Text("Mouse Pos: %f %f", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
		ImGui::ShowDemoWindow();
		ImGui::Render();

		ImGui::UpdatePlatformWindows();
		imgui::draw_viewports();
		device->end_frame();

		imgui::present_viewports();

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(6ms);
	}
	ImGui::DestroyContext();
	imgui::destroy();
	}

	unload_all_modules();

	return 0;
}
