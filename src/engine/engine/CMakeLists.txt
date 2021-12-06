ze_add_module(engine public/engine/placeholder.cpp)
target_include_directories(engine PUBLIC public PRIVATE private)
target_link_libraries(engine PUBLIC core)