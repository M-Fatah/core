#pragma once

#include "core/export.h"
#include "core/defines.h"
#include "core/containers/slice.h"

/*
TODO:
- [ ] Keep heavier scheduler ideas parked until there is clear demand: priorities, fibers/sysmon, cancellation, dependency graph.
*/

struct Scheduler_Desc
{
	U32 worker_count;
	U32 replacement_worker_count;
	U32 initial_task_queue_capacity;
	const char *worker_thread_name;
};

struct Scheduler;
struct Scheduler_Group;

struct Scheduler_Task
{
	void (*function)(void *data);
	void *data;
};

struct Scheduler_Parallel_For_Desc
{
	U32 count;
	U32 chunk_size;
	void (*function)(U32 begin, U32 end, void *data);
	void *data;
};

struct Scheduler_Stats
{
	U32 worker_count;
	U32 replacement_worker_count;
	U32 active_replacement_worker_count;
	U32 blocked_worker_count;
	U32 active_task_count;
	U32 queued_task_count;
	U32 live_group_count;
};

CORE_API Scheduler *
scheduler_init(Scheduler_Desc desc);

CORE_API void
scheduler_deinit(Scheduler *self);

CORE_API Scheduler_Group *
scheduler_group_init(Scheduler *self);

CORE_API void
scheduler_group_deinit(Scheduler *self, Scheduler_Group *group);

CORE_API void
scheduler_submit(Scheduler *self, Slice<const Scheduler_Task> tasks, Scheduler_Group *group = nullptr);

CORE_API void
scheduler_wait_group(Scheduler *self, Scheduler_Group *group);

CORE_API void
scheduler_wait_all(Scheduler *self);

CORE_API void
scheduler_worker_block_ahead(Scheduler *self);

CORE_API void
scheduler_worker_block_clear(Scheduler *self);

CORE_API U32
scheduler_get_current_worker_index(Scheduler *self);

CORE_API Scheduler_Stats
scheduler_get_stats(Scheduler *self);

CORE_API void
scheduler_parallel_for(Scheduler *self, Scheduler_Parallel_For_Desc desc);