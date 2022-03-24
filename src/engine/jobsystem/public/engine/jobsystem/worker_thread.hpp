#pragma once

#include <thread>
#include <mutex>
#include "concurrentqueue/concurrentqueue.h"

namespace ze::jobsystem
{

class Job;

class WorkerThread
{
public:
	WorkerThread(size_t in_index);
	~WorkerThread();

	WorkerThread(const WorkerThread&) = delete;
	WorkerThread& operator=(const WorkerThread&) = delete;

	WorkerThread(WorkerThread&& other) noexcept : index(other.index),
		active(other.active.load()),
		thread(std::move(other.thread)),
		high_job_queue(std::move(other.high_job_queue)),
		normal_job_queue(std::move(other.normal_job_queue)),
		low_job_queue(std::move(other.low_job_queue)),
		high_consumer_tokens(std::move(other.high_consumer_tokens)),
		normal_consumer_tokens(std::move(other.normal_consumer_tokens)),
		low_consumer_tokens(std::move(other.low_consumer_tokens)) {}

	bool flush_one();
	void enqueue(const Job* job);

	auto& get_high_consumer_tokens() { return high_consumer_tokens; }
	auto& get_normal_consumer_tokens() { return normal_consumer_tokens; }
	auto& get_low_consumer_tokens() { return low_consumer_tokens; }

	static std::condition_variable& get_global_sleep_var() { return global_sleep_var;  }
	static size_t get_current_worker_idx() { return current_worker_idx;  }
private:
	void run();
	const Job* try_get_or_steal_job();
	bool try_dequeue(const Job*& job);
private:
	size_t index;
	std::atomic_bool active;
	std::thread thread;
	moodycamel::ConcurrentQueue<const Job*> high_job_queue;
	moodycamel::ConcurrentQueue<const Job*> normal_job_queue;
	moodycamel::ConcurrentQueue<const Job*> low_job_queue;
	std::vector<moodycamel::ConsumerToken> high_consumer_tokens;
	std::vector<moodycamel::ConsumerToken> normal_consumer_tokens;
	std::vector<moodycamel::ConsumerToken> low_consumer_tokens;
	std::mutex sleep_mutex;

	inline static std::condition_variable global_sleep_var;
	inline static thread_local size_t current_worker_idx = std::numeric_limits<size_t>::max();
};

}
