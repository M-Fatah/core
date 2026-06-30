# Scheduler

**Header:** `core/scheduler.h`

Long-lived worker-thread scheduler built on the platform thread, mutex, and condition-variable primitives.

---

## Lifetime

```cpp
#include <core/scheduler.h>

Scheduler *scheduler = scheduler_init(Scheduler_Desc {
	.worker_count = 4
});

scheduler_deinit(scheduler);
```

`scheduler_init` starts the requested worker threads and returns only after every worker has entered its wait state.

`scheduler_deinit` requests shutdown, wakes all workers, joins every worker thread, and releases scheduler-owned synchronization objects.

`worker_count` must be greater than 0.

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

scheduler_wait_group(scheduler, group);
scheduler_group_deinit(scheduler, group);
```

Groups track completion for a specific batch of tasks. `scheduler_wait_group` blocks until every task submitted with that group has finished, but it does not wait for unrelated scheduler work.

A group belongs to the scheduler passed to `scheduler_group_init`. Deinit the group after its pending task count reaches 0 and before deinitializing the scheduler.

When `scheduler_wait_group` is called from a worker owned by the same scheduler, that worker executes queued tasks while it waits. This lets a task submit child tasks to a group and wait for them without putting the worker to sleep.

The scheduler validates that a task does not wait for its own group.

---

## Cleanup

Later, after the scheduler shape settles, consider adding a `Queue<T>` convenience wrapper over `Ring_Buffer<T>` for FIFO-only call sites. This is a readability cleanup, not a required data-structure change.