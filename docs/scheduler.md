# Scheduler

**Header:** `core/scheduler.h`

Long-lived worker-thread scheduler built on the platform thread, mutex, and condition-variable primitives.

---

## Lifetime

```cpp
#include <core/scheduler.h>

Scheduler *scheduler = scheduler_init(Scheduler_Desc {
	.worker_count = 4,
	.initial_task_queue_capacity = 1024
});

scheduler_deinit(scheduler);
```

`scheduler_init` starts the requested worker threads and returns only after every worker has entered its wait state.

`scheduler_deinit` requests shutdown, wakes all workers, joins every worker thread, and releases scheduler-owned synchronization objects. Workers drain tasks that were already submitted before shutdown; new submissions are rejected after shutdown starts.

All `Scheduler_Group` objects created from the scheduler must be deinitialized before `scheduler_deinit`. A scheduler worker also cannot deinitialize its own scheduler, because that would require the thread to join itself.

`worker_count` must be greater than 0.

`initial_task_queue_capacity` is optional. When greater than 0, the scheduler reserves task queue storage during initialization so normal task submission can avoid the first queue allocation.

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

Submitted tasks are stored in a FIFO queue. Workers take tasks from the front of the queue and execute them outside the scheduler mutex.

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

Batch submission reserves queue space once, queues all task descriptors under one scheduler lock, and wakes workers once after the batch is queued. It copies `Scheduler_Task` values into the scheduler queue; task data must still remain valid until the matching task has executed.

---

## Groups

```cpp
Scheduler_Group *group = scheduler_group_init(scheduler);

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
scheduler_group_deinit(scheduler, group);
```

Groups track completion for a specific batch of tasks. `scheduler_wait_group` blocks until every task submitted with that group has finished, but it does not wait for unrelated scheduler work.

A group belongs to the scheduler passed to `scheduler_group_init`. Deinit the group after its pending task count reaches 0 and before deinitializing the scheduler. The scheduler tracks live groups and validates that none are alive during `scheduler_deinit`.

When `scheduler_wait_group` is called from a worker owned by the same scheduler, that worker executes queued tasks while it waits. This lets a task submit child tasks to a group and wait for them without putting the worker to sleep.

The scheduler validates that a task does not wait for its own group.

---

## Cleanup

Later, after the scheduler shape settles, consider adding a `Queue<T>` convenience wrapper over `Ring_Buffer<T>` for FIFO-only call sites. This is a readability cleanup, not a required data-structure change.