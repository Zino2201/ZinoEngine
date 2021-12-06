cmake_minimum_required(VERSION 3.20)

project(ZinoEngine CXX)

# Remove exceptions (enabled by default by CMake)
string(REGEX REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(ZE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ZE_BIN_DIR ${ZE_ROOT_DIR}/bin/${CMAKE_BUILD_TYPE})
set(ZE_SRC_DIR ${ZE_ROOT_DIR}/src)
set(ZE_THIRD_PARTY_DIR ${ZE_SRC_DIR}/thirdparty)
set(ZE_MODULE_PREFIX "ze-")

# Options
option(ZE_WITH_VULKAN "Build with Vulkan support (requires Vulkan SDK)" ON)
option(ZE_MONOLITHIC "Monolithic mode (statc libs)" OFF)
option(ZE_WITH_TESTS "With Tests" ON)
option(ZE_WITH_SANITIZERS "With Sanitizers" ON)

message(STATUS "With Vulkan: ${ZE_WITH_VULKAN}")
message(STATUS "Monolithic: ${ZE_MONOLITHIC}")
message(STATUS "With Tests: ${ZE_WITH_TESTS}")
message(STATUS "With Sanitizers: ${ZE_WITH_SANITIZERS}")

add_library(AllModules INTERFACE)

macro(ze_add_module TARGET)
    if(ZE_MONOLITHIC)
        add_library(${TARGET} STATIC ${ARGN})
    else()
        add_library(${TARGET} SHARED ${ARGN})
    endif()

    set_target_properties(${TARGET} PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS TRUE
        LIBRARY_OUTPUT_DIRECTORY "${ZE_BIN_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${ZE_BIN_DIR}"
        PREFIX "${ZE_MODULE_PREFIX}"
        UNITY_BUILD ON
        DEFINE_SYMBOL "")

    if(ZE_WITH_SANITIZERS)
        target_compile_options(${TARGET} PRIVATE /fsanitize=address)
        target_compile_definitions(${TARGET} PRIVATE ZE_HAS_ADDRESS_SANITIZER=1)
    endif()

    target_link_libraries(AllModules INTERFACE ${TARGET})
endmacro()

if(ZE_WITH_TESTS)
	enable_testing()
endif()
add_subdirectory(src)