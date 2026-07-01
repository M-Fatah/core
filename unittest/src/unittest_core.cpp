#include <core/tester.h>
#include <core/json.h>
#include <core/base64.h>
#include <core/log.h>
#include <core/result.h>
#include <core/scheduler.h>
#include <core/validate.h>
#include <core/memory/allocator.h>
#include <core/memory/pool_allocator.h>
#include <core/memory/arena_allocator.h>
#include <core/platform/platform.h>

TESTER_TEST("[CORE]: Scheduler")
{
	Scheduler *scheduler = scheduler_init(Scheduler_Desc {
		.worker_count = 2,
		.initial_task_queue_capacity = 8
	});
	TESTER_CHECK(scheduler != nullptr);
	scheduler_deinit(scheduler);
}

struct Scheduler_Test_Worker_Query_Context
{
	Scheduler *scheduler;
	Scheduler *other_scheduler;
	Platform_Mutex *mutex;
	U32 worker_count;
	U32 worker_index;
	U32 other_scheduler_worker_index;
};

inline static void
_scheduler_test_worker_query_task(void *data)
{
	Scheduler_Test_Worker_Query_Context *context = (Scheduler_Test_Worker_Query_Context *)data;
	U32 worker_count = scheduler_get_worker_count(context->scheduler);
	U32 worker_index = scheduler_get_current_worker_index(context->scheduler);
	U32 other_scheduler_worker_index = scheduler_get_current_worker_index(context->other_scheduler);

	platform_mutex_lock(context->mutex);
	context->worker_count = worker_count;
	context->worker_index = worker_index;
	context->other_scheduler_worker_index = other_scheduler_worker_index;
	platform_mutex_unlock(context->mutex);
}

TESTER_TEST("[CORE]: Scheduler Worker Queries")
{
	Scheduler *scheduler = scheduler_init(Scheduler_Desc {
		.worker_count = 2
	});
	Scheduler *other_scheduler = scheduler_init(Scheduler_Desc {
		.worker_count = 1
	});
	Platform_Mutex *mutex = platform_mutex_init();
	Scheduler_Test_Worker_Query_Context context = {
		.scheduler = scheduler,
		.other_scheduler = other_scheduler,
		.mutex = mutex,
		.worker_index = U32_MAX,
		.other_scheduler_worker_index = U32_MAX
	};

	TESTER_CHECK(scheduler_get_worker_count(scheduler) == 2);
	TESTER_CHECK(scheduler_get_current_worker_index(scheduler) == U32_MAX);

	scheduler_submit(scheduler, Scheduler_Task {
		.function = _scheduler_test_worker_query_task,
		.data = &context
	});
	scheduler_wait_all(scheduler);

	platform_mutex_lock(mutex);
	U32 worker_count = context.worker_count;
	U32 worker_index = context.worker_index;
	U32 other_scheduler_worker_index = context.other_scheduler_worker_index;
	platform_mutex_unlock(mutex);

	TESTER_CHECK(worker_count == 2);
	TESTER_CHECK(worker_index < 2);
	TESTER_CHECK(other_scheduler_worker_index == U32_MAX);

	platform_mutex_deinit(mutex);
	scheduler_deinit(other_scheduler);
	scheduler_deinit(scheduler);
}

struct Scheduler_Test_Worker_Blocking_Context
{
	Scheduler *scheduler;
	Platform_Mutex *mutex;
	Platform_Condition_Variable *condition_variable;
	U32 count_after_first_block;
	U32 count_after_nested_block;
	U32 count_after_first_clear;
	U32 count_after_final_clear;
	bool block_started;
	bool release;
};

inline static void
_scheduler_test_worker_blocking_task(void *data)
{
	Scheduler_Test_Worker_Blocking_Context *context = (Scheduler_Test_Worker_Blocking_Context *)data;

	scheduler_worker_block_ahead(context->scheduler);
	U32 count_after_first_block = scheduler_get_blocked_worker_count(context->scheduler);
	scheduler_worker_block_ahead(context->scheduler);
	U32 count_after_nested_block = scheduler_get_blocked_worker_count(context->scheduler);

	platform_mutex_lock(context->mutex);
	context->count_after_first_block = count_after_first_block;
	context->count_after_nested_block = count_after_nested_block;
	context->block_started = true;
	platform_condition_variable_signal(context->condition_variable);
	while (!context->release)
		platform_condition_variable_wait(context->condition_variable, context->mutex);
	platform_mutex_unlock(context->mutex);

	scheduler_worker_block_clear(context->scheduler);
	U32 count_after_first_clear = scheduler_get_blocked_worker_count(context->scheduler);
	scheduler_worker_block_clear(context->scheduler);
	U32 count_after_final_clear = scheduler_get_blocked_worker_count(context->scheduler);

	platform_mutex_lock(context->mutex);
	context->count_after_first_clear = count_after_first_clear;
	context->count_after_final_clear = count_after_final_clear;
	platform_mutex_unlock(context->mutex);
}

TESTER_TEST("[CORE]: Scheduler Worker Blocking")
{
	Scheduler *scheduler = scheduler_init(Scheduler_Desc {
		.worker_count = 1
	});
	Platform_Mutex *mutex = platform_mutex_init();
	Platform_Condition_Variable *condition_variable = platform_condition_variable_init();
	Scheduler_Test_Worker_Blocking_Context context = {
		.scheduler = scheduler,
		.mutex = mutex,
		.condition_variable = condition_variable
	};

	TESTER_CHECK(scheduler_get_blocked_worker_count(scheduler) == 0);

	scheduler_submit(scheduler, Scheduler_Task {
		.function = _scheduler_test_worker_blocking_task,
		.data = &context
	});

	platform_mutex_lock(mutex);
	while (!context.block_started)
		platform_condition_variable_wait(condition_variable, mutex);
	platform_mutex_unlock(mutex);

	TESTER_CHECK(scheduler_get_blocked_worker_count(scheduler) == 1);

	platform_mutex_lock(mutex);
	context.release = true;
	platform_condition_variable_signal(condition_variable);
	platform_mutex_unlock(mutex);

	scheduler_wait_all(scheduler);

	platform_mutex_lock(mutex);
	U32 count_after_first_block = context.count_after_first_block;
	U32 count_after_nested_block = context.count_after_nested_block;
	U32 count_after_first_clear = context.count_after_first_clear;
	U32 count_after_final_clear = context.count_after_final_clear;
	platform_mutex_unlock(mutex);

	TESTER_CHECK(count_after_first_block == 1);
	TESTER_CHECK(count_after_nested_block == 1);
	TESTER_CHECK(count_after_first_clear == 1);
	TESTER_CHECK(count_after_final_clear == 0);
	TESTER_CHECK(scheduler_get_blocked_worker_count(scheduler) == 0);

	platform_condition_variable_deinit(condition_variable);
	platform_mutex_deinit(mutex);
	scheduler_deinit(scheduler);
}

struct Scheduler_Test_Task_Context
{
	Platform_Mutex *mutex;
	U32 finished_count;
};

inline static void
_scheduler_test_task(void *data)
{
	Scheduler_Test_Task_Context *context = (Scheduler_Test_Task_Context *)data;

	platform_mutex_lock(context->mutex);
	++context->finished_count;
	platform_mutex_unlock(context->mutex);
}

TESTER_TEST("[CORE]: Scheduler Submit")
{
	constexpr U32 TASK_COUNT = 64;

	Platform_Mutex *mutex = platform_mutex_init();
	Scheduler_Test_Task_Context context = {
		.mutex = mutex
	};

	Scheduler *scheduler = scheduler_init(Scheduler_Desc {
		.worker_count = 2,
		.initial_task_queue_capacity = TASK_COUNT
	});

	for (U32 i = 0; i < TASK_COUNT; ++i)
	{
		scheduler_submit(scheduler, Scheduler_Task {
			.function = _scheduler_test_task,
			.data = &context
		});
	}

	scheduler_wait_all(scheduler);

	platform_mutex_lock(mutex);
	U32 finished_count = context.finished_count;
	platform_mutex_unlock(mutex);

	TESTER_CHECK(finished_count == TASK_COUNT);
	scheduler_deinit(scheduler);
	platform_mutex_deinit(mutex);
}

TESTER_TEST("[CORE]: Scheduler Submit Batch")
{
	constexpr U32 TASK_COUNT = 64;

	Platform_Mutex *mutex = platform_mutex_init();
	Scheduler_Test_Task_Context context = {
		.mutex = mutex
	};
	Scheduler_Task tasks[TASK_COUNT];

	for (U32 i = 0; i < TASK_COUNT; ++i)
	{
		tasks[i] = Scheduler_Task {
			.function = _scheduler_test_task,
			.data = &context
		};
	}

	Scheduler *scheduler = scheduler_init(Scheduler_Desc {
		.worker_count = 2,
		.initial_task_queue_capacity = TASK_COUNT
	});

	scheduler_submit(scheduler, span_init(tasks));
	scheduler_wait_all(scheduler);

	platform_mutex_lock(mutex);
	U32 finished_count = context.finished_count;
	platform_mutex_unlock(mutex);

	TESTER_CHECK(finished_count == TASK_COUNT);
	scheduler_deinit(scheduler);
	platform_mutex_deinit(mutex);
}

TESTER_TEST("[CORE]: Scheduler Deinit Drains Tasks")
{
	constexpr U32 TASK_COUNT = 64;

	Platform_Mutex *mutex = platform_mutex_init();
	Scheduler_Test_Task_Context context = {
		.mutex = mutex
	};

	Scheduler *scheduler = scheduler_init(Scheduler_Desc {
		.worker_count = 2,
		.initial_task_queue_capacity = TASK_COUNT
	});

	for (U32 i = 0; i < TASK_COUNT; ++i)
	{
		scheduler_submit(scheduler, Scheduler_Task {
			.function = _scheduler_test_task,
			.data = &context
		});
	}

	scheduler_deinit(scheduler);

	platform_mutex_lock(mutex);
	U32 finished_count = context.finished_count;
	platform_mutex_unlock(mutex);

	TESTER_CHECK(finished_count == TASK_COUNT);
	platform_mutex_deinit(mutex);
}

struct Scheduler_Test_Parallel_For_Context
{
	Platform_Mutex *mutex;
	bool *visited;
	U32 visited_count;
	U32 index_sum;
};

inline static void
_scheduler_test_parallel_for(U32 begin, U32 end, void *data)
{
	Scheduler_Test_Parallel_For_Context *context = (Scheduler_Test_Parallel_For_Context *)data;

	for (U32 i = begin; i < end; ++i)
	{
		platform_mutex_lock(context->mutex);
		validate(!context->visited[i], "[SCHEDULER][TEST]: Parallel for index was visited more than once.");
		context->visited[i] = true;
		++context->visited_count;
		context->index_sum += i;
		platform_mutex_unlock(context->mutex);
	}
}

TESTER_TEST("[CORE]: Scheduler Parallel For")
{
	constexpr U32 ITEM_COUNT = 257;
	constexpr U32 EXPECTED_INDEX_SUM = ITEM_COUNT * (ITEM_COUNT - 1) / 2;

	bool visited[ITEM_COUNT] = {};
	Platform_Mutex *mutex = platform_mutex_init();
	Scheduler_Test_Parallel_For_Context context = {
		.mutex = mutex,
		.visited = visited
	};

	memory::Allocator *temp_allocator = memory::temp_allocator();
	memory::Arena_Allocator_Mark temp_allocator_mark = memory::temp_allocator_mark();
	DEFER(memory::temp_allocator_reset_to_mark(temp_allocator_mark));
	Memory_Block expected_temp_block = memory::allocate(temp_allocator, 16, alignof(U8));
	memory::temp_allocator_reset_to_mark(temp_allocator_mark);

	Scheduler *scheduler = scheduler_init(Scheduler_Desc {
		.worker_count = 2,
		.initial_task_queue_capacity = 16
	});

	scheduler_parallel_for(scheduler, Scheduler_Parallel_For_Desc {
		.count = ITEM_COUNT,
		.chunk_size = 32,
		.function = _scheduler_test_parallel_for,
		.data = &context
	});

	platform_mutex_lock(mutex);
	U32 visited_count = context.visited_count;
	U32 index_sum = context.index_sum;
	platform_mutex_unlock(mutex);
	Memory_Block actual_temp_block = memory::allocate(temp_allocator, 16, alignof(U8));

	for (U32 i = 0; i < ITEM_COUNT; ++i)
		TESTER_CHECK(visited[i]);
	TESTER_CHECK(visited_count == ITEM_COUNT);
	TESTER_CHECK(index_sum == EXPECTED_INDEX_SUM);
	TESTER_CHECK(actual_temp_block.data == expected_temp_block.data);

	scheduler_deinit(scheduler);
	platform_mutex_deinit(mutex);
}

struct Scheduler_Test_Blocking_Task_Context
{
	Platform_Mutex *mutex;
	Platform_Condition_Variable *condition_variable;
	bool started;
	bool release;
	bool finished;
};

inline static void
_scheduler_test_blocking_task(void *data)
{
	Scheduler_Test_Blocking_Task_Context *context = (Scheduler_Test_Blocking_Task_Context *)data;

	platform_mutex_lock(context->mutex);
	context->started = true;
	platform_condition_variable_signal(context->condition_variable);
	while (!context->release)
		platform_condition_variable_wait(context->condition_variable, context->mutex);
	context->finished = true;
	platform_mutex_unlock(context->mutex);
}

TESTER_TEST("[CORE]: Scheduler Wait Group")
{
	constexpr U32 TASK_COUNT = 64;

	Platform_Mutex *task_mutex = platform_mutex_init();
	Scheduler_Test_Task_Context task_context = {
		.mutex = task_mutex
	};

	Platform_Mutex *blocking_mutex = platform_mutex_init();
	Platform_Condition_Variable *blocking_condition_variable = platform_condition_variable_init();
	Scheduler_Test_Blocking_Task_Context blocking_context = {
		.mutex = blocking_mutex,
		.condition_variable = blocking_condition_variable
	};

	Scheduler *scheduler = scheduler_init(Scheduler_Desc {
		.worker_count = 2
	});
	Scheduler_Group *group = scheduler_group_init(scheduler);

	scheduler_submit(scheduler, Scheduler_Task {
		.function = _scheduler_test_blocking_task,
		.data = &blocking_context
	});

	platform_mutex_lock(blocking_mutex);
	while (!blocking_context.started)
		platform_condition_variable_wait(blocking_condition_variable, blocking_mutex);
	platform_mutex_unlock(blocking_mutex);

	Scheduler_Task tasks[TASK_COUNT];
	for (U32 i = 0; i < TASK_COUNT; ++i)
	{
		tasks[i] = Scheduler_Task {
			.function = _scheduler_test_task,
			.data = &task_context
		};
	}
	scheduler_submit(scheduler, span_init(tasks), group);

	scheduler_wait_group(scheduler, group);

	platform_mutex_lock(task_mutex);
	U32 finished_count = task_context.finished_count;
	platform_mutex_unlock(task_mutex);

	platform_mutex_lock(blocking_mutex);
	bool blocking_task_finished = blocking_context.finished;
	blocking_context.release = true;
	platform_condition_variable_signal(blocking_condition_variable);
	platform_mutex_unlock(blocking_mutex);

	TESTER_CHECK(finished_count == TASK_COUNT);
	TESTER_CHECK(!blocking_task_finished);

	scheduler_wait_all(scheduler);

	scheduler_group_deinit(scheduler, group);
	scheduler_deinit(scheduler);
	platform_condition_variable_deinit(blocking_condition_variable);
	platform_mutex_deinit(blocking_mutex);
	platform_mutex_deinit(task_mutex);
}

struct Scheduler_Test_Waiting_Task_Context
{
	Scheduler *scheduler;
	Scheduler_Group *group;
	Platform_Mutex *mutex;
	U32 child_task_count;
	U32 finished_count;
	bool parent_finished;
};

inline static void
_scheduler_test_child_task(void *data)
{
	Scheduler_Test_Waiting_Task_Context *context = (Scheduler_Test_Waiting_Task_Context *)data;

	platform_mutex_lock(context->mutex);
	++context->finished_count;
	platform_mutex_unlock(context->mutex);
}

inline static void
_scheduler_test_waiting_parent_task(void *data)
{
	Scheduler_Test_Waiting_Task_Context *context = (Scheduler_Test_Waiting_Task_Context *)data;

	for (U32 i = 0; i < context->child_task_count; ++i)
	{
		scheduler_submit(context->scheduler, Scheduler_Task {
			.function = _scheduler_test_child_task,
			.data = context
		}, context->group);
	}

	scheduler_wait_group(context->scheduler, context->group);

	platform_mutex_lock(context->mutex);
	context->parent_finished = true;
	platform_mutex_unlock(context->mutex);
}

TESTER_TEST("[CORE]: Scheduler Worker Wait Group")
{
	constexpr U32 TASK_COUNT = 64;

	Platform_Mutex *mutex = platform_mutex_init();
	Scheduler *scheduler = scheduler_init(Scheduler_Desc {
		.worker_count = 1
	});
	Scheduler_Group *group = scheduler_group_init(scheduler);
	Scheduler_Test_Waiting_Task_Context context = {
		.scheduler = scheduler,
		.group = group,
		.mutex = mutex,
		.child_task_count = TASK_COUNT
	};

	scheduler_submit(scheduler, Scheduler_Task {
		.function = _scheduler_test_waiting_parent_task,
		.data = &context
	});

	scheduler_wait_all(scheduler);

	platform_mutex_lock(mutex);
	U32 finished_count = context.finished_count;
	bool parent_finished = context.parent_finished;
	platform_mutex_unlock(mutex);

	TESTER_CHECK(finished_count == TASK_COUNT);
	TESTER_CHECK(parent_finished);

	scheduler_group_deinit(scheduler, group);
	scheduler_deinit(scheduler);
	platform_mutex_deinit(mutex);
}

TESTER_TEST("[CORE]: Arena_Allocator")
{
	U64 page_size = platform_virtual_memory_get_page_size();

	memory::Arena_Allocator *arena = memory::arena_allocator_init(1024);
	DEFER(memory::arena_allocator_deinit(arena));

	Memory_Block a = memory::arena_allocator_allocate(arena, 4, 1);
	Memory_Block b = memory::arena_allocator_allocate(arena, 8, 1);

	TESTER_CHECK(a.data != nullptr);
	TESTER_CHECK(b.data != nullptr);

	TESTER_CHECK(memory::arena_allocator_get_used(arena) == 12);
	TESTER_CHECK(memory::arena_allocator_get_peak(arena) == 12);

	arena_allocator_clear(arena);

	TESTER_CHECK(memory::arena_allocator_get_used(arena) == 0);
	TESTER_CHECK(memory::arena_allocator_get_peak(arena) == 12);

	Memory_Block reused = memory::arena_allocator_allocate(arena, 4, 1);
	TESTER_CHECK(reused.data == a.data);
	arena_allocator_clear(arena);

	Memory_Block large = memory::arena_allocator_allocate(arena, page_size + 32, 1);
	U8 *large_bytes = (U8 *)large.data;
	large_bytes[0] = 1;
	large_bytes[page_size + 31] = 2;

	TESTER_CHECK(large_bytes[0] == 1);
	TESTER_CHECK(large_bytes[page_size + 31] == 2);
	TESTER_CHECK(memory::arena_allocator_get_used(arena) == page_size + 32);
	TESTER_CHECK(memory::arena_allocator_get_peak(arena) == page_size + 32);

	arena_allocator_clear(arena);

	TESTER_CHECK(memory::arena_allocator_get_used(arena) == 0);
	TESTER_CHECK(memory::arena_allocator_get_peak(arena) == page_size + 32);

	Memory_Block after_large_clear = memory::arena_allocator_allocate(arena, 4, 1);
	TESTER_CHECK(after_large_clear.data == large.data);
}

TESTER_TEST("[CORE]: Arena_Allocator_Clear_Growth")
{
	U64 page_size = platform_virtual_memory_get_page_size();
	U64 half_page = page_size / 2;

	memory::Arena_Allocator *arena = memory::arena_allocator_init(1024);
	DEFER(memory::arena_allocator_deinit(arena));

	Memory_Block first = memory::arena_allocator_allocate(arena, half_page, 1);
	Memory_Block second = memory::arena_allocator_allocate(arena, half_page, 1);
	TESTER_CHECK(first.data != nullptr);
	TESTER_CHECK(second.data != nullptr);
	TESTER_CHECK(memory::arena_allocator_get_peak(arena) == page_size);

	arena_allocator_clear(arena);
	TESTER_CHECK(memory::arena_allocator_get_used(arena) == 0);
	TESTER_CHECK(memory::arena_allocator_get_peak(arena) == page_size);

	Memory_Block retained = memory::arena_allocator_allocate(arena, page_size, 1);
	U8 *bytes = (U8 *)retained.data;
	bytes[0] = 1;
	bytes[page_size - 1] = 2;
	TESTER_CHECK(bytes[0] == 1);
	TESTER_CHECK(bytes[page_size - 1] == 2);
	TESTER_CHECK(memory::arena_allocator_get_used(arena) == page_size);
}

TESTER_TEST("[CORE]: Arena_Allocator_Mark")
{
	U64 page_size = platform_virtual_memory_get_page_size();

	memory::Arena_Allocator *arena = memory::arena_allocator_init(64);
	DEFER(memory::arena_allocator_deinit(arena));

	Memory_Block base = memory::arena_allocator_allocate(arena, 16, 1);
	TESTER_CHECK(base.data != nullptr);
	TESTER_CHECK(memory::arena_allocator_get_used(arena) == 16);
	TESTER_CHECK(memory::arena_allocator_get_peak(arena) == 16);

	memory::Arena_Allocator_Mark mark = memory::arena_allocator_mark(arena);
	Memory_Block tail = memory::arena_allocator_allocate(arena, 8, 1);
	TESTER_CHECK(tail.data != nullptr);
	TESTER_CHECK(memory::arena_allocator_get_used(arena) == 24);
	TESTER_CHECK(memory::arena_allocator_get_peak(arena) == 24);

	memory::arena_allocator_reset_to_mark(arena, mark);
	TESTER_CHECK(memory::arena_allocator_get_used(arena) == 16);
	TESTER_CHECK(memory::arena_allocator_get_peak(arena) == 24);

	Memory_Block reused_tail = memory::arena_allocator_allocate(arena, 8, 1);
	TESTER_CHECK(reused_tail.data == tail.data);
	TESTER_CHECK(memory::arena_allocator_get_used(arena) == 24);

	memory::Arena_Allocator_Mark cross_node_mark = memory::arena_allocator_mark(arena);
	U64 large_size = page_size * 2;
	Memory_Block large = memory::arena_allocator_allocate(arena, large_size, 1);
	TESTER_CHECK(large.data != nullptr);
	U8 *large_bytes = (U8 *)large.data;
	large_bytes[0] = 3;
	large_bytes[large_size - 1] = 4;
	TESTER_CHECK(large_bytes[0] == 3);
	TESTER_CHECK(large_bytes[large_size - 1] == 4);
	TESTER_CHECK(memory::arena_allocator_get_used(arena) == 24 + large_size);
	TESTER_CHECK(memory::arena_allocator_get_peak(arena) == 24 + large_size);

	memory::arena_allocator_reset_to_mark(arena, cross_node_mark);
	TESTER_CHECK(memory::arena_allocator_get_used(arena) == 24);
	TESTER_CHECK(memory::arena_allocator_get_peak(arena) == 24 + large_size);

	Memory_Block after_cross_node_reset = memory::arena_allocator_allocate(arena, 8, 1);
	TESTER_CHECK(after_cross_node_reset.data != nullptr);
	TESTER_CHECK(after_cross_node_reset.data != large.data);
	TESTER_CHECK(memory::arena_allocator_get_used(arena) == 32);
	TESTER_CHECK(memory::arena_allocator_get_peak(arena) == 24 + large_size);
}

TESTER_TEST("[CORE]: Pool_Allocator")
{
	struct Entity
	{
		F32 x, y, z;
	};

	memory::Pool_Allocator *pool = memory::pool_allocator_init(sizeof(Entity), 10);
	DEFER(memory::pool_allocator_deinit(pool));

	Entity *e1 = (Entity *)memory::pool_allocator_allocate(pool).data;
	TESTER_CHECK(e1 != nullptr);
	*e1 = Entity{1.0f, 2.0f, 3.0f};
	memory::pool_allocator_deallocate(pool, Memory_Block{e1, sizeof(Entity)});

	Entity *e2 = (Entity *)memory::pool_allocator_allocate(pool).data;
	TESTER_CHECK(e2 == e1);

	Entity *e3 = (Entity *)memory::pool_allocator_allocate(pool).data;
	memory::pool_allocator_deallocate(pool, Memory_Block{e3, sizeof(Entity)});
	memory::pool_allocator_deallocate(pool, Memory_Block{e2, sizeof(Entity)});

	Entity *p4 = (Entity *)memory::pool_allocator_allocate(pool).data;
	TESTER_CHECK(p4 == e2);

	Entity *p5 = (Entity *)memory::pool_allocator_allocate(pool).data;
	TESTER_CHECK(p5 == e3);
}

TESTER_TEST("[CORE]: Memory_Block allocation")
{
	struct Tracking_Allocator final : memory::Allocator
	{
		bool allocated;
		bool deallocated;

		Memory_Block
		allocate(U64 size, U64 alignment) override
		{
			allocated = true;
			return memory::heap_allocator()->allocate(size, alignment);
		}

		void
		deallocate(Memory_Block block) override
		{
			deallocated = true;
			memory::heap_allocator()->deallocate(block);
		}
	};

	Memory_Block block = memory::allocate(sizeof(I32) * 4, alignof(I32));
	DEFER(memory::deallocate(block));

	TESTER_CHECK(block.data != nullptr);
	TESTER_CHECK(block.size == sizeof(I32) * 4);

	I32 *values = (I32 *)block.data;
	for (U64 i = 0; i < 4; ++i)
		values[i] = (I32)i;

	for (U64 i = 0; i < 4; ++i)
		TESTER_CHECK(values[i] == (I32)i);

	I32 *single = memory::allocate<I32>();
	DEFER(memory::deallocate(single));
	*single = 42;
	TESTER_CHECK(*single == 42);

	Tracking_Allocator tracking_allocator = {};
	Memory_Block tracked_block = memory::allocate(&tracking_allocator, sizeof(I32), alignof(I32));
	TESTER_CHECK(tracking_allocator.allocated);
	memory::deallocate(&tracking_allocator, tracked_block);
	TESTER_CHECK(tracking_allocator.deallocated);

	memory::Allocator *temp = memory::temp_allocator();
	TESTER_CHECK(temp != nullptr);
	memory::Arena_Allocator_Mark mark = memory::temp_allocator_mark();
	Memory_Block temp_block = memory::allocate(temp, 16, alignof(U8));
	TESTER_CHECK(temp_block.data != nullptr);
	memory::temp_allocator_reset_to_mark(mark);

	Memory_Block temp_clear_block = memory::allocate(temp, 16, alignof(U8));
	TESTER_CHECK(temp_clear_block.data != nullptr);
	memory::temp_allocator_clear();
	Memory_Block temp_after_clear_block = memory::allocate(temp, 16, alignof(U8));
	TESTER_CHECK(temp_after_clear_block.data != nullptr);
	memory::temp_allocator_clear();
}

TESTER_TEST("[CORE]: Temp_Allocator_Mark")
{
	memory::Allocator *temp = memory::temp_allocator();
	memory::Arena_Allocator_Mark start_mark = memory::temp_allocator_mark();
	DEFER(memory::temp_allocator_reset_to_mark(start_mark));

	Memory_Block first = memory::allocate(temp, 16, alignof(U8));
	TESTER_CHECK(first.data != nullptr);

	memory::Arena_Allocator_Mark mark = memory::temp_allocator_mark();
	Memory_Block second = memory::allocate(temp, 16, alignof(U8));
	TESTER_CHECK(second.data != nullptr);

	memory::temp_allocator_reset_to_mark(mark);
	Memory_Block second_reused = memory::allocate(temp, 16, alignof(U8));
	TESTER_CHECK(second_reused.data == second.data);

	memory::temp_allocator_reset_to_mark(start_mark);
	Memory_Block first_reused = memory::allocate(temp, 16, alignof(U8));
	TESTER_CHECK(first_reused.data == first.data);
}

struct Temp_Allocator_Thread_Test_Context
{
	memory::Allocator *allocator;
};

inline static void
_temp_allocator_thread_test(void *data)
{
	Temp_Allocator_Thread_Test_Context *context = (Temp_Allocator_Thread_Test_Context *)data;
	context->allocator = memory::temp_allocator();
}

TESTER_TEST("[CORE]: Temp_Allocator Thread Local")
{
	memory::Allocator *main_allocator = memory::temp_allocator();
	Temp_Allocator_Thread_Test_Context context = {};
	Platform_Thread *thread = platform_thread_init(Platform_Thread_Desc {
		.function = _temp_allocator_thread_test,
		.data = &context
	});
	platform_thread_deinit(thread);

	TESTER_CHECK(context.allocator != nullptr);
	TESTER_CHECK(context.allocator != main_allocator);
}

TESTER_TEST("[CORE]: Virtual_Memory")
{
	U64 page_size = platform_virtual_memory_get_page_size();
	TESTER_CHECK(page_size > 0);
	TESTER_CHECK((page_size & (page_size - 1)) == 0);

	Memory_Block reserved = platform_virtual_memory_reserve(page_size);
	TESTER_CHECK(reserved.data != nullptr);
	TESTER_CHECK(reserved.size == page_size);
	TESTER_CHECK(platform_virtual_memory_commit(reserved));

	U8 *bytes = (U8 *)reserved.data;
	bytes[0] = 1;
	bytes[page_size - 1] = 2;
	TESTER_CHECK(bytes[0] == 1);
	TESTER_CHECK(bytes[page_size - 1] == 2);

	TESTER_CHECK(platform_virtual_memory_decommit(reserved));
	TESTER_CHECK(platform_virtual_memory_commit(reserved));
	platform_virtual_memory_release(reserved);
}

inline static Result<I32>
_result_test_with_default_error_pseudo_disk_read(bool success)
{
	if (success)
		return 1;
	return Error{"Could not read from disk."};
}

enum class PSEUDO_DISK_READ_RESULT_CODE { OK, NOT_OK };

inline static Result<I32, PSEUDO_DISK_READ_RESULT_CODE>
_result_test_with_custom_error_pseudo_disk_read(bool success)
{
	if (success)
		return 1;
	return PSEUDO_DISK_READ_RESULT_CODE::NOT_OK;
}

TESTER_TEST("[CORE]: Result")
{
	// ("default error - success")
	{
		auto [result, error] = _result_test_with_default_error_pseudo_disk_read(true);
		TESTER_CHECK(error == false);
		TESTER_CHECK(result == 1);
	}

	// ("default error - failure")
	{
		auto [result, error] = _result_test_with_default_error_pseudo_disk_read(false);
		TESTER_CHECK(error == true);
	}

	// ("custom error - success")
	{
		auto [result, error] = _result_test_with_custom_error_pseudo_disk_read(true);
		TESTER_CHECK(error == PSEUDO_DISK_READ_RESULT_CODE::OK);
		TESTER_CHECK(result == 1);
	}

	// ("custom error - failure")
	{
		auto [result, error] = _result_test_with_custom_error_pseudo_disk_read(false);
		TESTER_CHECK(error == PSEUDO_DISK_READ_RESULT_CODE::NOT_OK);
	}
}

TESTER_TEST("[CORE]: JSON")
{
	// TODO: Add json_value_object_find().
	// ("parse string")
	{
		auto json = R"""(
			{
				"name": "Mist",
				"nil": null,
				"right": true,
				"wrong": false,
				"number": 123.456,
				"array": [
					1, false
				],
				"sub_object": {
					"name": "sub_object"
				}
			}
		)""";

		auto [value, error] = json_value_from_string(json, memory::temp_allocator());
		if (error)
			log_error("{}", error.message.data);

		TESTER_CHECK(error == false);
		TESTER_CHECK(value.kind == JSON_VALUE_KIND_OBJECT);
		TESTER_CHECK(value.as_object.count == 7);

		{
			auto name_entry = hash_table_find(value.as_object, string_literal("name"));
			TESTER_CHECK(name_entry != nullptr);
			TESTER_CHECK(name_entry->key == "name");
			TESTER_CHECK(name_entry->value.kind == JSON_VALUE_KIND_STRING);
			TESTER_CHECK(name_entry->value.as_string == "Mist");
		}
		{
			auto nil_entry = hash_table_find(value.as_object, string_literal("nil"));
			TESTER_CHECK(nil_entry != nullptr);
			TESTER_CHECK(nil_entry->key == "nil");
			TESTER_CHECK(nil_entry->value.kind == JSON_VALUE_KIND_NULL);
		}
		{
			auto right_entry = hash_table_find(value.as_object, string_literal("right"));
			TESTER_CHECK(right_entry != nullptr);
			TESTER_CHECK(right_entry->key == "right");
			TESTER_CHECK(right_entry->value.kind == JSON_VALUE_KIND_BOOL);
			TESTER_CHECK(right_entry->value.as_bool == true);
		}
		{
			auto wrong_entry = hash_table_find(value.as_object, string_literal("wrong"));
			TESTER_CHECK(wrong_entry != nullptr);
			TESTER_CHECK(wrong_entry->key == "wrong");
			TESTER_CHECK(wrong_entry->value.kind == JSON_VALUE_KIND_BOOL);
			TESTER_CHECK(wrong_entry->value.as_bool == false);
		}
		{
			auto number_entry = hash_table_find(value.as_object, string_literal("number"));
			TESTER_CHECK(number_entry != nullptr);
			TESTER_CHECK(number_entry->key == "number");
			TESTER_CHECK(number_entry->value.kind == JSON_VALUE_KIND_NUMBER);
			TESTER_CHECK(number_entry->value.as_number == 123.456);
		}
		{
			auto array_entry = hash_table_find(value.as_object, string_literal("array"));
			TESTER_CHECK(array_entry != nullptr);
			TESTER_CHECK(array_entry->key == "array");
			TESTER_CHECK(array_entry->value.as_array.count == 2);
			TESTER_CHECK(array_entry->value.as_array[0].kind == JSON_VALUE_KIND_NUMBER);
			TESTER_CHECK(array_entry->value.as_array[0].as_number == 1);
			TESTER_CHECK(array_entry->value.as_array[1].kind == JSON_VALUE_KIND_BOOL);
			TESTER_CHECK(array_entry->value.as_array[1].as_bool == false);
		}
		{
			auto sub_object_entry = hash_table_find(value.as_object, string_literal("sub_object"));
			TESTER_CHECK(sub_object_entry != nullptr);
			TESTER_CHECK(sub_object_entry->key == "sub_object");
			TESTER_CHECK(sub_object_entry->value.kind == JSON_VALUE_KIND_OBJECT);

			auto sub_object_name_entry = hash_table_find(sub_object_entry->value.as_object, string_literal("name"));
			TESTER_CHECK(sub_object_name_entry != nullptr);
			TESTER_CHECK(sub_object_name_entry->key == "name");
			TESTER_CHECK(sub_object_name_entry->value.kind == JSON_VALUE_KIND_STRING);
			TESTER_CHECK(sub_object_name_entry->value.as_string == "sub_object");
		}
	}

	// ("clone")
	{
		auto json =
R"""({
	"name": "Mist",
	"nil": null,
	"right": true,
	"wrong": false,
	"number": 123.456,
	"array": [
		1,
		false
	],
	"sub_object": {
		"name": "sub_object"
	}
})""";

		auto [value, error] = json_value_from_string(json, memory::temp_allocator());
		if (error)
			log_error("{}", error.message.data);

		auto value_copy        = clone(value, memory::temp_allocator());
		auto [value_string, _] = json_value_to_string(value_copy, memory::temp_allocator());
		TESTER_CHECK(value_string == json);
	}
}

TESTER_TEST("Base64")
{
	// ("Encode")
	{
		String result = base64_encode("Hello");
		DEFER(string_deinit(result));
		TESTER_CHECK(result == "SGVsbG8=");
	}

	// ("Decode")
	{
		String result = base64_decode("SGVsbG8=");
		DEFER(string_deinit(result));
		TESTER_CHECK(result == "Hello");
	}
}