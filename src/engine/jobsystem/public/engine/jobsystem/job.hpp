#pragma once

#include <atomic>
#include <array>
#include <memory>
#include <type_traits>
#include "engine/util/simple_pool.hpp"

namespace ze::jobsystem
{

static constexpr size_t job_alignement = std::hardware_destructive_interference_size;

enum class JobType
{
	/**
	 * The default job type,
	 * Jobs of this type will always be queue to a worker thread to not block any execution
	 */
	Normal,

	/**
	 * Mark this job has lightweight
	 * Allow this job to be directly executed
	 * If the job has any dependencies, it will also be directly executed instead to be enqueued
	 */
	Lightweight,
};

enum class JobPriority
{
	High,
	Normal,
	Low,
};

#pragma warning(disable: 4324)

/**
 * A job, must be constructed using new_job functions since jobs must not be allocated on the stack
 * new_job provide a safe way of constructing job while performant using a simple pool
 */
class alignas(job_alignement) Job
{
public:
	using Function = void(*)(Job& job);
	static constexpr size_t userdata_size = 128;
	static constexpr size_t max_continuations = 16;
	static constexpr size_t max_childs = 255;

	Job(Function in_function, JobType in_type, const JobPriority in_priority = JobPriority::Normal)
		: type(in_type), parent(nullptr), function(in_function), priority(in_priority), unfinished_jobs(1) {}

	template<typename UserDataType, typename... UserDataArgs>
		requires (std::is_standard_layout_v<UserDataType> && sizeof(UserDataType) <= userdata_size)
	Job(Function in_function, JobType in_type, const JobPriority in_priority = JobPriority::Normal, UserDataArgs&&... in_args)
		: type(in_type), parent(nullptr), function(in_function), priority(in_priority), unfinished_jobs(1)
	{
		new (get_userdata<UserDataType>()) UserDataType(std::forward<UserDataArgs>(in_args)...);
	}

	template<typename Lambda>
	Job(Lambda in_lambda, JobType in_type, const JobPriority in_priority) :
		type(in_type), parent(nullptr), function(nullptr), priority(in_priority), unfinished_jobs(1)
	{
		function = [](Job& job)
		{
			const Lambda& lambda = *job.get_userdata<Lambda>();
			lambda(job);
			lambda.~Lambda();
		};
		new (userdata.data()) Lambda(std::forward<Lambda>(in_lambda));
	}

	/** Child jobs ctor */
	Job(Function in_function, Job* in_parent, JobType in_type, const JobPriority in_priority = JobPriority::Normal)
		: type(in_type), parent(in_parent), function(in_function), priority(in_priority), unfinished_jobs(1)
	{
		++in_parent->unfinished_jobs;
	}

	template<typename UserDataType, typename... UserDataArgs>
		requires (std::is_standard_layout_v<UserDataType> && sizeof(UserDataType) <= userdata_size)
	Job(Function in_function, Job* in_parent, JobType in_type, const JobPriority in_priority = JobPriority::Normal, UserDataArgs&&... in_args)
		: type(in_type), parent(in_parent), function(in_function), priority(in_priority), unfinished_jobs(1)
	{
		++in_parent->unfinished_jobs;
		new (get_userdata<UserDataType>()) UserDataType(std::forward<UserDataArgs>(in_args)...);
	}

	template<typename Lambda>
	Job(Lambda in_lambda, Job* in_parent, JobType in_type, const JobPriority in_priority) :
		type(in_type), parent(in_parent), function(nullptr), priority(in_priority), unfinished_jobs(1)
	{
		++in_parent->unfinished_jobs;
		function = [](Job& job)
		{
			const Lambda& lambda = *job.get_userdata<Lambda>();
			lambda(job);
			lambda.~Lambda();
		};
		new (userdata.data()) Lambda(std::forward<Lambda>(in_lambda));
	}
public:
	void execute();
	void schedule();
	void continuate(Job* continuation);
	void wait() const;

	template<typename T>
	[[nodiscard]] const T* get_userdata() const
	{
		return reinterpret_cast<const T*>(userdata.data());
	}

	JobPriority get_priority() const { return priority; }
	bool is_finished() const { return unfinished_jobs == 0; }
	bool is_running() const { return unfinished_jobs > 0; }
private:
	void finish();
private:
	JobType type;
	Job* parent;
	Function function;
	JobPriority priority;

	/** Unfinished job counts, accounting for childs. 0 = finished, 1 = not finished, > 1 not finished + childs not finished */
	std::atomic_uint8_t unfinished_jobs;
	std::atomic_uint8_t dependance_count;
	std::atomic_uint8_t continuation_count;
	std::array<Job*, max_continuations> continuations;
	std::array<uint8_t, userdata_size> userdata;
};

#pragma warning(default: 4324)

namespace detail
{

using JobPool = SimplePool<Job, 256, true>;
JobPool& get_job_pool();

}

inline Job* new_job(Job::Function in_function, JobType in_type, const JobPriority in_priority = JobPriority::Normal)
{
	return detail::get_job_pool().allocate(in_function, in_type, in_priority);
}

template<typename UserDataType, typename... UserDataArgs>
	requires (std::is_standard_layout_v<UserDataType> && sizeof(UserDataType) <= Job::userdata_size)
Job* new_job(Job::Function in_function, JobType in_type, const JobPriority in_priority = JobPriority::Normal, UserDataArgs&&... in_args)
{
	auto job = new_job(in_function, in_type, in_priority);
	new (job->get_userdata<UserDataType>()) UserDataType(std::forward<UserDataArgs>(in_args)...);
	return job;
}

template<typename Lambda>
Job* new_job(Lambda in_lambda, JobType in_type, const JobPriority in_priority = JobPriority::Normal)
{
	return detail::get_job_pool().allocate(in_lambda, in_type, in_priority);
}

inline Job* new_child_job(Job::Function in_function, Job* in_parent, JobType in_type, const JobPriority in_priority = JobPriority::Normal)
{
	return detail::get_job_pool().allocate(in_function, in_parent, in_type, in_priority);
}

template<typename UserDataType, typename... UserDataArgs>
	requires (std::is_standard_layout_v<UserDataType> && sizeof(UserDataType) <= Job::userdata_size)
Job* new_child_job(Job::Function in_function, Job* in_parent, JobType in_type, const JobPriority in_priority = JobPriority::Normal, UserDataArgs&&... in_args)
{
	auto job = new_child_job(in_function, in_parent, in_type, in_priority);
	new (job->get_userdata<UserDataType>()) UserDataType(std::forward<UserDataArgs>(in_args)...);
	return job;
}

template<typename Lambda>
Job* new_child_job(Lambda in_lambda, Job* in_parent, JobType in_type, const JobPriority in_priority = JobPriority::Normal)
{
	return detail::get_job_pool().allocate(in_lambda, in_parent, in_type, in_priority);
}


}
