#define UUID_SYSTEM_GENERATOR
#include "editor/windows/AssetExplorer.h"
#include "imgui/ImGui.h"
#include <imgui_internal.h>
#include <editor/assets/AssetFactory.h>
#include "gfx/Gfx.h"
#include "reflection/Class.h"
#include "editor/IconManager.h"
#include "editor/assets/AssetActions.h"
#include "editor/assetutils/AssetUtils.h"
#include "zefs/Paths.h"
#include "editor/ZEEditor.h"
#include "assets/Asset.h"
#include "PlatformMgr.h"

namespace ze::editor
{

static constexpr int max_hierarchy_width = 250.f;
static const ImVec2 entry_size = ImVec2(90, 120);

AssetExplorer::AssetExplorer() : Window("Asset Explorer", 
	WindowFlags(),
	ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse),
	current_path("Assets")
{

}

void AssetExplorer::draw()
{
	draw_project_hierarchy();
	ImGui::SameLine();
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
	ImGui::SameLine();
	draw_asset_list();
}

void AssetExplorer::draw_project_hierarchy()
{
	ImGui::BeginChild("Project Hierarchy", ImVec2(max_hierarchy_width, ImGui::GetContentRegionAvail().y));
	if(ImGui::TreeNodeEx("##Game", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow |
		ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_SpanAvailWidth))
	{
		Icon* icon = get_icon("icon-asset-directory");
		ImGui::SameLine(0, 5);
		ImGui::Image((void*) &icon->texture_view, ImVec2(16, 16));
		ImGui::SameLine(0, 5);
		ImGui::TextUnformatted("Game");

		if(ImGui::IsItemClicked())
		{
			set_current_path("Assets");
		}

		draw_project_hierarchy_tree("Assets");
		ImGui::TreePop();
	}
	ImGui::EndChild();
}

void AssetExplorer::draw_project_hierarchy_tree(const std::filesystem::path& in_root)
{
	for(const auto& entry : assetdatabase::get_subdirectories(in_root))
	{
		if(ImGui::TreeNodeEx(std::string("##" + entry.string()).c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_SpanAvailWidth))
		{
			if(ImGui::IsItemClicked())
			{
				set_current_path(in_root / entry);
			}

			Icon* icon = get_icon("icon-asset-directory");
			ImGui::SameLine(0, 5);
			ImGui::Image((void*) &icon->texture_view, ImVec2(16, 16));
			ImGui::SameLine(0, 5);
			ImGui::TextUnformatted(entry.string().c_str());
			draw_project_hierarchy_tree(in_root / entry);
			ImGui::TreePop();
		}
	}
}

void AssetExplorer::draw_asset_list()
{
	ImGui::BeginChild("Asset List", ImGui::GetContentRegionAvail());

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
	if(ImGui::BeginPopupContextWindow())
	{
		ImGui::TextUnformatted("Asset actions");
		ImGui::Separator();
		if(ImGui::BeginMenu("Create new asset"))
		{
			for(const auto& factory : get_factories())
			{
				if (factory->can_instantiated() &&
					ImGui::MenuItem(factory->get_name().c_str()))
				{
					OwnerPtr<Asset> asset = factory->instantiate();

					/** Serialize the asset and then unload it */
					const ze::reflection::Class* asset_class = asset->get_class();
					std::filesystem::path path = std::filesystem::path(
						ze::filesystem::get_current_working_dir()) / current_path;

					/** Compute path */
					{
						std::string name = "NewAsset";
						while (std::filesystem::exists(path / name))
							name += "(1)";
						name += "." + factory->get_asset_file_extension();
						asset->set_path(path / name);
					}

					/** Set asset metadata file */
					{
						AssetMetadata metadata;
						metadata.uuid = uuids::uuid_system_generator{}();
						metadata.engine_version = get_version();
						metadata.asset_class = asset_class;
						asset->set_metadata(metadata);
					}

					assetmanager::save_asset(assetmanager::AssetSaveInfo(asset, get_running_platform()));
					delete asset;

					/** Notify the database */
					assetdatabase::scan(current_path);
				}
			}
			ImGui::EndMenu();
		}

		if(ImGui::MenuItem("Import here"))
		{
			assetutils::import_assets_dialog(filesystem::get_current_working_dir() / current_path,
				current_path);
		}

		ImGui::EndPopup();
	}
	ImGui::PopStyleVar();

	ImGui::Columns(std::max<int>(1, ImGui::GetContentRegionAvail().x / entry_size.x), nullptr, false);
	
	for(const auto& entry : assetdatabase::get_subdirectories(current_path))
	{
		draw_asset_entry(entry.filename().string(), DirectoryEntry { current_path / entry });
		ImGui::NextColumn();
	}
	
	for(const auto& entry : assetdatabase::get_assets(current_path))
	{
		draw_asset_entry(entry.name, entry);
		ImGui::NextColumn();
	}
	ImGui::Columns(1);
	ImGui::EndChild();
}

void AssetExplorer::draw_asset_entry(const std::string& in_name, 
	const std::variant<assetdatabase::AssetPrimitiveData, DirectoryEntry>& in_data)
{
	bool is_asset = in_data.index() == 0;

	/** Asset button & image */
	{
		ImVec2 cursor = ImGui::GetCursorPos();

		ImGui::Selectable(std::string("##" + in_name).c_str(), false, ImGuiSelectableFlags_None, entry_size);

		/** Asset properties */
		if(is_asset && ImGui::IsItemHovered())
		{
			const assetdatabase::AssetPrimitiveData& asset_data = std::get<0>(in_data);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
			ImGui::BeginTooltip();
			ImGui::Text("Type: %s", asset_data.metadata.asset_class->get_name().c_str());
			ImGui::Text("Disk size: %f KiB", asset_data.size / 1048576.f);
			ImGui::Separator();
			ImGui::Text("Path: %s", asset_data.path.string().c_str());
			ImGui::Text("Saved using ZE v%d.%d.%d", asset_data.metadata.engine_version.major,
				asset_data.metadata.engine_version.minor,
				asset_data.metadata.engine_version.patch);
			ImGui::EndTooltip();
			ImGui::PopStyleVar();
		}
		
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if(is_asset)
			{
				const assetdatabase::AssetPrimitiveData& asset_data = std::get<0>(in_data);
				if(AssetActions* actions = get_actions_for(asset_data.metadata.asset_class))
				{
					auto request = assetmanager::load_asset_sync(asset_data.path);
					std::string window_title_name = request.first->get_path().stem().string();
					
					/**
					 * Focus the already existing asset editor with this asset if it exists
					 */
					if(EditorApp::get().has_window(window_title_name))
					{
						ImGui::SetWindowFocus(window_title_name.c_str());
					}
					else
					{
						actions->open_editor(request.first, request.second);
					}
				}
			}
			else
			{
				const DirectoryEntry& dir_data = std::get<1>(in_data);
				set_current_path(dir_data.full_path);
			}
		}

		ImGui::SetCursorPos(cursor);
			
		ImVec2 thumbnail_size = ImVec2(64, 64);
		ImVec2 cursor_pos_offset = ImVec2(thumbnail_size.x / 8, 16);
		ImRect bb(ImGui::GetCursorScreenPos() + cursor_pos_offset, ImGui::GetCursorScreenPos() + cursor_pos_offset + thumbnail_size);
	/*	ImGui::GetWindowDrawList()->AddRectFilled(bb.Min, bb.Max,
			IM_COL32(255, 0, 0, 255));*/

		auto asset_icon = get_icon(is_asset ? "icon-asset-file" : "icon-asset-directory");
		ImGui::GetWindowDrawList()->AddImage(
			(void*) &asset_icon->texture_view.get(), 
			bb.Min + ImVec2(1, 1), 
			bb.Max - ImVec2(1, 1), 
			ImVec2(0, 0), 
			ImVec2(1, 1), 
			IM_COL32_WHITE);
	}

	/** Asset name text */
	{
		ImVec2 available_size = ImGui::GetContentRegionAvail();
		ImVec2 text_size = ImGui::CalcTextSize(in_name.c_str());
		ImGui::SetCursorPosY(entry_size.y - text_size.y);
		ImGui::TextUnformatted(in_name.c_str());
	}
}

void AssetExplorer::set_current_path(const std::filesystem::path& in_current_path)
{
	current_path = in_current_path;
}

}