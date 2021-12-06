find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_executable(test_core sparse_array.cpp)
target_link_libraries(test_core PRIVATE core GTest::gtest_main)
set_target_properties(test_core 
	PROPERTIES 
		RUNTIME_OUTPUT_DIRECTORY "${ZE_BIN_DIR}")
gtest_discover_tests(test_core)