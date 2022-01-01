find_package(TBB CONFIG REQUIRED)
ze_add_module(jobsystem
	public/engine/jobsystem/job.hpp
	public/engine/jobsystem/job_group.hpp
	public/engine/jobsystem/worker_thread.hpp
	public/engine/jobsystem/jobsystem.hpp
	private/engine/jobsystem/jobsystem.cpp
	private/engine/jobsystem/job.cpp
	private/engine/jobsystem/worker_thread.cpp)
target_include_directories(jobsystem PUBLIC public PRIVATE private)
target_link_libraries(jobsystem PUBLIC core TBB::tbb)