ze_add_module(application
		public/engine/application/application_module.hpp
	public/engine/application/platform_window.hpp
	public/engine/application/platform_application.hpp
	public/engine/application/message_handler.hpp
	private/engine/application/application_module.cpp)

if(WIN32)
	target_sources(application PRIVATE
		public/engine/application/windows/application.hpp
		public/engine/application/windows/window.hpp
		private/engine/application/windows/application.cpp
		private/engine/application/windows/window.cpp)
	target_link_libraries(application PRIVATE shcore)
endif()

target_include_directories(application PUBLIC public PRIVATE private)
target_link_libraries(application PUBLIC core)