#include "core/scheduler.h"

#include "core/validate.h"
#include "core/memory/memory.h"
#include "core/containers/array.h"
#include "core/platform/platform.h"

struct Scheduler_Worker
{
	Scheduler *scheduler;
	Platform_Thread *thread;
};

struct Scheduler
{
	Platform_Mutex *mutex;
	Platform_Condition_Variable *startup_condition_variable;
	Platform_Condition_Variable *shutdown_condition_variable;
	Array<Scheduler_Worker> workers;
	U32 started_worker_count;
	bool running;
};

inline static void
destroy(Scheduler_Worker &self)
{
	platform_thread_deinit(self.thread);
}

inline static void
_scheduler_worker_entry(void *data)
{
	Scheduler_Worker *worker = (Scheduler_Worker *)data;
	Scheduler *self = worker->scheduler;

	platform_mutex_lock(self->mutex);
	self->started_worker_count += 1;
	platform_condition_variable_signal(self->startup_condition_variable);
	while (self->running)
		platform_condition_variable_wait(self->shutdown_condition_variable, self->mutex);
	platform_mutex_unlock(self->mutex);
}

// API.
Scheduler *
scheduler_init(Scheduler_Desc desc)
{
	validate(desc.worker_count > 0, "[SCHEDULER]: Worker count must be greater than 0.");

	Scheduler *self = memory::allocate_zeroed<Scheduler>();
	self->mutex = platform_mutex_init();
	self->startup_condition_variable = platform_condition_variable_init();
	self->shutdown_condition_variable = platform_condition_variable_init();
	self->workers = array_init_with_count<Scheduler_Worker>(desc.worker_count);
	self->running = true;

	for (U32 i = 0; i < desc.worker_count; ++i)
	{
		Scheduler_Worker *worker = &self->workers[i];
		worker->scheduler = self;
		worker->thread = platform_thread_init(Platform_Thread_Desc {
			.function = _scheduler_worker_entry,
			.data = worker
		});
	}

	platform_mutex_lock(self->mutex);
	while (self->started_worker_count != desc.worker_count)
		platform_condition_variable_wait(self->startup_condition_variable, self->mutex);
	platform_mutex_unlock(self->mutex);

	return self;
}

void
scheduler_deinit(Scheduler *self)
{
	platform_mutex_lock(self->mutex);
	self->running = false;
	platform_condition_variable_broadcast(self->shutdown_condition_variable);
	platform_mutex_unlock(self->mutex);

	destroy(self->workers);
	platform_condition_variable_deinit(self->shutdown_condition_variable);
	platform_condition_variable_deinit(self->startup_condition_variable);
	platform_mutex_deinit(self->mutex);
	memory::deallocate(self);
}