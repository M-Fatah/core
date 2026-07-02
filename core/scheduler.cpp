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

struct Scheduler_Group
{
	Scheduler *scheduler;
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
	Ring_Buffer<Scheduler_Queued_Task> tasks;
	Scheduler_Group *current_group;
	U32 index;
	U32 blocking_depth;
};

struct Scheduler
{
	Platform_Mutex *mutex;
	Platform_Condition_Variable *startup_condition_variable;
	Platform_Condition_Variable *work_condition_variable;
	Platform_Condition_Variable *idle_condition_variable;
	Platform_Condition_Variable *group_condition_variable;
	Array<Scheduler_Worker> workers;
	U64 next_worker_index;
	U64 queued_task_count;
	U32 started_worker_count;
	U32 active_task_count;
	U32 live_group_count;
	U32 blocked_worker_count;
	bool running;
};

inline static void
_scheduler_wake_task_waiters(Scheduler *self)
{
	platform_condition_variable_broadcast(self->work_condition_variable);
	platform_condition_variable_broadcast(self->group_condition_variable);
}

inline static void
_scheduler_queue_task(Scheduler *self, Scheduler_Queued_Task queued_task)
{
	Scheduler_Worker *worker = &self->workers[self->next_worker_index];
	self->next_worker_index = (self->next_worker_index + 1) % self->workers.count;
	ring_buffer_push_back(worker->tasks, queued_task);
	++self->queued_task_count;
}

inline static void
_scheduler_reserve_worker_queue_space(Scheduler *self, U64 task_count)
{
	U64 worker_count = self->workers.count;
	U64 task_count_per_worker = task_count / worker_count;
	U64 extra_task_count = task_count % worker_count;

	for (U64 i = 0; i < worker_count; ++i)
	{
		U64 worker_offset = i >= self->next_worker_index ? i - self->next_worker_index : worker_count - self->next_worker_index + i;
		U64 worker_task_count = task_count_per_worker;
		if (worker_offset < extra_task_count)
			++worker_task_count;
		ring_buffer_reserve(self->workers[i].tasks, worker_task_count);
	}
}

inline static bool
_scheduler_try_run_next_task_from_locked_worker_queues(Scheduler *self, Scheduler_Worker *worker)
{
	Scheduler_Worker *source_worker = worker;
	if (ring_buffer_is_empty(source_worker->tasks))
	{
		source_worker = nullptr;
		for (U64 i = 1; i < self->workers.count; ++i)
		{
			Scheduler_Worker *other_worker = &self->workers[(worker->index + i) % self->workers.count];
			if (!ring_buffer_is_empty(other_worker->tasks))
			{
				source_worker = other_worker;
				break;
			}
		}
	}

	if (source_worker == nullptr)
		return false;

	Scheduler_Queued_Task queued_task = ring_buffer_front(source_worker->tasks);
	ring_buffer_pop_front(source_worker->tasks);
	--self->queued_task_count;
	++self->active_task_count;
	platform_mutex_unlock(self->mutex);

	Scheduler_Group *previous_group = worker->current_group;
	worker->current_group = queued_task.group;
	queued_task.task.function(queued_task.task.data);
	worker->current_group = previous_group;
	validate(worker->blocking_depth == 0, "[SCHEDULER]: Scheduler worker finished task while still marked as blocking.");

	platform_mutex_lock(self->mutex);
	--self->active_task_count;
	if (queued_task.group != nullptr)
	{
		--queued_task.group->pending_task_count;
		if (queued_task.group->pending_task_count == 0)
			platform_condition_variable_broadcast(self->group_condition_variable);
	}

	if (self->queued_task_count == 0 && self->active_task_count == 0)
		platform_condition_variable_broadcast(self->idle_condition_variable);

	return true;
}

// API.
Scheduler *
scheduler_init(Scheduler_Desc desc)
{
	validate(desc.worker_count > 0, "[SCHEDULER]: Worker count must be greater than 0.");

	Platform_Thread_Function worker_function = [](void *data) {
		Scheduler_Worker *worker = (Scheduler_Worker *)data;
		Scheduler *self = worker->scheduler;
		Scheduler_Worker *previous_worker = scheduler_current_worker;
		scheduler_current_worker = worker;

		platform_mutex_lock(self->mutex);
		++self->started_worker_count;
		platform_condition_variable_signal(self->startup_condition_variable);
		while (true)
		{
			while (self->running && self->queued_task_count == 0)
				platform_condition_variable_wait(self->work_condition_variable, self->mutex);

			if (!self->running && self->queued_task_count == 0)
				break;

			_scheduler_try_run_next_task_from_locked_worker_queues(self, worker);
		}
		platform_mutex_unlock(self->mutex);
		scheduler_current_worker = previous_worker;
	};

	Scheduler *self = memory::allocate_zeroed<Scheduler>();
	self->mutex = platform_mutex_init();
	self->startup_condition_variable = platform_condition_variable_init();
	self->work_condition_variable = platform_condition_variable_init();
	self->idle_condition_variable = platform_condition_variable_init();
	self->group_condition_variable = platform_condition_variable_init();
	self->workers = array_init_with_count<Scheduler_Worker>(desc.worker_count);
	self->running = true;

	for (U32 i = 0; i < desc.worker_count; ++i)
	{
		Scheduler_Worker *worker = &self->workers[i];
		*worker = Scheduler_Worker {};
		worker->scheduler = self;
		worker->tasks = ring_buffer_init<Scheduler_Queued_Task>();
		U64 task_queue_capacity = desc.initial_task_queue_capacity / desc.worker_count;
		if (i < desc.initial_task_queue_capacity % desc.worker_count)
			++task_queue_capacity;
		ring_buffer_reserve(worker->tasks, task_queue_capacity);
		worker->current_group = nullptr;
		worker->index = i;
		worker->blocking_depth = 0;
		worker->thread = platform_thread_init(Platform_Thread_Desc {
			.function = worker_function,
			.data = worker,
			.name = desc.worker_name != nullptr ? desc.worker_name : "Scheduler"
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

	for (U64 i = 0; i < self->workers.count; ++i)
	{
		platform_thread_deinit(self->workers[i].thread);
		ring_buffer_deinit(self->workers[i].tasks);
	}
	array_deinit(self->workers);
	validate(self->blocked_worker_count == 0, "[SCHEDULER]: Cannot deinit scheduler while workers are marked as blocking.");
	platform_condition_variable_deinit(self->group_condition_variable);
	platform_condition_variable_deinit(self->idle_condition_variable);
	platform_condition_variable_deinit(self->work_condition_variable);
	platform_condition_variable_deinit(self->startup_condition_variable);
	platform_mutex_deinit(self->mutex);
	memory::deallocate(self);
}

Scheduler_Group *
scheduler_group_init(Scheduler *self)
{
	platform_mutex_lock(self->mutex);
	validate(self->running, "[SCHEDULER]: Cannot create task group after shutdown.");
	++self->live_group_count;
	platform_mutex_unlock(self->mutex);

	Scheduler_Group *group = memory::allocate_zeroed<Scheduler_Group>();
	group->scheduler = self;
	return group;
}

void
scheduler_group_deinit(Scheduler *self, Scheduler_Group *group)
{
	validate(group->scheduler == self, "[SCHEDULER]: Task group belongs to a different scheduler.");

	platform_mutex_lock(self->mutex);
	validate(group->pending_task_count == 0, "[SCHEDULER]: Cannot deinit task group while tasks are pending.");
	--self->live_group_count;
	platform_mutex_unlock(self->mutex);

	memory::deallocate(group);
}

void
scheduler_submit(Scheduler *self, Scheduler_Task task)
{
	platform_mutex_lock(self->mutex);
	validate(self->running, "[SCHEDULER]: Cannot submit task after shutdown.");
	_scheduler_queue_task(self, Scheduler_Queued_Task {
		.task = task
	});
	_scheduler_wake_task_waiters(self);
	platform_mutex_unlock(self->mutex);
}

void
scheduler_submit(Scheduler *self, Span<const Scheduler_Task> tasks)
{
	platform_mutex_lock(self->mutex);
	validate(self->running, "[SCHEDULER]: Cannot submit task after shutdown.");
	if (tasks.count != 0)
	{
		_scheduler_reserve_worker_queue_space(self, tasks.count);
		for (U64 i = 0; i < tasks.count; ++i)
		{
			_scheduler_queue_task(self, Scheduler_Queued_Task {
				.task = tasks.data[i]
			});
		}
		_scheduler_wake_task_waiters(self);
	}
	platform_mutex_unlock(self->mutex);
}

void
scheduler_submit(Scheduler *self, Scheduler_Task task, Scheduler_Group *group)
{
	validate(group->scheduler == self, "[SCHEDULER]: Task group belongs to a different scheduler.");

	platform_mutex_lock(self->mutex);
	validate(self->running, "[SCHEDULER]: Cannot submit task after shutdown.");
	++group->pending_task_count;
	_scheduler_queue_task(self, Scheduler_Queued_Task {
		.task = task,
		.group = group
	});
	_scheduler_wake_task_waiters(self);
	platform_mutex_unlock(self->mutex);
}

void
scheduler_submit(Scheduler *self, Span<const Scheduler_Task> tasks, Scheduler_Group *group)
{
	validate(group->scheduler == self, "[SCHEDULER]: Task group belongs to a different scheduler.");

	platform_mutex_lock(self->mutex);
	validate(self->running, "[SCHEDULER]: Cannot submit task after shutdown.");
	if (tasks.count != 0)
	{
		group->pending_task_count += tasks.count;
		_scheduler_reserve_worker_queue_space(self, tasks.count);
		for (U64 i = 0; i < tasks.count; ++i)
		{
			_scheduler_queue_task(self, Scheduler_Queued_Task {
				.task = tasks.data[i],
				.group = group
			});
		}
		_scheduler_wake_task_waiters(self);
	}
	platform_mutex_unlock(self->mutex);
}

void
scheduler_wait_group(Scheduler *self, Scheduler_Group *group)
{
	validate(group->scheduler == self, "[SCHEDULER]: Scheduler group belongs to a different scheduler.");
	if (scheduler_current_worker != nullptr && scheduler_current_worker->scheduler == self)
		validate(scheduler_current_worker->current_group != group, "[SCHEDULER]: Scheduler task cannot wait for its own group.");

	platform_mutex_lock(self->mutex);
	if (scheduler_current_worker != nullptr && scheduler_current_worker->scheduler == self)
	{
		while (group->pending_task_count != 0)
			if (!_scheduler_try_run_next_task_from_locked_worker_queues(self, scheduler_current_worker))
				platform_condition_variable_wait(self->group_condition_variable, self->mutex);
	}
	else
	{
		while (group->pending_task_count != 0)
			platform_condition_variable_wait(self->group_condition_variable, self->mutex);
	}
	platform_mutex_unlock(self->mutex);
}

void
scheduler_wait_all(Scheduler *self)
{
	validate(scheduler_current_worker == nullptr || scheduler_current_worker->scheduler != self, "[SCHEDULER]: Scheduler worker cannot wait for all work.");

	platform_mutex_lock(self->mutex);
	while (self->queued_task_count != 0 || self->active_task_count != 0)
		platform_condition_variable_wait(self->idle_condition_variable, self->mutex);
	platform_mutex_unlock(self->mutex);
}

void
scheduler_worker_block_ahead(Scheduler *self)
{
	validate(scheduler_current_worker != nullptr && scheduler_current_worker->scheduler == self, "[SCHEDULER]: Only a scheduler worker can mark itself as blocking.");

	platform_mutex_lock(self->mutex);
	validate(scheduler_current_worker->blocking_depth < U32_MAX, "[SCHEDULER]: Worker blocking depth overflow.");
	if (scheduler_current_worker->blocking_depth == 0)
		++self->blocked_worker_count;
	++scheduler_current_worker->blocking_depth;
	platform_mutex_unlock(self->mutex);
}

void
scheduler_worker_block_clear(Scheduler *self)
{
	validate(scheduler_current_worker != nullptr && scheduler_current_worker->scheduler == self, "[SCHEDULER]: Only a scheduler worker can clear its blocking marker.");

	platform_mutex_lock(self->mutex);
	validate(scheduler_current_worker->blocking_depth > 0, "[SCHEDULER]: Worker blocking marker was not set.");
	--scheduler_current_worker->blocking_depth;
	if (scheduler_current_worker->blocking_depth == 0)
	{
		validate(self->blocked_worker_count > 0, "[SCHEDULER]: Blocked worker count underflow.");
		--self->blocked_worker_count;
	}
	platform_mutex_unlock(self->mutex);
}

U32
scheduler_get_worker_count(Scheduler *self)
{
	return (U32)self->workers.count;
}

U32
scheduler_get_blocked_worker_count(Scheduler *self)
{
	platform_mutex_lock(self->mutex);
	U32 result = self->blocked_worker_count;
	platform_mutex_unlock(self->mutex);
	return result;
}

U32
scheduler_get_current_worker_index(Scheduler *self)
{
	if (scheduler_current_worker == nullptr || scheduler_current_worker->scheduler != self)
		return U32_MAX;

	return scheduler_current_worker->index;
}

void
scheduler_parallel_for(Scheduler *self, Scheduler_Parallel_For_Desc desc)
{
	if (desc.count == 0)
		return;

	struct Scheduler_Parallel_For_Task_Context
	{
		Scheduler_Parallel_For_Function function;
		void *data;
		U32 begin;
		U32 end;
	};

	Scheduler_Task_Function task_function = [](void *data) {
		Scheduler_Parallel_For_Task_Context *context = (Scheduler_Parallel_For_Task_Context *)data;
		context->function(context->begin, context->end, context->data);
	};

	auto auto_chunk_size = [](Scheduler *self, U32 count) -> U32 {
		U64 target_chunk_count = self->workers.count * 4;
		if (target_chunk_count > count)
			target_chunk_count = count;
		return (U32)(((U64)count + target_chunk_count - 1) / target_chunk_count);
	};

	if (desc.chunk_size == 0)
		desc.chunk_size = auto_chunk_size(self, desc.count);

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
			.function = task_function,
			.data = &contexts[i]
		};
	}

	Scheduler_Group *group = scheduler_group_init(self);
	DEFER(scheduler_group_deinit(self, group));
	scheduler_submit(self, span_init(tasks), group);
	scheduler_wait_group(self, group);
}