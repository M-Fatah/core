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
	U32 worker_count;
	U32 active_replacement_worker_count;
	U32 queued_task_count;
	U32 started_worker_count;
	U32 active_task_count;
	U32 live_group_count;
	U32 blocked_worker_count;
	U32 next_worker_index;
	bool is_running;
};

inline static void
_scheduler_update_active_replacement_worker_count(Scheduler *self)
{
	U32 replacement_worker_count = (U32)self->workers.count - self->worker_count;
	U32 active_replacement_worker_count = u32_min(self->blocked_worker_count, replacement_worker_count);
	if (self->active_replacement_worker_count == active_replacement_worker_count)
		return;

	self->active_replacement_worker_count = active_replacement_worker_count;
	platform_condition_variable_broadcast(self->work_condition_variable);
}

inline static bool
_scheduler_try_run_next_task_from_locked_worker_queues(Scheduler *self, Scheduler_Worker *worker)
{
	Scheduler_Queued_Task queued_task = {};
	if (!ring_buffer_is_empty(worker->tasks))
	{
		queued_task = ring_buffer_front(worker->tasks);
		ring_buffer_pop_front(worker->tasks);
	}
	else
	{
		Scheduler_Worker *source_worker = nullptr;
		for (U32 i = 0; i < self->worker_count; ++i)
		{
			Scheduler_Worker *other_worker = &self->workers[(worker->index + i) % self->worker_count];
			if (other_worker == worker || ring_buffer_is_empty(other_worker->tasks))
				continue;

			source_worker = other_worker;
			break;
		}

		if (source_worker == nullptr)
			return false;

		queued_task = ring_buffer_back(source_worker->tasks);
		ring_buffer_pop_back(source_worker->tasks);
	}

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
	U32 total_worker_count = desc.worker_count + desc.replacement_worker_count;

	Platform_Thread_Function worker_function = [](void *data) {
		Scheduler_Worker *worker = (Scheduler_Worker *)data;
		Scheduler *self = worker->scheduler;
		scheduler_current_worker = worker;
		constexpr auto worker_is_active = [](Scheduler *self, Scheduler_Worker *worker) -> bool {
			if (worker->index < self->worker_count)
				return true;
			return worker->index - self->worker_count < self->active_replacement_worker_count;
		};

		platform_mutex_lock(self->mutex);
		++self->started_worker_count;
		platform_condition_variable_signal(self->startup_condition_variable);
		while (true)
		{
			while (self->is_running && (self->queued_task_count == 0 || !worker_is_active(self, worker)))
				platform_condition_variable_wait(self->work_condition_variable, self->mutex);

			if (!self->is_running && (self->queued_task_count == 0 || !worker_is_active(self, worker)))
				break;

			_scheduler_try_run_next_task_from_locked_worker_queues(self, worker);
		}
		platform_mutex_unlock(self->mutex);
		scheduler_current_worker = nullptr;
	};

	Scheduler *self = memory::allocate_zeroed<Scheduler>();
	self->mutex                      = platform_mutex_init();
	self->startup_condition_variable = platform_condition_variable_init();
	self->work_condition_variable    = platform_condition_variable_init();
	self->idle_condition_variable    = platform_condition_variable_init();
	self->group_condition_variable   = platform_condition_variable_init();
	self->workers                    = array_init_with_count<Scheduler_Worker>(total_worker_count);
	self->worker_count               = desc.worker_count;
	self->is_running                 = true;

	U64 task_queue_capacity_per_worker = desc.initial_task_queue_capacity / desc.worker_count;
	U64 extra_task_queue_capacity      = desc.initial_task_queue_capacity % desc.worker_count;
	for (U32 i = 0; i < total_worker_count; ++i)
	{
		Scheduler_Worker *worker = &self->workers[i];
		*worker = Scheduler_Worker {};
		worker->scheduler = self;
		worker->tasks = ring_buffer_init<Scheduler_Queued_Task>();
		if (i < desc.worker_count)
		{
			U64 task_queue_capacity = task_queue_capacity_per_worker;
			if (i < extra_task_queue_capacity)
				++task_queue_capacity;
			ring_buffer_reserve(worker->tasks, task_queue_capacity);
		}
		worker->index = i;
		worker->thread = platform_thread_init(Platform_Thread_Desc {
			.function = worker_function,
			.data = worker,
			.name = desc.worker_thread_name != nullptr ? desc.worker_thread_name : "Scheduler"
		});
	}

	platform_mutex_lock(self->mutex);
	while (self->started_worker_count != total_worker_count)
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
	self->is_running = false;
	platform_condition_variable_broadcast(self->work_condition_variable);
	platform_mutex_unlock(self->mutex);

	for (U64 i = 0; i < self->workers.count; ++i)
		platform_thread_deinit(self->workers[i].thread);

	validate(self->queued_task_count == 0 && self->active_task_count == 0, "[SCHEDULER]: Shutdown did not drain all tasks.");

	for (U64 i = 0; i < self->workers.count; ++i)
		ring_buffer_deinit(self->workers[i].tasks);
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
	validate(self->is_running, "[SCHEDULER]: Cannot create task group after shutdown.");
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
scheduler_submit(Scheduler *self, Slice<const Scheduler_Task> tasks, Scheduler_Group *group)
{
	validate(group == nullptr || group->scheduler == self, "[SCHEDULER]: Task group belongs to a different scheduler.");

	platform_mutex_lock(self->mutex);
	validate(self->is_running, "[SCHEDULER]: Cannot submit task after shutdown.");
	if (tasks.count != 0)
	{
		U64 task_count_per_worker = tasks.count / self->worker_count;
		U64 remaining_task_count = tasks.count % self->worker_count;
		for (U32 i = 0; i < self->worker_count; ++i)
		{
			U64 worker_offset = i >= self->next_worker_index ? i - self->next_worker_index : self->worker_count - self->next_worker_index + i;
			U64 worker_task_count = task_count_per_worker;
			if (worker_offset < remaining_task_count)
				++worker_task_count;
			ring_buffer_reserve(self->workers[i].tasks, worker_task_count);
		}

		if (group != nullptr)
			group->pending_task_count += tasks.count;

		for (U64 i = 0; i < tasks.count; ++i)
		{
			Scheduler_Worker *worker = &self->workers[self->next_worker_index];
			self->next_worker_index = (self->next_worker_index + 1) % self->worker_count;
			ring_buffer_push_back(worker->tasks, Scheduler_Queued_Task {
				.task = tasks.data[i],
				.group = group
			});
			++self->queued_task_count;
		}
		platform_condition_variable_broadcast(self->work_condition_variable);
		platform_condition_variable_broadcast(self->group_condition_variable);
	}
	platform_mutex_unlock(self->mutex);
}

void
scheduler_wait_group(Scheduler *self, Scheduler_Group *group)
{
	validate(group->scheduler == self, "[SCHEDULER]: Scheduler group belongs to a different scheduler.");
	bool is_scheduler_worker = scheduler_current_worker != nullptr && scheduler_current_worker->scheduler == self;
	validate(!is_scheduler_worker || scheduler_current_worker->current_group != group, "[SCHEDULER]: Scheduler task cannot wait for its own group.");

	platform_mutex_lock(self->mutex);
	while (group->pending_task_count != 0)
	{
		if (is_scheduler_worker && _scheduler_try_run_next_task_from_locked_worker_queues(self, scheduler_current_worker))
			continue;
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
	if (scheduler_current_worker->blocking_depth == 0)
	{
		++self->blocked_worker_count;
		_scheduler_update_active_replacement_worker_count(self);
	}
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
		--self->blocked_worker_count;
		_scheduler_update_active_replacement_worker_count(self);
	}
	platform_mutex_unlock(self->mutex);
}

U32
scheduler_get_current_worker_index(Scheduler *self)
{
	if (scheduler_current_worker == nullptr || scheduler_current_worker->scheduler != self)
		return U32_MAX;

	return scheduler_current_worker->index;
}

Scheduler_Stats
scheduler_get_stats(Scheduler *self)
{
	platform_mutex_lock(self->mutex);
	Scheduler_Stats stats = {
		.worker_count = self->worker_count,
		.replacement_worker_count = (U32)self->workers.count - self->worker_count,
		.active_replacement_worker_count = self->active_replacement_worker_count,
		.blocked_worker_count = self->blocked_worker_count,
		.active_task_count = self->active_task_count,
		.queued_task_count = self->queued_task_count,
		.live_group_count = self->live_group_count
	};
	platform_mutex_unlock(self->mutex);
	return stats;
}

void
scheduler_parallel_for(Scheduler *self, Scheduler_Parallel_For_Desc desc)
{
	if (desc.count == 0)
		return;

	struct Scheduler_Parallel_For_Task_Context
	{
		void (*function)(U32 begin, U32 end, void *data);
		void *data;
		U32 begin;
		U32 end;
	};

	constexpr auto task_function = [](void *data) {
		Scheduler_Parallel_For_Task_Context *context = (Scheduler_Parallel_For_Task_Context *)data;
		context->function(context->begin, context->end, context->data);
	};

	constexpr auto auto_chunk_size = [](Scheduler *self, U32 count) -> U32 {
		U64 target_chunk_count = self->worker_count * 4;
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
	scheduler_submit(self, slice_from(tasks), group);
	scheduler_wait_group(self, group);
}