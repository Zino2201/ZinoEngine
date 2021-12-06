#pragma once

#include "engine/core.hpp"
#include <imgui.h>
#include "engine/gfx/device.hpp"

namespace ze::ui
{


/**
 * Initialize ImGui renderer (create default font atlas and setup pipeline & shaders)
 */
void initialize_imgui();
void draw_imgui(gfx::CommandListHandle in_handle);
void destroy_imgui();


}