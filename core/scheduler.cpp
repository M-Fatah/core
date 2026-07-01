#include "core/scheduler.h"
#include "core/defer.h"
#include "core/validate.h"
#include "core/memory/allocator.h"
#include "core/memory/arena_allocator.h"
#include "core/math/u32.h"
#include "core/containers/array.h"
#include "core/containers/ring_buffer.h"
#include "core/platform/platform.h"

inline static thread_local struct Scheduler_Worker *scheduler_current_worker = nullptr;

struct Scheduler_Parallel_For_Task_Context
{
	Scheduler_Parallel_For_Function function;
	void *data;
	U32 begin;
	U32 end;
};

struct Scheduler_Group
{
	Scheduler *scheduler;
	Platform_Condition_Variable *condition_variable;
	U64 pending_task_count;
};

struct Scheduler_Queued_Task
{
	Scheduler_Task task;
	Scheduler_Group *group;
};

struct Scheduler_Worker
{
	Scheduler *scheduler;
	Platform_Thread *thread;
	Scheduler_Group *current_group;
};

struct Scheduler
{
	Platform_Mutex *mutex;
	Platform_Condition_Variable *startup_condition_variable;
	Platform_Condition_Variable *work_condition_variable;
	Platform_Condition_Variable *idle_condition_variable;
	Array<Scheduler_Worker> workers;
	Ring_Buffer<Scheduler_Queued_Task> tasks;
	U32 started_worker_count;
	U32 active_task_count;
	U32 live_group_count;
	bool running;
};

inline static void
destroy(Scheduler_Worker &self)
{
	platform_thread_deinit(self.thread);
}

inline static void
_scheduler_parallel_for_task(void *data)
{
	Scheduler_Parallel_For_Task_Context *context = (Scheduler_Parallel_For_Task_Context *)data;
	context->function(context->begin, context->end, context->data);
}

inline static bool
_scheduler_try_run_next_task_from_locked_queue(Scheduler *self)
{
	if (ring_buffer_is_empty(self->tasks))
		return false;

	Scheduler_Queued_Task queued_task = ring_buffer_front(self->tasks);
	ring_buffer_pop_front(self->tasks);
	++self->active_task_count;
	platform_mutex_unlock(self->mutex);

	Scheduler_Group *previous_group = scheduler_current_worker->current_group;
	scheduler_current_worker->current_group = queued_task.group;
	queued_task.task.function(queued_task.task.data);
	scheduler_current_worker->current_group = previous_group;

	platform_mutex_lock(self->mutex);
	--self->active_task_count;
	if (queued_task.group != nullptr)
	{
		--queued_task.group->pending_task_count;
		if (queued_task.group->pending_task_count == 0)
			platform_condition_variable_broadcast(queued_task.group->condition_variable);
	}

	if (ring_buffer_is_empty(self->tasks) && self->active_task_count == 0)
		platform_condition_variable_broadcast(self->idle_condition_variable);

	return true;
}

inline static void
_scheduler_worker_main(void *data)
{
	Scheduler_Worker *worker = (Scheduler_Worker *)data;
	Scheduler *self = worker->scheduler;
	Scheduler_Worker *previous_worker = scheduler_current_worker;
	scheduler_current_worker = worker;

	platform_mutex_lock(self->mutex);
	++self->started_worker_count;
	platform_condition_variable_signal(self->startup_condition_variable);
	while (true)
	{
		while (self->running && ring_buffer_is_empty(self->tasks))
			platform_condition_variable_wait(self->work_condition_variable, self->mutex);

		if (!self->running && ring_buffer_is_empty(self->tasks))
			break;

		_scheduler_try_run_next_task_from_locked_queue(self);
	}
	platform_mutex_unlock(self->mutex);
	scheduler_current_worker = previous_worker;
}

// API.
Scheduler *
scheduler_init(Scheduler_Desc desc)
{
	validate(desc.worker_count > 0, "[SCHEDULER]: Worker count must be greater than 0.");

	Scheduler *self = memory::allocate_zeroed<Scheduler>();
	self->mutex = platform_mutex_init();
	self->startup_condition_variable = platform_condition_variable_init();
	self->work_condition_variable = platform_condition_variable_init();
	self->idle_condition_variable = platform_condition_variable_init();
	self->workers = array_init_with_count<Scheduler_Worker>(desc.worker_count);
	self->tasks = ring_buffer_init<Scheduler_Queued_Task>();
	ring_buffer_reserve(self->tasks, desc.initial_task_queue_capacity);
	self->running = true;

	for (U32 i = 0; i < desc.worker_count; ++i)
	{
		Scheduler_Worker *worker = &self->workers[i];
		worker->scheduler = self;
		worker->thread = platform_thread_init(Platform_Thread_Desc {
			.function = _scheduler_worker_main,
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
	validate(scheduler_current_worker == nullptr || scheduler_current_worker->scheduler != self, "[SCHEDULER]: Scheduler worker cannot deinit its own scheduler.");

	platform_mutex_lock(self->mutex);
	validate(self->live_group_count == 0, "[SCHEDULER]: Cannot deinit scheduler while task groups are alive.");
	self->running = false;
	platform_condition_variable_broadcast(self->work_condition_variable);
	platform_mutex_unlock(self->mutex);

	destroy(self->workers);
	ring_buffer_deinit(self->tasks);
	platform_condition_variable_deinit(self->idle_condition_variable);
	platform_condition_variable_deinit(self->work_condition_variable);
	platform_condition_variable_deinit(self->startup_condition_variable);
	platform_mutex_deinit(self->mutex);
	memory::deallocate(self);
}

Scheduler_Group *
scheduler_group_init(Scheduler *self)
{
	validate(self != nullptr, "[SCHEDULER]: Scheduler is not valid.");

	platform_mutex_lock(self->mutex);
	validate(self->running, "[SCHEDULER]: Cannot create task group after shutdown.");
	++self->live_group_count;
	platform_mutex_unlock(self->mutex);

	Scheduler_Group *group = memory::allocate_zeroed<Scheduler_Group>();
	group->scheduler = self;
	group->condition_variable = platform_condition_variable_init();
	return group;
}

void
scheduler_group_deinit(Scheduler *self, Scheduler_Group *group)
{
	validate(group != nullptr, "[SCHEDULER]: Task group is not valid.");
	validate(group->scheduler == self, "[SCHEDULER]: Task group belongs to a different scheduler.");

	platform_mutex_lock(self->mutex);
	validate(group->pending_task_count == 0, "[SCHEDULER]: Cannot deinit task group while tasks are pending.");
	--self->live_group_count;
	platform_mutex_unlock(self->mutex);

	platform_condition_variable_deinit(group->condition_variable);
	memory::deallocate(group);
}

void
scheduler_submit(Scheduler *self, Scheduler_Task task)
{
	validate(task.function != nullptr, "[SCHEDULER]: Task function is not valid.");

	platform_mutex_lock(self->mutex);
	validate(self->running, "[SCHEDULER]: Cannot submit task after shutdown.");
	ring_buffer_push_back(self->tasks, Scheduler_Queued_Task {
		.task = task
	});
	platform_condition_variable_signal(self->work_condition_variable);
	platform_mutex_unlock(self->mutex);
}

void
scheduler_submit(Scheduler *self, Span<const Scheduler_Task> tasks)
{
	platform_mutex_lock(self->mutex);
	validate(self->running, "[SCHEDULER]: Cannot submit task after shutdown.");
	if (tasks.count != 0)
	{
		ring_buffer_reserve(self->tasks, tasks.count);
		for (U64 i = 0; i < tasks.count; ++i)
		{
			ring_buffer_push_back(self->tasks, Scheduler_Queued_Task {
				.task = tasks.data[i]
			});
		}

		if (tasks.count == 1)
			platform_condition_variable_signal(self->work_condition_variable);
		else
			platform_condition_variable_broadcast(self->work_condition_variable);
	}
	platform_mutex_unlock(self->mutex);
}

void
scheduler_submit(Scheduler *self, Scheduler_Task task, Scheduler_Group *group)
{
	validate(task.function != nullptr, "[SCHEDULER]: Task function is not valid.");
	validate(group != nullptr, "[SCHEDULER]: Task group is not valid.");
	validate(group->scheduler == self, "[SCHEDULER]: Task group belongs to a different scheduler.");

	platform_mutex_lock(self->mutex);
	validate(self->running, "[SCHEDULER]: Cannot submit task after shutdown.");
	++group->pending_task_count;
	ring_buffer_push_back(self->tasks, Scheduler_Queued_Task {
		.task = task,
		.group = group
	});
	platform_condition_variable_signal(self->work_condition_variable);
	platform_mutex_unlock(self->mutex);
}

void
scheduler_submit(Scheduler *self, Span<const Scheduler_Task> tasks, Scheduler_Group *group)
{
	validate(group != nullptr, "[SCHEDULER]: Task group is not valid.");
	validate(group->scheduler == self, "[SCHEDULER]: Task group belongs to a different scheduler.");

	platform_mutex_lock(self->mutex);
	validate(self->running, "[SCHEDULER]: Cannot submit task after shutdown.");
	if (tasks.count != 0)
	{
		group->pending_task_count += tasks.count;
		ring_buffer_reserve(self->tasks, tasks.count);
		for (U64 i = 0; i < tasks.count; ++i)
		{
			ring_buffer_push_back(self->tasks, Scheduler_Queued_Task {
				.task = tasks.data[i],
				.group = group
			});
		}

		if (tasks.count == 1)
			platform_condition_variable_signal(self->work_condition_variable);
		else
			platform_condition_variable_broadcast(self->work_condition_variable);
	}
	platform_mutex_unlock(self->mutex);
}

void
scheduler_wait_group(Scheduler *self, Scheduler_Group *group)
{
	validate(group != nullptr, "[SCHEDULER]: Scheduler group is not valid.");
	validate(group->scheduler == self, "[SCHEDULER]: Scheduler group belongs to a different scheduler.");
	if (scheduler_current_worker != nullptr && scheduler_current_worker->scheduler == self)
		validate(scheduler_current_worker->current_group != group, "[SCHEDULER]: Scheduler task cannot wait for its own group.");

	platform_mutex_lock(self->mutex);
	if (scheduler_current_worker != nullptr && scheduler_current_worker->scheduler == self)
	{
		while (group->pending_task_count != 0)
			if (!_scheduler_try_run_next_task_from_locked_queue(self))
				platform_condition_variable_wait(group->condition_variable, self->mutex);
	}
	else
	{
		while (group->pending_task_count != 0)
			platform_condition_variable_wait(group->condition_variable, self->mutex);
	}
	platform_mutex_unlock(self->mutex);
}

void
scheduler_wait_all(Scheduler *self)
{
	validate(scheduler_current_worker == nullptr || scheduler_current_worker->scheduler != self, "[SCHEDULER]: Scheduler worker cannot wait for all work.");

	platform_mutex_lock(self->mutex);
	while (!ring_buffer_is_empty(self->tasks) || self->active_task_count != 0)
		platform_condition_variable_wait(self->idle_condition_variable, self->mutex);
	platform_mutex_unlock(self->mutex);
}

void
scheduler_parallel_for(Scheduler *self, Scheduler_Parallel_For_Desc desc)
{
	if (desc.count == 0)
		return;

	validate(self != nullptr, "[SCHEDULER]: Scheduler is not valid.");
	validate(desc.function != nullptr, "[SCHEDULER]: Parallel for function is not valid.");
	validate(desc.chunk_size > 0, "[SCHEDULER]: Parallel for chunk size must be greater than 0.");

	memory::Arena_Allocator_Mark temp_allocator_mark = memory::temp_allocator_mark();
	DEFER(memory::temp_allocator_reset_to_mark(temp_allocator_mark));

	U32 chunk_count = (U32)(((U64)desc.count + desc.chunk_size - 1) / desc.chunk_size);
	Array<Scheduler_Parallel_For_Task_Context> contexts = array_init_with_count<Scheduler_Parallel_For_Task_Context>(chunk_count, memory::temp_allocator());
	Array<Scheduler_Task> tasks = array_init_with_count<Scheduler_Task>(chunk_count, memory::temp_allocator());

	for (U32 i = 0; i < chunk_count; ++i)
	{
		U32 begin = i * desc.chunk_size;
		U32 remaining_count = desc.count - begin;
		U32 range_count = u32_min(remaining_count, desc.chunk_size);
		contexts[i] = Scheduler_Parallel_For_Task_Context {
			.function = desc.function,
			.data = desc.data,
			.begin = begin,
			.end = begin + range_count
		};
		tasks[i] = Scheduler_Task {
			.function = _scheduler_parallel_for_task,
			.data = &contexts[i]
		};
	}

	Scheduler_Group *group = scheduler_group_init(self);
	DEFER(scheduler_group_deinit(self, group));
	scheduler_submit(self, span_init(tasks), group);
	scheduler_wait_group(self, group);
}