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

scheduler_wait_idle(scheduler);
```

Submitted tasks are stored in a FIFO `Ring_Buffer<Scheduler_Task>`. Workers take tasks from the front of the queue and execute them outside the scheduler mutex.

Task data must remain valid until the task has executed.

`scheduler_wait_idle` blocks until there are no queued tasks and no worker is currently executing a task.

Call `scheduler_wait_idle` from the thread coordinating the scheduler. A scheduler task must not wait for its own scheduler to become idle because that task is part of the active task count.

---

## Cleanup

Later, after the scheduler shape settles, consider adding a `Queue<T>` convenience wrapper over `Ring_Buffer<T>` for FIFO-only call sites. This is a readability cleanup, not a required data-structure change.