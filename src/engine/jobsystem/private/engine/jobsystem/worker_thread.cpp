#include "engine/jobsystem/worker_thread.hpp"
#include "engine/jobsystem/jobsystem.hpp"
#include "engine/hal/thread.hpp"
#include <random>
#include "fmt/format.h"

namespace ze::jobsystem
{

WorkerThread::WorkerThread(size_t in_index)
	: index(in_index),
	active(true),
	thread([&] { run(); })
{

}

void WorkerThread::run()
{
	current_worker_idx = index;
	hal::set_thread_name(thread.get_id(), fmt::format("Worker Thread {}", index));

	while (active)
	{
		if (!flush_one())
		{
			std::unique_lock lock(sleep_mutex);
			global_sleep_var.wait(lock);
		}
	}
}

bool WorkerThread::flush_one()
{
	if (Job* job = const_cast<Job*>(try_get_or_steal_job()))
	{
		job->execute();
		return true;
	}

	return false;
}

void WorkerThread::enqueue(const Job* job)
{
	job_queue.push(job);
}

const Job* WorkerThread::try_get_or_steal_job()
{
	const Job* job = nullptr;
	if(!job_queue.try_pop(job))
	{
		std::random_device device;
		std::mt19937 gen(device());
		std::uniform_int_distribution<size_t> distribution(0, get_worker_count() - 1);

		const size_t worker_idx = distribution(gen);
		auto& worker_to_steal = get_worker_by_idx(worker_idx);

		if (this != &worker_to_steal)
		{
			const Job* stolen_job = nullptr;
			if(worker_to_steal.job_queue.try_pop(stolen_job))
				return stolen_job;
		}

		std::this_thread::yield();
		return nullptr;
	}

	return job;
}

}
