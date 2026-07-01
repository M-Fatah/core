#pragma once

#include "core/export.h"
#include "core/defines.h"
#include "core/containers/span.h"

using Scheduler_Parallel_For_Function = void (*)(U32 begin, U32 end, void *data);

struct Scheduler_Parallel_For_Desc
{
	U32 count;
	U32 chunk_size;
	Scheduler_Parallel_For_Function function;
	void *data;
};

using Scheduler_Task_Function = void (*)(void *);

struct Scheduler_Task
{
	Scheduler_Task_Function function;
	void *data;
};

struct Scheduler_Group;

struct Scheduler_Desc
{
	U32 worker_count;
	U32 initial_task_queue_capacity;
};

struct Scheduler;

CORE_API Scheduler *
scheduler_init(Scheduler_Desc desc);

CORE_API void
scheduler_deinit(Scheduler *self);

CORE_API Scheduler_Group *
scheduler_group_init(Scheduler *self);

CORE_API void
scheduler_group_deinit(Scheduler *self, Scheduler_Group *group);

CORE_API void
scheduler_submit(Scheduler *self, Scheduler_Task task);

CORE_API void
scheduler_submit(Scheduler *self, Span<const Scheduler_Task> tasks);

CORE_API void
scheduler_submit(Scheduler *self, Scheduler_Task task, Scheduler_Group *group);

CORE_API void
scheduler_submit(Scheduler *self, Span<const Scheduler_Task> tasks, Scheduler_Group *group);

CORE_API void
scheduler_wait_group(Scheduler *self, Scheduler_Group *group);

CORE_API void
scheduler_wait_all(Scheduler *self);

CORE_API void
scheduler_worker_block_ahead(Scheduler *self);

CORE_API void
scheduler_worker_block_clear(Scheduler *self);

CORE_API U32
scheduler_get_worker_count(Scheduler *self);

CORE_API U32
scheduler_get_blocked_worker_count(Scheduler *self);

CORE_API U32
scheduler_get_current_worker_index(Scheduler *self);

CORE_API void
scheduler_parallel_for(Scheduler *self, Scheduler_Parallel_For_Desc desc);