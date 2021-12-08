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

	auto platform = get_module<ApplicationModule>("Application");

	class Santor : public PlatformApplicationMessageHandler
	{
	public:
		void on_resized_window(PlatformWindow& in_window, uint32_t in_width, uint32_t in_height) override
		{
			if(win)
				win(in_window, in_width, in_height);
		}

		void on_closing_window(PlatformWindow& in_window) override
		{
			running = false;
		}

		void on_mouse_down(PlatformWindow& in_window, PlatformMouseButton in_button, const glm::ivec2& in_mouse_pos) override
		{
			ui::on_mouse_down(in_window, in_button, in_mouse_pos);
		}

		void on_mouse_up(PlatformWindow& in_window, PlatformMouseButton in_button, const glm::ivec2& in_mouse_pos) override
		{
			ui::on_mouse_up(in_window, in_button, in_mouse_pos);
		}

		void on_mouse_wheel(PlatformWindow& in_window, const float in_delta, const glm::ivec2& in_mouse_pos) override
		{
			ui::on_mouse_wheel(in_window, in_delta, in_mouse_pos);
		}
	public:
		std::function<void(PlatformWindow&, uint32_t, uint32_t)> win;
		bool running = true;
	};

	Santor s;
	platform->get_application().set_message_handler(&s);

	std::unique_ptr<PlatformWindow> win = platform->get_application().create_window(
		"ZinoEngine", 
		1280, 
		720, 
		0, 
		0,
		PlatformWindowFlags(PlatformWindowFlagBits::Centered | PlatformWindowFlagBits::Maximized));

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

	s.win = [&](PlatformWindow& win, uint32_t width, uint32_t height)
	{
			logger::verbose("Resizing swapchain and recreating resources...");

			UniqueSwapchain old_swapchain(swapchain.free());

			device->wait_idle();
			swapchain = UniqueSwapchain(device->create_swapchain(gfx::SwapChainCreateInfo(
				win.get_handle(),
				width,
				height,
				device->get_swapchain_backend_handle(old_swapchain.get()))).get_value());

			depth_texture = UniqueTexture(device->create_texture(TextureInfo::make_depth_stencil_attachment(
				width, height, Format::D24UnormS8Uint).set_debug_name("Depth Buffer Texture")).get_value());

			depth_texture_view = UniqueTextureView(device->create_texture_view(TextureViewInfo::make_depth(depth_texture.get(),
				Format::D24UnormS8Uint).set_debug_name("Depth Buffer View")).get_value());
		};

	float cam_pitch = 0.f, cam_yaw = 0.f;

	ImGui::SetCurrentContext(ImGui::CreateContext());
	ui::initialize_imgui(win.get());

	while (s.running)
	{
		platform->get_application().pump_messages();
		ui::new_frame();

		if (device->acquire_swapchain_texture(swapchain.get(),
			image_available_semaphore.get()) != gfx::GfxResult::Success)
			continue;

		device->new_frame();

		static auto start_time = std::chrono::high_resolution_clock::now();
		auto current_time = std::chrono::high_resolution_clock::now();
		float delta_time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
		start_time = current_time;

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = { static_cast<float>(win->get_width()), static_cast<float>(win->get_height()) };
		io.DeltaTime = delta_time;

		ImGui::NewFrame();
		ImGui::Text("%s (Shader Model: %s, Shader Format: %s)", result.get_value()->get_name().data(),
			std::to_string(ShaderModel::SM6_0).c_str(),
			std::to_string(result.get_value()->get_shader_language()).c_str());
		ImGui::Text("%.0f FPS", 1.f / ImGui::GetIO().DeltaTime, ImGui::GetIO().DeltaTime);
		ImGui::Text("%.2f ms", ImGui::GetIO().DeltaTime * 1000);
		ImGui::Text("Mouse Pos: %f %f", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
		ImGui::ShowDemoWindow();
		ImGui::Render();

		auto list = device->allocate_cmd_list(QueueType::Gfx);

		std::array clear_values = { ClearValue(ClearColorValue({0, 0, 0, 1})),
			ClearValue(ClearDepthStencilValue(1.f, 0)) };
		std::array color_attachments = { device->get_swapchain_backbuffer_view(swapchain.get()) };

		RenderPassInfo info;
		info.render_area = Rect2D(0, 0, win->get_width(), win->get_height());
		info.color_attachments = color_attachments;
		info.clear_attachment_flags = 1 << 0;
		info.store_attachment_flags = 1 << 0;
		info.clear_values = clear_values;
		info.depth_stencil_attachment = depth_texture_view.get();

		std::array color_attachments_refs = { 0Ui32 };
		std::array subpasses = { RenderPassInfo::Subpass(color_attachments_refs,
			{},
			{},
			RenderPassInfo::DepthStencilMode::ReadWrite) };
		info.subpasses = subpasses;
		device->cmd_begin_render_pass(list, info);
		device->cmd_bind_texture_view(list, 0, 3, TextureViewHandle());

		ui::draw_imgui(list);

		device->cmd_end_render_pass(list);
		device->submit(list, render_wait_semaphores, render_finished_semaphores);
		device->end_frame();

		device->present(swapchain.get(), present_wait_semaphores);

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(6ms);
	}
	ui::destroy_imgui();
	}

	ImGui::DestroyContext();
	unload_all_modules();

	return 0;
}
