#pragma once

#include "engine/core.hpp"
#include <imgui.h>
#include "engine/application/message_handler.hpp"
#include "engine/gfx/device.hpp"
#include "glm/vec2.hpp"

namespace ze { class PlatformWindow;  }

namespace ze::ui
{


/**
 * Initialize ImGui renderer (create default font atlas and setup pipeline & shaders)
 */
void initialize_imgui(PlatformWindow* in_window);
void new_frame();
void draw_imgui(gfx::CommandListHandle in_handle);
void on_mouse_down(PlatformWindow& in_window, PlatformMouseButton in_button, const glm::ivec2& in_mouse_pos);
void on_mouse_up(PlatformWindow& in_window, PlatformMouseButton in_button, const glm::ivec2& in_mouse_pos);
void on_mouse_wheel(PlatformWindow& in_window, const float in_delta, const glm::ivec2& in_mouse_pos);
void destroy_imgui();


}