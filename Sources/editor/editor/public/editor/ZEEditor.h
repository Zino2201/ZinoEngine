#pragma once

#include "engine/Engine.h"
#include <filesystem>
#include "gfx/Gfx.h"
#include "engine/Viewport.h"
#include "imgui/ImGuiRenderer.h"
#include "maths/Vector.h"
#include "maths/Matrix.h"

namespace ze 
{ 
class Viewport; 
class World; 
class NativeWindow; 
}

struct ModelVertexCon
{
	ze::maths::Vector3f position;
	ze::maths::Vector3f normal;
	ze::maths::Vector2f uv;

	bool operator==(const ModelVertexCon& other) const
	{
		return position == other.position;
	}
};

namespace std
{

	template<> struct hash<ModelVertexCon>
	{
		ZE_FORCEINLINE uint64_t operator()(const ModelVertexCon& in_vertex) const
		{
			uint64_t hash = 0;

			ze::hash_combine(hash, in_vertex.position);
			ze::hash_combine(hash, in_vertex.uv);
			ze::hash_combine(hash, in_vertex.normal);

			return hash;
		}
	};
}

struct ImFont;

namespace ze::editor
{

class Window;

struct EditorTask
{
	std::string text;
	uint32_t work_amount;
	uint32_t completed_work;

	EditorTask(const std::string& in_text, const uint32_t& in_work_amount) :
		text(in_text), work_amount(in_work_amount), completed_work(0) {}
};

class EditorApp final : public EngineApp
{
public:
	EditorApp();
	~EditorApp();

	EditorApp(const EditorApp&) = delete;
	void operator=(const EditorApp&) = delete;

	static EditorApp& get();

	void draw();
	void draw_task();
	void push_task(const EditorTask& in_task) 
	{
		tasks.push_back(in_task);
	}

	void pop_task() 
	{ 
		tasks.pop_back(); 
		if(tasks.size() == 0) 
		{ 
			ImGui::SetCurrentContext(main_context); 
			gfx::Device::get().cancel_frame();
			gfx::Device::get().wait_gpu_idle();
		} 
	}

	EditorTask& get_top_task() { return tasks.back(); }

	void process_event(const SDL_Event& in_event, const float in_delta_time) override;
	void post_tick(const float in_delta_time) override;

	void add_window(OwnerPtr<Window> in_window);
	bool has_window(const std::string& in_title);
private:
	void on_asset_imported(const std::filesystem::path& InPath,
		const std::filesystem::path& InTarget);
private:
	struct TEEST
	{
		maths::Matrix4f wvp;
		maths::Matrix4f world;
		maths::Vector3f cam_pos;
	};

	ImFont* font;
	ImGuiContext* main_context;
	ImGuiContext* tasks_window_context;
	std::unique_ptr<NativeWindow> window;
	std::vector<std::unique_ptr<Window>> main_windows;
	std::vector<std::unique_ptr<Window>> main_windows_queue;
	ui::imgui::ViewportData main_viewport_data;
	std::vector<EditorTask> tasks;
	gfx::UniqueBuffer landscape_vbuffer;
	gfx::UniqueBuffer landscape_ibuffer;
	gfx::UniqueShader shader_vert;
	gfx::UniqueShader shader_frag;
	gfx::UniqueTexture depth_texture;
	gfx::UniqueTextureView depth_view;
	gfx::UniformBuffer<TEEST> ubo_wvp;
	gfx::UniquePipelineLayout landscape_playout;
	std::vector<ModelVertexCon> landscape_vertices;
	std::vector<uint32_t> landscape_indices;
	maths::Vector3f cam_pos;
	maths::Vector3f cam_fwd;
	float cam_pitch = 0.f, cam_yaw = 0.f;
	std::vector<Window*> expired_childs;
	ImFontAtlas font_atlas;
	bool cancel_next_submission;
};

}