ze_add_module(filesystem
	public/engine/filesystem/filesystem.hpp
	public/engine/filesystem/mount_point.hpp
	public/engine/filesystem/std_mount_point.hpp
	public/engine/filesystem/filesystem_module.hpp
	private/engine/filesystem/filesystem.cpp
	private/engine/filesystem/std_mount_point.cpp
	private/engine/filesystem/filesystem_module.cpp)
target_include_directories(filesystem PUBLIC public PRIVATE private)
target_link_libraries(filesystem PUBLIC core gfx)