find_package(robin_hood CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS locale)

ze_add_module(core
	public/core.natvis
	public/engine/random.hpp
	public/engine/core.hpp
	public/engine/flags.hpp
	public/engine/platform_macros.hpp
	public/engine/hash.hpp
	public/engine/multicast_delegate.hpp
	public/engine/result.hpp
	public/engine/debug/assertions.hpp
	public/engine/logger/logger.hpp
	public/engine/logger/sink.hpp
	public/engine/containers/sparse_array.hpp
	public/engine/logger/sinks/stdout_sink.hpp
	public/engine/module/module.hpp
	public/engine/module/module_manager.hpp
	public/engine/util/simple_pool.hpp
	public/engine/hal/library.hpp
	public/engine/hal/thread.hpp
	private/engine/logger/logger.cpp
	private/engine/logger/sinks/stdout_sink.cpp
	private/engine/module/module_manager.cpp
	private/engine/module/module.cpp
	private/engine/core.cpp)

if(WIN32)
	target_sources(core PRIVATE
		public/engine/hal/windows/library.hpp
		public/engine/hal/windows/thread.hpp
		private/engine/hal/windows/library.cpp
		private/engine/hal/windows/thread.cpp)
endif()

target_include_directories(core PUBLIC public PRIVATE private)
target_compile_options(core PUBLIC /GR- /W4)
target_compile_features(core PUBLIC cxx_std_20)
target_compile_definitions(core PUBLIC FMT_EXCEPTIONS=0 _HAS_EXCEPTIONS=0 TBB_USE_EXCEPTIONS=0 ZE_MODULE_PREFIX="${ZE_MODULE_PREFIX}" 
	GLM_FORCE_LEFT_HANDED GLM_FORCE_RADIANS GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_INTRINSICS NOMINMAX UNICODE D_UNICODE)
if(ZE_MONOLITHIC)
	target_compile_definitions(core PUBLIC ZE_MONOLITHIC=1)
else()
	target_compile_definitions(core PUBLIC ZE_MONOLITHIC=0)
endif()

if(ZE_WITH_SANITIZERS)
	target_compile_definitions(core PUBLIC BOOST_USE_ASAN=1)
endif()

if(ZE_WITH_PROFILING)
	find_package(Tracy REQUIRED)
	find_package(Threads REQUIRED)
	target_link_libraries(core PUBLIC Tracy::TracyClient)
	target_compile_definitions(core PUBLIC ZE_HAS_PROFILING=1 TRACY_ENABLE)
endif()

target_compile_definitions(core PUBLIC "$<$<CONFIG:Debug>:ZE_BUILD_TYPE_DEBUG=1>$<$<CONFIG:RelWithDebInfo>:ZE_BUILD_TYPE_RELWITHDEBINFO=1>$<$<CONFIG:Release>:ZE_BUILD_TYPE_RELEASE=1>")
if(CMAKE_BUILD_TYPE MATCHES "Debug")
	target_compile_definitions(core PUBLIC "TBB_USE_DEBUG=1")
endif()
target_link_libraries(core PUBLIC robin_hood::robin_hood glm::glm fmt::fmt-header-only Boost::locale)