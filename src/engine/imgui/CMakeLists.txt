find_package(freetype CONFIG REQUIRED)

ze_add_module(imgui
	private/engine/imgui/imgui.cpp
	public/engine/imgui/imgui.hpp
	${IMGUI_ROOT_DIR}/imgui.cpp
	${IMGUI_ROOT_DIR}/imgui_draw.cpp
	${IMGUI_ROOT_DIR}/imgui_demo.cpp
	${IMGUI_ROOT_DIR}/imgui_tables.cpp
	${IMGUI_ROOT_DIR}/imgui_widgets.cpp
	${IMGUI_ROOT_DIR}/misc/freetype/imgui_freetype.h
	${IMGUI_ROOT_DIR}/misc/freetype/imgui_freetype.cpp)
target_include_directories(imgui PUBLIC public ${IMGUI_ROOT_DIR} PRIVATE private)
target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_FREETYPE)
target_link_libraries(imgui PUBLIC core gfx PRIVATE application freetype shadersystem)