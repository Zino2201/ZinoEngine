#pragma once

#include <thread>
#include "tbb/concurrent_priority_queue.h"

namespace ze::jobsystem
{

class Job;

class WorkerThread
{
	struct JobCompare
	{
		bool operator()(const Job* left, const Job* right) const
		{
			return left->get_priority() < right->get_priority();
		}
	};

public:
	WorkerThread(size_t in_index);

	WorkerThread(const WorkerThread&) = delete;
	WorkerThread& operator=(const WorkerThread&) = delete;

	WorkerThread(WorkerThread&& other) noexcept : index(other.index),
		active(other.active.load()),
		thread(std::move(other.thread)),
		job_queue(std::move(other.job_queue)) {}

	bool flush_one();
	void enqueue(const Job* job);

	static std::condition_variable& get_global_sleep_var() { return global_sleep_var;  }
	static size_t get_current_worker_idx() { return current_worker_idx;  }
private:
	void run();
	const Job* try_get_or_steal_job();
private:
	size_t index;
	std::atomic_bool active;
	std::thread thread;
	tbb::concurrent_priority_queue<const Job*, JobCompare> job_queue;
	std::mutex sleep_mutex;
	inline static std::condition_variable global_sleep_var;
	inline static thread_local size_t current_worker_idx = std::numeric_limits<size_t>::max();
};

}
