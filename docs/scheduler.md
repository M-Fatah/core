# Scheduler

**Header:** `core/scheduler.h`

Long-lived worker-thread scheduler built on the platform thread, mutex, and condition-variable primitives.

---

## Lifetime

```cpp
#include <core/defer.h>
#include <core/scheduler.h>

void run_application()
{
	Scheduler *scheduler = scheduler_init(Scheduler_Desc {
		.worker_count = 4,
		.replacement_worker_count = 1,
		.initial_task_queue_capacity = 1024,
		.worker_thread_name = "GameWorker"
	});
	DEFER(scheduler_deinit(scheduler));

	// Main loop, compiler driver, asset pipeline, or tool lifetime.
}
```

`scheduler_init` starts the requested worker threads and returns only after every worker has entered its wait state. Initialize one long-lived scheduler near application, compiler, tool, or game-engine startup and keep it alive until shutdown.

`scheduler_deinit` requests shutdown, wakes all workers, joins every worker thread, and releases scheduler-owned synchronization objects. Workers drain tasks that were already submitted before shutdown; new submissions are rejected after shutdown starts.

All `Scheduler_Group` objects created from the scheduler must be deinitialized before `scheduler_deinit`. A scheduler worker also cannot deinitialize its own scheduler, because that would require the thread to join itself.

`worker_count` must be greater than 0.

`replacement_worker_count` is optional standby worker count for blocking replacement. Replacement workers are owned by the scheduler, but they only execute work while workers counted by `worker_count` have active blocking markers from `scheduler_worker_block_ahead`. Leave it as `0` when all scheduler tasks are expected to stay CPU-bound.

`initial_task_queue_capacity` is optional. When greater than 0, the scheduler distributes that capacity across worker task queues during initialization so normal task submission can avoid the first queue allocation.

`worker_thread_name` is optional. When omitted, scheduler worker threads use `"Scheduler"` as their platform thread name.

---

## Worker Queries

```cpp
Scheduler_Stats stats = scheduler_get_stats(scheduler);
U32 worker_thread_count = stats.worker_count + stats.replacement_worker_count;
U32 worker_index = scheduler_get_current_worker_index(scheduler);
```

`scheduler_get_stats` returns a locked snapshot of scheduler counters: worker count, replacement worker count, active replacement worker count, blocked worker count, active task count, queued task count, and live group count. Use it for debug UI, profiling, tests, and future scheduler policy decisions.

`stats.worker_count` is the number of primary worker threads. `stats.replacement_worker_count` is the number of standby replacement workers created at init time. `stats.worker_count + stats.replacement_worker_count` is the total number of worker threads owned by the scheduler.

`scheduler_get_current_worker_index` returns the calling worker's index inside that scheduler. It returns `U32_MAX` when called from a non-worker thread or from a worker owned by a different scheduler.

This is useful for indexing caller-owned per-worker scratch buffers, compiler state, profiling counters, or game-system temporary state without adding locks.

---

## Blocking Work

Workers can mark that they are about to enter blocking work:

```cpp
struct Read_File_Task
{
	Scheduler *scheduler;
	const char *path;
	Memory_Block buffer;
	U64 bytes_read;
};

void read_file_task(void *data)
{
	Read_File_Task *task = (Read_File_Task *)data;

	scheduler_worker_block_ahead(task->scheduler);
	DEFER(scheduler_worker_block_clear(task->scheduler));

	task->bytes_read = platform_file_read(task->path, task->buffer);
}
```

`scheduler_worker_block_ahead` and `scheduler_worker_block_clear` must be called by a worker owned by that scheduler. Blocking markers are nested; every `scheduler_worker_block_ahead` must be matched by one `scheduler_worker_block_clear`.

`stats.blocked_worker_count` reports how many workers currently have at least one active blocking marker. When replacement workers were configured at init time, the scheduler activates up to one replacement worker per blocked worker, capped by `replacement_worker_count`; `stats.active_replacement_worker_count` reports how many replacement workers are currently active.

---

## Tasks

```cpp
void task_entry(void *data)
{
	// Do work.
}

scheduler_submit(scheduler, Scheduler_Task {
	.function = task_entry,
	.data = user_data
});

scheduler_wait_all(scheduler);
```

Submitted tasks are distributed round-robin across the `worker_count` worker queues. Workers take tasks from the front of their own queue first. When their own queue is empty, they steal from the back of another `worker_count` worker queue. Replacement workers do not receive direct submissions; when active, they only steal from those queues. Tasks execute outside the scheduler mutex.

Task data must remain valid until the task has executed.

`scheduler_wait_all` blocks until there are no queued tasks and no worker is currently executing a task.

Call `scheduler_wait_all` from the thread coordinating the scheduler. Scheduler workers cannot wait for all work because the running worker task is part of the active task count.

Multiple tasks can be submitted as a batch:

```cpp
Scheduler_Task tasks[] = {
	{
		.function = task_entry,
		.data = first_user_data
	},
	{
		.function = task_entry,
		.data = second_user_data
	}
};

scheduler_submit(scheduler, span_init(tasks));
```

Batch submission reserves space in each target worker queue, queues all task descriptors under one scheduler lock, and wakes workers once after the batch is queued. It copies `Scheduler_Task` values into scheduler-owned queues; task data must still remain valid until the matching task has executed.

---

## Parallel For

```cpp
struct Mesh_Build_Context
{
	Mesh *meshes;
};

void build_meshes(U32 begin, U32 end, void *data)
{
	Mesh_Build_Context *context = (Mesh_Build_Context *)data;
	for (U32 i = begin; i < end; ++i)
	{
		build_mesh(context->meshes[i]);
	}
}

Mesh_Build_Context context = {
	.meshes = meshes
};

scheduler_parallel_for(scheduler, Scheduler_Parallel_For_Desc {
	.count = mesh_count,
	.chunk_size = 64,
	.function = build_meshes,
	.data = &context
});
```

`scheduler_parallel_for` splits `[0, count)` into half-open chunk ranges and submits one task per chunk. It waits for all chunks before returning.

`chunk_size` controls task granularity. Smaller chunks can balance uneven work better, while larger chunks reduce scheduling overhead. Leave it as `0` to let the scheduler choose a chunk size from `worker_count`.

The callback data must remain valid until `scheduler_parallel_for` returns.

The helper uses the calling thread's temp allocator for its internal chunk task storage and resets that temporary storage before returning.

---

## Groups

```cpp
Scheduler_Group *group = scheduler_group_init(scheduler);
DEFER(scheduler_group_deinit(scheduler, group));

scheduler_submit(scheduler, Scheduler_Task {
	.function = task_entry,
	.data = first_user_data
}, group);

scheduler_submit(scheduler, Scheduler_Task {
	.function = task_entry,
	.data = second_user_data
}, group);

Scheduler_Task more_tasks[] = {
	{
		.function = task_entry,
		.data = third_user_data
	},
	{
		.function = task_entry,
		.data = fourth_user_data
	}
};

scheduler_submit(scheduler, span_init(more_tasks), group);

scheduler_wait_group(scheduler, group);
```

Groups track completion for a specific batch of tasks. `scheduler_wait_group` blocks until every task submitted with that group has finished, but it does not wait for unrelated scheduler work.

A group belongs to the scheduler passed to `scheduler_group_init`. Deinit the group after its pending task count reaches 0 and before deinitializing the scheduler. The scheduler tracks live groups and validates that none are alive during `scheduler_deinit`.

When `scheduler_wait_group` is called from a worker owned by the same scheduler, that worker executes tasks from its own queue first and can steal from other worker queues while it waits. This lets a task submit child tasks to a group and wait for them without putting the worker to sleep.

The scheduler validates that a task does not wait for its own group.

---

## Cleanup

Later, after the scheduler shape settles, consider adding a `Queue<T>` convenience wrapper over `Ring_Buffer<T>` for FIFO-only call sites. This is a readability cleanup, not a required data-structure change.