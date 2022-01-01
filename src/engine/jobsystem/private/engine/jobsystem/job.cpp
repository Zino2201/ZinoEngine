#include "engine/jobsystem/job.hpp"
#include <deque>
#include "engine/util/simple_pool.hpp"

namespace ze::jobsystem
{

namespace detail
{

JobPool global_job_pool;

JobPool& get_job_pool()
{
	return global_job_pool;
}

}

void Job::schedule()
{
	if (type == JobType::Normal)
	{
		get_current_or_random_worker().enqueue(this);
		WorkerThread::get_global_sleep_var().notify_all();
	}
	else
	{
		execute();
	}
}

void Job::continuate(Job* continuation)
{
	ZE_ASSERT(continuation_count + 1 < Job::max_continuations);
	continuations[continuation_count++] = continuation;
}

void Job::wait() const
{
	while(!is_finished())
	{
		WorkerThread::get_global_sleep_var().notify_one();
		get_current_or_random_worker().flush_one();
	}
}

void Job::execute()
{
	function(*this);
	finish();
	detail::get_job_pool().free(this);
}

void Job::finish()
{
	--unfinished_jobs;

	if (is_finished())
	{
		/** Tell the parent that we finished */
		if (parent)
		{
			parent->finish();
		}

		/** Run dependents */
		for (uint8_t i = 0; i < continuation_count; ++i)
		{
			if (Job* continuation = continuations[i])
			{
				continuation->schedule();
			}
		}
	}
}

}
