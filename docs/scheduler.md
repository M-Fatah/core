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
```

Submitted tasks are stored in a FIFO `Ring_Buffer<Scheduler_Task>`. Workers take tasks from the front of the queue and execute them outside the scheduler mutex.

Task data must remain valid until the task has executed.

---

## Cleanup

Later, after the scheduler shape settles, consider adding a `Queue<T>` convenience wrapper over `Ring_Buffer<T>` for FIFO-only call sites. This is a readability cleanup, not a required data-structure change.