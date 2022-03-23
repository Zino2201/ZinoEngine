#include "engine/core.hpp"
#include "engine/jobsystem/job.hpp"
#include "engine/jobsystem/jobsystem.hpp"
#include <random>
#include "engine/jobsystem/worker_thread.hpp"

namespace ze::jobsystem
{

ZE_DEFINE_LOG_CATEGORY(jobsystem);

std::vector<WorkerThread> worker_threads;

void initialize()
{
	const uint32_t num_cores = std::thread::hardware_concurrency();
	logger::info(log_jobsystem, "{} cores detected, spawning {} workers",
		num_cores, num_cores - 1);
	worker_threads.reserve(num_cores);
	for(size_t i = 0; i < num_cores - 1; ++i)
	{
		worker_threads.emplace_back(i);
	}
}

void shutdown()
{
	worker_threads.clear();
	WorkerThread::get_global_sleep_var().notify_all();
}

size_t get_worker_count()
{
	return worker_threads.size();
}

WorkerThread& get_worker_by_idx(size_t in_index)
{
	return worker_threads[in_index];
}

WorkerThread& get_current_or_random_worker()
{
	if (WorkerThread::get_current_worker_idx() != -1)
		return get_worker_by_idx(WorkerThread::get_current_worker_idx());

	std::random_device device;
	std::mt19937 gen(device());
	std::uniform_int_distribution<size_t> distribution(0, get_worker_count() - 1);
	return get_worker_by_idx(distribution(gen));
}

}