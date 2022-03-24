#include "engine/jobsystem/worker_thread.hpp"
#include "engine/jobsystem/job.hpp"
#include "engine/jobsystem/jobsystem.hpp"
#include "engine/hal/thread.hpp"
#include <random>
#include "fmt/format.h"
#include "engine/random.hpp"
#if ZE_FEATURE(PROFILING)
#include <Tracy.hpp>
#endif

namespace ze::jobsystem
{

WorkerThread::WorkerThread(size_t in_index)
	: index(in_index),
	active(true),
	thread([&] { run(); })
{
	for(size_t i = 0; i < std::thread::hardware_concurrency(); ++i)
	{
		high_consumer_tokens.emplace_back(high_job_queue);
		normal_consumer_tokens.emplace_back(normal_job_queue);
		low_consumer_tokens.emplace_back(low_job_queue);
	}
}

WorkerThread::~WorkerThread()
{
	active = false;
	thread.detach();
}

void WorkerThread::run()
{
	current_worker_idx = index;
	hal::set_thread_name(thread.get_id(), fmt::format("Worker Thread {}", index));
#if ZE_FEATURE(PROFILING)
	tracy::SetThreadName(fmt::format("Worker Thread {}", index).c_str());
#endif

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
	switch(job->get_priority())
	{
	case JobPriority::High:
		high_job_queue.enqueue(job);
		break;
	case JobPriority::Normal:
		normal_job_queue.enqueue(job);
		break;
	case JobPriority::Low:
		low_job_queue.enqueue(job);
		break;
	}
}

const Job* WorkerThread::try_get_or_steal_job()
{
	const Job* job = nullptr;
	if(!try_dequeue(job))
	{
		const size_t worker_idx = random_SM64<size_t>(0, get_worker_count() - 1);
		auto& worker_to_steal = get_worker_by_idx(worker_idx);

		if (this != &worker_to_steal)
		{
			const Job* stolen_job = nullptr;
			if(worker_to_steal.try_dequeue(stolen_job))
				return stolen_job;
		}

		std::this_thread::yield();
		return nullptr;
	}

	return job;
}

bool WorkerThread::try_dequeue(const Job*& job)
{
	if (high_job_queue.try_dequeue(high_consumer_tokens[index], job))
		return true;

	if (normal_job_queue.try_dequeue(normal_consumer_tokens[index], job))
		return true;

	if (low_job_queue.try_dequeue(low_consumer_tokens[index], job))
		return true;

	return false;
}

}
