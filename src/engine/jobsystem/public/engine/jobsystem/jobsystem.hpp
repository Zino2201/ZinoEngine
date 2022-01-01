#pragma once

namespace ze::jobsystem
{

class WorkerThread;

void initialize();
size_t get_worker_count();
WorkerThread& get_worker_by_idx(size_t in_index);
WorkerThread& get_current_or_random_worker();

}