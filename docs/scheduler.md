# Scheduler

**Header:** `core/scheduler.h`

Long-lived worker-thread lifetime helper built on the platform thread, mutex, and condition-variable primitives.

This first scheduler slice only owns startup and shutdown. It does not submit jobs yet.

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