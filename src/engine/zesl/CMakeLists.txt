if(false)
find_package(ShaderWriter REQUIRED)

file(GLOB ZE_COMPILER_HLSL_SRCS private/engine/zesl/CompilerHlsl/*.cpp private/engine/zesl/CompilerHlsl/*.hpp)

ze_add_module(zesl
	${ZE_COMPILER_HLSL_SRCS}
	public/engine/zesl/zesl.hpp
	public/engine/zesl/parameter.hpp
	private/engine/zesl/parser.hpp
	private/engine/zesl/token.hpp
	private/engine/zesl/ast/attribute.hpp
	private/engine/zesl/token.cpp
	private/engine/zesl/parser.cpp
	private/engine/zesl/zesl.cpp)
target_include_directories(zesl PUBLIC public PRIVATE private)
target_link_libraries(zesl PUBLIC core gfx sdw::ShaderAST)
endif()