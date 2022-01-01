add_executable(main main.cpp)
set_target_properties(main PROPERTIES OUTPUT_NAME ZETest)
set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${ZE_BIN_DIR}")
target_link_libraries(main PRIVATE core engine AllModules)