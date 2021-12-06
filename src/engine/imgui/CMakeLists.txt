ze_add_module(imgui
	private/engine/imgui/imgui.cpp
	public/engine/imgui/imgui.hpp
	${IMGUI_ROOT_DIR}/imgui.cpp
	${IMGUI_ROOT_DIR}/imgui_draw.cpp
	${IMGUI_ROOT_DIR}/imgui_demo.cpp
	${IMGUI_ROOT_DIR}/imgui_tables.cpp
	${IMGUI_ROOT_DIR}/imgui_widgets.cpp)
target_include_directories(imgui PUBLIC public ${IMGUI_ROOT_DIR} PRIVATE private)
target_link_directories(imgui PRIVATE ${DXC_LIB_DIR})
target_link_libraries(imgui PUBLIC core gfx PRIVATE shadercompiler)