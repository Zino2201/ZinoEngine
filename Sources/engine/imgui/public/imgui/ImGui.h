#pragma once

#include "EngineCore.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include "module/Module.h"

namespace ze::ui
{

class IMGUI_API ImGuiModule : public ze::module::Module
{
public:
	ImGuiModule();
	~ImGuiModule();
};

struct SImGuiAutoStyleColor
{
	SImGuiAutoStyleColor(ImGuiCol InIdx, const ImVec4& InColor)
	{
		ImGui::PushStyleColor(InIdx, InColor);
	}

	~SImGuiAutoStyleColor()
	{
		ImGui::PopStyleColor();
	}
};

struct SImGuiAutoStyleVar
{
	SImGuiAutoStyleVar(ImGuiStyleVar InIdx, const ImVec2& InVal)
	{
		ImGui::PushStyleVar(InIdx, InVal);
	}

	SImGuiAutoStyleVar(ImGuiStyleVar InIdx, const float& InVal)
	{
		ImGui::PushStyleVar(InIdx, InVal);
	}

	~SImGuiAutoStyleVar()
	{
		ImGui::PopStyleVar();
	}
};
}