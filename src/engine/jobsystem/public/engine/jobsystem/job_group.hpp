#pragma once

#include "job.hpp"

namespace ze::jobsystem
{

class JobGroup
{
public:
	void add(jobsystem::Job* in_job)
	{
		jobs.emplace_back(in_job);
	}

	void schedule()
	{
		for (const auto& job : jobs)
			job->schedule();
	}

	void wait()
	{
		for(const auto& job : jobs)
			job->wait();
	}

	void schedule_and_wait()
	{
		schedule();
		wait();
	}
private:
	std::vector<jobsystem::Job*> jobs;
};

}