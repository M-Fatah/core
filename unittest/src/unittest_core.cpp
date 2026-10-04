#include <core/tester.h>
#include <core/atomic.h>
#include <core/command_line.h>
#include <core/json.h>
#include <core/math/f64.h>
#include <core/base64.h>
#include <core/log.h>
#include <core/result.h>
#include <core/scheduler.h>
#include <core/validate.h>
#include <core/memory/allocator.h>
#include <core/memory/pool_allocator.h>
#include <core/memory/arena_allocator.h>
#include <core/platform/platform.h>

#include <errno.h>

TESTER_TEST("[CORE]: Command Line")
{
	static Command_Line_Option_Desc option_descs[] = {
		Command_Line_Option_Desc {.name = "help",    .short_name = 'h'},
		Command_Line_Option_Desc {.name = "verbose", .short_name = 'v'},
		Command_Line_Option_Desc {.name = "output",  .short_name = 'o', .requires_value = true},
		Command_Line_Option_Desc {.name = "include", .short_name = 'I', .requires_value = true},
		Command_Line_Option_Desc {.name = "threads", .short_name = 'j', .requires_value = true},
	};
	char executable[] = "compiler";
	char main_file[] = "main.cpp";
	char math_file[] = "math.cpp";
	char output[] = "--output=game.exe";
	char include_core[] = "-Icore";
	char include[] = "-I";
	char deps[] = "deps";
	char threads[] = "--threads";
	char thread_count[] = "8";
	char verbose[] = "--verbose";
	char verbose_short[] = "-v";
	char separator[] = "--";
	char literal[] = "--literal";
	char *arguments[] = {
		executable,
		main_file,
		math_file,
		output,
		include_core,
		include,
		deps,
		threads,
		thread_count,
		verbose,
		verbose_short,
		separator,
		literal
	};

	Command_Line command_line = command_line_init(arguments, (I32)slice_from(arguments).count, option_descs);
	DEFER(command_line_deinit(command_line));

	TESTER_CHECK(!command_line_has_errors(command_line));
	TESTER_CHECK(slice_from(command_line.executable) == slice_from("compiler"));
	TESTER_CHECK(command_line_has_option(command_line, "output"));
	TESTER_CHECK(command_line_has_option(command_line, "verbose"));
	TESTER_CHECK(!command_line_has_option(command_line, "help"));

	TESTER_CHECK(command_line.options.count == 4);

	Slice<const char *> output_values = command_line_get_option_values(command_line, "output");
	TESTER_CHECK(output_values.count == 1);
	TESTER_CHECK(slice_from(output_values[0]) == slice_from("game.exe"));

	Slice<const char *> thread_values = command_line_get_option_values(command_line, "threads");
	TESTER_CHECK(thread_values.count == 1);
	TESTER_CHECK(slice_from(thread_values[0]) == slice_from("8"));

	Slice<const char *> include_values = command_line_get_option_values(command_line, "include");
	TESTER_CHECK(include_values.count == 2);
	TESTER_CHECK(slice_from(include_values[0]) == slice_from("core"));
	TESTER_CHECK(slice_from(include_values[1]) == slice_from("deps"));

	Slice<const char *> verbose_values = command_line_get_option_values(command_line, "verbose");
	TESTER_CHECK(verbose_values.count == 2);
	TESTER_CHECK(verbose_values[0] == nullptr);
	TESTER_CHECK(verbose_values[1] == nullptr);
	TESTER_CHECK(command_line_get_option_values(command_line, "help").count == 0);

	Slice<const char *> positionals = slice_from(command_line.positionals);
	TESTER_CHECK(positionals.count == 3);
	TESTER_CHECK(slice_from(positionals[0]) == slice_from("main.cpp"));
	TESTER_CHECK(slice_from(positionals[1]) == slice_from("math.cpp"));
	TESTER_CHECK(slice_from(positionals[2]) == slice_from("--literal"));
}

TESTER_TEST("[CORE]: Command Line Argc Argv")
{
	static Command_Line_Option_Desc option_descs[] = {
		Command_Line_Option_Desc {.name = "help", .short_name = 'h'},
	};
	char executable[] = "tool";
	char help[] = "--help";
	char *arguments[] = {executable, help};

	Command_Line command_line = command_line_init(arguments, (I32)slice_from(arguments).count, option_descs);
	DEFER(command_line_deinit(command_line));

	TESTER_CHECK(!command_line_has_errors(command_line));
	TESTER_CHECK(slice_from(command_line.executable) == slice_from("tool"));
	TESTER_CHECK(command_line_has_option(command_line, "help"));
	Slice<const char *> values = command_line_get_option_values(command_line, "help");
	TESTER_CHECK(values.count == 1);
	TESTER_CHECK(values[0] == nullptr);
}

TESTER_TEST("[CORE]: Command Line Grammar")
{
	static Command_Line_Option_Desc option_descs[] = {
		Command_Line_Option_Desc {.name = "help",    .short_name = 'h'},
		Command_Line_Option_Desc {.name = "verbose", .short_name = 'v'},
		Command_Line_Option_Desc {.name = "output",  .short_name = 'o', .requires_value = true},
	};
	char executable[] = "tool";
	char dash[] = "-";
	char output[] = "--output";
	char option_value[] = "--verbose";
	char separator[] = "--";
	char literal[] = "--help";
	char *arguments[] = {
		executable,
		dash,
		output,
		option_value,
		separator,
		literal
	};

	Command_Line command_line = command_line_init(arguments, (I32)slice_from(arguments).count, option_descs);
	DEFER(command_line_deinit(command_line));

	TESTER_CHECK(!command_line_has_errors(command_line));
	TESTER_CHECK(command_line_has_option(command_line, "output"));
	TESTER_CHECK(!command_line_has_option(command_line, "verbose"));
	TESTER_CHECK(!command_line_has_option(command_line, "help"));

	Slice<const char *> output_values = command_line_get_option_values(command_line, "output");
	TESTER_CHECK(output_values.count == 1);
	TESTER_CHECK(slice_from(output_values[0]) == "--verbose");

	TESTER_CHECK(command_line.positionals.count == 2);
	TESTER_CHECK(slice_from(command_line.positionals[0]) == "-");
	TESTER_CHECK(slice_from(command_line.positionals[1]) == "--help");
}

TESTER_TEST("[CORE]: Command Line Empty")
{
	Command_Line command_line = command_line_init(nullptr, 0, {});
	DEFER(command_line_deinit(command_line));

	TESTER_CHECK(command_line.executable == nullptr);
	TESTER_CHECK(command_line.options.count == 0);
	TESTER_CHECK(command_line.positionals.count == 0);
	TESTER_CHECK(!command_line_has_errors(command_line));
}

TESTER_TEST("[CORE]: Command Line Errors")
{
	static Command_Line_Option_Desc option_descs[] = {
		Command_Line_Option_Desc {.name = "help", .short_name = 'h'},
		Command_Line_Option_Desc {.name = "output", .short_name = 'o', .requires_value = true},
	};
	char executable[] = "tool";
	char unknown[] = "--unknown";
	char unknown_short[] = "-x";
	char help[] = "--help=true";
	char help_short[] = "-htrue";
	char empty_output[] = "--output=";
	char output_short[] = "-o";
	char empty_value[] = "";
	char output[] = "--output";
	char *arguments[] = {
		executable,
		unknown,
		unknown_short,
		help,
		help_short,
		empty_output,
		output_short,
		empty_value,
		output
	};

	Command_Line command_line = command_line_init(arguments, (I32)slice_from(arguments).count, option_descs);
	DEFER(command_line_deinit(command_line));

	TESTER_CHECK(command_line_has_errors(command_line));
	Slice<const Command_Line_Error> errors = slice_from(command_line.errors);
	TESTER_CHECK(errors.count == 7);
	TESTER_CHECK(errors[0].code == COMMAND_LINE_ERROR_UNKNOWN_OPTION);
	TESTER_CHECK(slice_from(errors[0].argument) == slice_from("--unknown"));
	TESTER_CHECK(errors[1].code == COMMAND_LINE_ERROR_UNKNOWN_OPTION);
	TESTER_CHECK(slice_from(errors[1].argument) == slice_from("-x"));
	TESTER_CHECK(errors[2].code == COMMAND_LINE_ERROR_UNEXPECTED_VALUE);
	TESTER_CHECK(slice_from(errors[2].argument) == slice_from("--help=true"));
	TESTER_CHECK(errors[3].code == COMMAND_LINE_ERROR_UNEXPECTED_VALUE);
	TESTER_CHECK(slice_from(errors[3].argument) == slice_from("-htrue"));
	TESTER_CHECK(errors[4].code == COMMAND_LINE_ERROR_MISSING_VALUE);
	TESTER_CHECK(slice_from(errors[4].argument) == slice_from("--output="));
	TESTER_CHECK(errors[5].code == COMMAND_LINE_ERROR_MISSING_VALUE);
	TESTER_CHECK(slice_from(errors[5].argument) == slice_from("-o"));
	TESTER_CHECK(errors[6].code == COMMAND_LINE_ERROR_MISSING_VALUE);
	TESTER_CHECK(slice_from(errors[6].argument) == slice_from("--output"));
}

TESTER_TEST("[CORE]: Atomic")
{
	Atomic<U32> value = atomic_init((U32)7);
	TESTER_CHECK(atomic_load(value) == 7);

	atomic_store(value, 11, COMPILER_ATOMIC_MEMORY_ORDER_RELEASE);
	TESTER_CHECK(atomic_load(value, COMPILER_ATOMIC_MEMORY_ORDER_ACQUIRE) == 11);
	TESTER_CHECK(atomic_exchange(value, 19) == 11);
	TESTER_CHECK(atomic_load(value) == 19);

	U32 expected = 19;
	TESTER_CHECK(atomic_compare_exchange(value, expected, 23));
	TESTER_CHECK(expected == 19);
	TESTER_CHECK(atomic_load(value) == 23);

	expected = 19;
	TESTER_CHECK(!atomic_compare_exchange(value, expected, 31));
	TESTER_CHECK(expected == 23);
	TESTER_CHECK(atomic_load(value) == 23);

	Atomic<U64> counter = atomic_init((U64)40);
	TESTER_CHECK(atomic_fetch_add(counter, (U64)2, COMPILER_ATOMIC_MEMORY_ORDER_RELAXED) == 40);
	TESTER_CHECK(atomic_load(counter) == 42);
	TESTER_CHECK(atomic_fetch_sub(counter, (U64)10, COMPILER_ATOMIC_MEMORY_ORDER_RELAXED) == 42);
	TESTER_CHECK(atomic_load(counter) == 32);
}

struct Atomic_Test_Thread_Context
{
	Atomic<U64> *counter;
	U32 iteration_count;
};

inline static void
_atomic_test_thread_main(void *data)
{
	Atomic_Test_Thread_Context *context = (Atomic_Test_Thread_Context *)data;
	for (U32 i = 0; i < context->iteration_count; ++i)
		atomic_fetch_add(*context->counter, (U64)1, COMPILER_ATOMIC_MEMORY_ORDER_RELAXED);
}

TESTER_TEST("[CORE]: Atomic Threads")
{
	const U32 THREAD_COUNT = 4;
	const U32 ITERATION_COUNT = 4096;
	Atomic<U64> counter = atomic_init((U64)0);
	Atomic_Test_Thread_Context context = {
		.counter = &counter,
		.iteration_count = ITERATION_COUNT
	};
	Platform_Thread *threads[THREAD_COUNT];

	for (U32 i = 0; i < THREAD_COUNT; ++i)
	{
		threads[i] = platform_thread_init(Platform_Thread_Desc {
			.function = _atomic_test_thread_main,
			.data = &context,
			.name = "AtomicTest"
		});
	}

	for (U32 i = 0; i < THREAD_COUNT; ++i)
		platform_thread_join(threads[i]);

	for (U32 i = 0; i < THREAD_COUNT; ++i)
		platform_thread_deinit(threads[i]);

	TESTER_CHECK(atomic_load(counter) == (U64)THREAD_COUNT * ITERATION_COUNT);
}

struct Platform_Semaphore_Test_Context
{
	Platform_Semaphore *ready_semaphore;
	Platform_Semaphore *start_semaphore;
	Platform_Semaphore *done_semaphore;
	Platform_Mutex *mutex;
	U32 finished_count;
};

inline static void
_platform_semaphore_test_thread(void *data)
{
	Platform_Semaphore_Test_Context *context = (Platform_Semaphore_Test_Context *)data;
	platform_semaphore_signal(context->ready_semaphore);
	platform_semaphore_wait(context->start_semaphore);

	platform_mutex_lock(context->mutex);
	++context->finished_count;
	platform_mutex_unlock(context->mutex);

	platform_semaphore_signal(context->done_semaphore);
}

TESTER_TEST("[CORE]: Platform Semaphore")
{
	constexpr U32 THREAD_COUNT = 4;

	Platform_Semaphore *ready_semaphore = platform_semaphore_init();
	Platform_Semaphore *start_semaphore = platform_semaphore_init();
	Platform_Semaphore *done_semaphore = platform_semaphore_init();
	Platform_Mutex *mutex = platform_mutex_init();
	Platform_Semaphore_Test_Context context = {
		.ready_semaphore = ready_semaphore,
		.start_semaphore = start_semaphore,
		.done_semaphore = done_semaphore,
		.mutex = mutex
	};
	Platform_Thread *threads[THREAD_COUNT];

	for (U32 i = 0; i < THREAD_COUNT; ++i)
	{
		threads[i] = platform_thread_init(Platform_Thread_Desc {
			.function = _platform_semaphore_test_thread,
			.data = &context,
			.name = "SemaphoreTest"
		});
	}

	for (U32 i = 0; i < THREAD_COUNT; ++i)
		platform_semaphore_wait(ready_semaphore);

	platform_mutex_lock(mutex);
	TESTER_CHECK(context.finished_count == 0);
	platform_mutex_unlock(mutex);

	platform_semaphore_signal(start_semaphore, THREAD_COUNT);
	for (U32 i = 0; i < THREAD_COUNT; ++i)
		platform_semaphore_wait(done_semaphore);

	for (U32 i = 0; i < THREAD_COUNT; ++i)
		platform_thread_deinit(threads[i]);

	platform_mutex_lock(mutex);
	TESTER_CHECK(context.finished_count == THREAD_COUNT);
	platform_mutex_unlock(mutex);

	platform_mutex_deinit(mutex);
	platform_semaphore_deinit(done_semaphore);
	platform_semaphore_deinit(start_semaphore);
	platform_semaphore_deinit(ready_semaphore);
}

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
	U32 worker_thread_count;
	U32 worker_index;
	U32 other_scheduler_worker_index;
};

inline static void
_scheduler_test_worker_query_task(void *data)
{
	Scheduler_Test_Worker_Query_Context *context = (Scheduler_Test_Worker_Query_Context *)data;
	Scheduler_Stats stats = scheduler_get_stats(context->scheduler);
	U32 worker_thread_count = stats.worker_count + stats.replacement_worker_count;
	U32 worker_index = scheduler_get_current_worker_index(context->scheduler);
	U32 other_scheduler_worker_index = scheduler_get_current_worker_index(context->other_scheduler);

	platform_mutex_lock(context->mutex);
	context->worker_thread_count = worker_thread_count;
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

	Scheduler_Stats stats = scheduler_get_stats(scheduler);
	TESTER_CHECK(stats.worker_count + stats.replacement_worker_count == 2);
	TESTER_CHECK(scheduler_get_current_worker_index(scheduler) == U32_MAX);

	scheduler_submit(scheduler, Scheduler_Task {
		.function = _scheduler_test_worker_query_task,
		.data = &context
	});
	scheduler_wait_all(scheduler);

	platform_mutex_lock(mutex);
	U32 worker_thread_count = context.worker_thread_count;
	U32 worker_index = context.worker_index;
	U32 other_scheduler_worker_index = context.other_scheduler_worker_index;
	platform_mutex_unlock(mutex);

	TESTER_CHECK(worker_thread_count == 2);
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
	U32 count_after_first_block = scheduler_get_stats(context->scheduler).blocked_worker_count;
	scheduler_worker_block_ahead(context->scheduler);
	U32 count_after_nested_block = scheduler_get_stats(context->scheduler).blocked_worker_count;

	platform_mutex_lock(context->mutex);
	context->count_after_first_block = count_after_first_block;
	context->count_after_nested_block = count_after_nested_block;
	context->block_started = true;
	platform_condition_variable_signal(context->condition_variable);
	while (!context->release)
		platform_condition_variable_wait(context->condition_variable, context->mutex);
	platform_mutex_unlock(context->mutex);

	scheduler_worker_block_clear(context->scheduler);
	U32 count_after_first_clear = scheduler_get_stats(context->scheduler).blocked_worker_count;
	scheduler_worker_block_clear(context->scheduler);
	U32 count_after_final_clear = scheduler_get_stats(context->scheduler).blocked_worker_count;

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

	TESTER_CHECK(scheduler_get_stats(scheduler).blocked_worker_count == 0);

	scheduler_submit(scheduler, Scheduler_Task {
		.function = _scheduler_test_worker_blocking_task,
		.data = &context
	});

	platform_mutex_lock(mutex);
	while (!context.block_started)
		platform_condition_variable_wait(condition_variable, mutex);
	platform_mutex_unlock(mutex);

	TESTER_CHECK(scheduler_get_stats(scheduler).blocked_worker_count == 1);

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
	TESTER_CHECK(scheduler_get_stats(scheduler).blocked_worker_count == 0);

	platform_condition_variable_deinit(condition_variable);
	platform_mutex_deinit(mutex);
	scheduler_deinit(scheduler);
}

struct Scheduler_Test_Worker_Blocking_Replacement_Context
{
	Scheduler *scheduler;
	Platform_Mutex *mutex;
	Platform_Condition_Variable *condition_variable;
	U32 worker_count;
	U32 finished_count;
	U32 replacement_worker_finished_count;
	bool block_started;
	bool release;
	bool blocking_finished;
};

inline static void
_scheduler_test_worker_blocking_replacement_blocking_task(void *data)
{
	Scheduler_Test_Worker_Blocking_Replacement_Context *context = (Scheduler_Test_Worker_Blocking_Replacement_Context *)data;

	scheduler_worker_block_ahead(context->scheduler);
	scheduler_worker_block_ahead(context->scheduler);

	platform_mutex_lock(context->mutex);
	context->block_started = true;
	platform_condition_variable_signal(context->condition_variable);
	while (!context->release)
		platform_condition_variable_wait(context->condition_variable, context->mutex);
	platform_mutex_unlock(context->mutex);

	scheduler_worker_block_clear(context->scheduler);
	scheduler_worker_block_clear(context->scheduler);

	platform_mutex_lock(context->mutex);
	context->blocking_finished = true;
	platform_mutex_unlock(context->mutex);
}

inline static void
_scheduler_test_worker_blocking_replacement_task(void *data)
{
	Scheduler_Test_Worker_Blocking_Replacement_Context *context = (Scheduler_Test_Worker_Blocking_Replacement_Context *)data;
	U32 worker_index = scheduler_get_current_worker_index(context->scheduler);

	platform_mutex_lock(context->mutex);
	++context->finished_count;
	if (worker_index >= context->worker_count)
		++context->replacement_worker_finished_count;
	platform_mutex_unlock(context->mutex);
}

TESTER_TEST("[CORE]: Scheduler Worker Blocking Replacement")
{
	constexpr U32 TASK_COUNT = 64;
	constexpr U32 WORKER_COUNT = 1;

	Scheduler *scheduler = scheduler_init(Scheduler_Desc {
		.worker_count = WORKER_COUNT,
		.replacement_worker_count = 1,
		.initial_task_queue_capacity = TASK_COUNT + 1
	});
	Scheduler_Group *group = scheduler_group_init(scheduler);
	Platform_Mutex *mutex = platform_mutex_init();
	Platform_Condition_Variable *condition_variable = platform_condition_variable_init();
	Scheduler_Test_Worker_Blocking_Replacement_Context context = {
		.scheduler = scheduler,
		.mutex = mutex,
		.condition_variable = condition_variable,
		.worker_count = WORKER_COUNT
	};

	Scheduler_Stats stats = scheduler_get_stats(scheduler);
	TESTER_CHECK(stats.worker_count + stats.replacement_worker_count == WORKER_COUNT + 1);

	scheduler_submit(scheduler, Scheduler_Task {
		.function = _scheduler_test_worker_blocking_replacement_blocking_task,
		.data = &context
	});

	platform_mutex_lock(mutex);
	while (!context.block_started)
		platform_condition_variable_wait(condition_variable, mutex);
	platform_mutex_unlock(mutex);

	TESTER_CHECK(scheduler_get_stats(scheduler).blocked_worker_count == 1);

	Scheduler_Task tasks[TASK_COUNT];
	for (U32 i = 0; i < TASK_COUNT; ++i)
	{
		tasks[i] = Scheduler_Task {
			.function = _scheduler_test_worker_blocking_replacement_task,
			.data = &context
		};
	}

	scheduler_submit(scheduler, slice_from(tasks), group);
	scheduler_wait_group(scheduler, group);

	platform_mutex_lock(mutex);
	U32 finished_count = context.finished_count;
	U32 replacement_worker_finished_count = context.replacement_worker_finished_count;
	bool blocking_finished = context.blocking_finished;
	context.release = true;
	platform_condition_variable_signal(condition_variable);
	platform_mutex_unlock(mutex);

	TESTER_CHECK(finished_count == TASK_COUNT);
	TESTER_CHECK(replacement_worker_finished_count == TASK_COUNT);
	TESTER_CHECK(!blocking_finished);

	scheduler_wait_all(scheduler);
	TESTER_CHECK(scheduler_get_stats(scheduler).blocked_worker_count == 0);

	platform_condition_variable_deinit(condition_variable);
	platform_mutex_deinit(mutex);
	scheduler_group_deinit(scheduler, group);
	scheduler_deinit(scheduler);
}

struct Scheduler_Test_Stats_Context
{
	Scheduler *scheduler;
	Platform_Mutex *mutex;
	Platform_Condition_Variable *condition_variable;
	U32 finished_count;
	bool block_started;
	bool release;
};

inline static void
_scheduler_test_stats_blocking_task(void *data)
{
	Scheduler_Test_Stats_Context *context = (Scheduler_Test_Stats_Context *)data;

	scheduler_worker_block_ahead(context->scheduler);

	platform_mutex_lock(context->mutex);
	context->block_started = true;
	platform_condition_variable_signal(context->condition_variable);
	while (!context->release)
		platform_condition_variable_wait(context->condition_variable, context->mutex);
	platform_mutex_unlock(context->mutex);

	scheduler_worker_block_clear(context->scheduler);
}

inline static void
_scheduler_test_stats_task(void *data)
{
	Scheduler_Test_Stats_Context *context = (Scheduler_Test_Stats_Context *)data;

	platform_mutex_lock(context->mutex);
	++context->finished_count;
	platform_mutex_unlock(context->mutex);
}

inline static void
_scheduler_test_stats_wait_for_block(Scheduler_Test_Stats_Context *context)
{
	platform_mutex_lock(context->mutex);
	while (!context->block_started)
		platform_condition_variable_wait(context->condition_variable, context->mutex);
	platform_mutex_unlock(context->mutex);
}

TESTER_TEST("[CORE]: Scheduler Stats")
{
	constexpr U32 TASK_COUNT = 3;

	Platform_Mutex *mutex = platform_mutex_init();
	Platform_Condition_Variable *condition_variable = platform_condition_variable_init();

	Scheduler *replacement_scheduler = scheduler_init(Scheduler_Desc {
		.worker_count = 1,
		.replacement_worker_count = 1,
		.initial_task_queue_capacity = 1
	});
	Scheduler_Test_Stats_Context context = {
		.scheduler = replacement_scheduler,
		.mutex = mutex,
		.condition_variable = condition_variable
	};

	Scheduler_Stats stats = scheduler_get_stats(replacement_scheduler);
	TESTER_CHECK(stats.worker_count == 1);
	TESTER_CHECK(stats.replacement_worker_count == 1);
	TESTER_CHECK(stats.active_replacement_worker_count == 0);
	TESTER_CHECK(stats.blocked_worker_count == 0);
	TESTER_CHECK(stats.active_task_count == 0);
	TESTER_CHECK(stats.queued_task_count == 0);
	TESTER_CHECK(stats.live_group_count == 0);

	Scheduler_Group *group = scheduler_group_init(replacement_scheduler);
	stats = scheduler_get_stats(replacement_scheduler);
	TESTER_CHECK(stats.live_group_count == 1);

	scheduler_submit(replacement_scheduler, Scheduler_Task {
		.function = _scheduler_test_stats_blocking_task,
		.data = &context
	});

	_scheduler_test_stats_wait_for_block(&context);

	stats = scheduler_get_stats(replacement_scheduler);
	TESTER_CHECK(stats.worker_count == 1);
	TESTER_CHECK(stats.replacement_worker_count == 1);
	TESTER_CHECK(stats.active_replacement_worker_count == 1);
	TESTER_CHECK(stats.blocked_worker_count == 1);
	TESTER_CHECK(stats.active_task_count == 1);
	TESTER_CHECK(stats.queued_task_count == 0);
	TESTER_CHECK(stats.live_group_count == 1);

	platform_mutex_lock(mutex);
	context.release = true;
	platform_condition_variable_signal(condition_variable);
	platform_mutex_unlock(mutex);

	scheduler_wait_all(replacement_scheduler);

	stats = scheduler_get_stats(replacement_scheduler);
	TESTER_CHECK(stats.active_replacement_worker_count == 0);
	TESTER_CHECK(stats.blocked_worker_count == 0);
	TESTER_CHECK(stats.active_task_count == 0);
	TESTER_CHECK(stats.queued_task_count == 0);
	TESTER_CHECK(stats.live_group_count == 1);

	scheduler_group_deinit(replacement_scheduler, group);

	stats = scheduler_get_stats(replacement_scheduler);
	TESTER_CHECK(stats.live_group_count == 0);

	scheduler_deinit(replacement_scheduler);

	Scheduler *queued_scheduler = scheduler_init(Scheduler_Desc {
		.worker_count = 1,
		.initial_task_queue_capacity = TASK_COUNT + 1
	});
	group = scheduler_group_init(queued_scheduler);
	context = Scheduler_Test_Stats_Context {
		.scheduler = queued_scheduler,
		.mutex = mutex,
		.condition_variable = condition_variable
	};

	scheduler_submit(queued_scheduler, Scheduler_Task {
		.function = _scheduler_test_stats_blocking_task,
		.data = &context
	});

	_scheduler_test_stats_wait_for_block(&context);

	stats = scheduler_get_stats(queued_scheduler);
	TESTER_CHECK(stats.worker_count == 1);
	TESTER_CHECK(stats.replacement_worker_count == 0);
	TESTER_CHECK(stats.active_replacement_worker_count == 0);
	TESTER_CHECK(stats.blocked_worker_count == 1);
	TESTER_CHECK(stats.active_task_count == 1);
	TESTER_CHECK(stats.queued_task_count == 0);
	TESTER_CHECK(stats.live_group_count == 1);

	Scheduler_Task tasks[TASK_COUNT];
	for (U32 i = 0; i < TASK_COUNT; ++i)
	{
		tasks[i] = Scheduler_Task {
			.function = _scheduler_test_stats_task,
			.data = &context
		};
	}

	scheduler_submit(queued_scheduler, slice_from(tasks), group);

	stats = scheduler_get_stats(queued_scheduler);
	TESTER_CHECK(stats.active_replacement_worker_count == 0);
	TESTER_CHECK(stats.blocked_worker_count == 1);
	TESTER_CHECK(stats.active_task_count == 1);
	TESTER_CHECK(stats.queued_task_count == TASK_COUNT);
	TESTER_CHECK(stats.live_group_count == 1);

	platform_mutex_lock(mutex);
	context.release = true;
	platform_condition_variable_signal(condition_variable);
	platform_mutex_unlock(mutex);

	scheduler_wait_group(queued_scheduler, group);
	scheduler_wait_all(queued_scheduler);

	platform_mutex_lock(mutex);
	U32 finished_count = context.finished_count;
	platform_mutex_unlock(mutex);

	stats = scheduler_get_stats(queued_scheduler);
	TESTER_CHECK(finished_count == TASK_COUNT);
	TESTER_CHECK(stats.active_replacement_worker_count == 0);
	TESTER_CHECK(stats.blocked_worker_count == 0);
	TESTER_CHECK(stats.active_task_count == 0);
	TESTER_CHECK(stats.queued_task_count == 0);
	TESTER_CHECK(stats.live_group_count == 1);

	scheduler_group_deinit(queued_scheduler, group);

	stats = scheduler_get_stats(queued_scheduler);
	TESTER_CHECK(stats.live_group_count == 0);

	scheduler_deinit(queued_scheduler);
	platform_condition_variable_deinit(condition_variable);
	platform_mutex_deinit(mutex);
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

inline static void
_scheduler_test_delayed_task(void *data)
{
	platform_thread_sleep(1);
	_scheduler_test_task(data);
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
	constexpr U32 TASK_COUNT = 257;

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
		.worker_count = 3,
		.initial_task_queue_capacity = 1
	});

	scheduler_submit(scheduler, slice_from(tasks));
	scheduler_wait_all(scheduler);

	platform_mutex_lock(mutex);
	U32 finished_count = context.finished_count;
	platform_mutex_unlock(mutex);

	TESTER_CHECK(finished_count == TASK_COUNT);

	scheduler_submit(scheduler, {
		Scheduler_Task {
			.function = _scheduler_test_task,
			.data = &context
		},
		Scheduler_Task {
			.function = _scheduler_test_task,
			.data = &context
		}
	});
	scheduler_wait_all(scheduler);

	platform_mutex_lock(mutex);
	finished_count = context.finished_count;
	platform_mutex_unlock(mutex);

	TESTER_CHECK(finished_count == TASK_COUNT + 2);
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

TESTER_TEST("[CORE]: Scheduler Deinit Drains Queued Tasks")
{
	constexpr U32 TASK_COUNT = 96;

	Platform_Mutex *mutex = platform_mutex_init();
	Scheduler_Test_Task_Context context = {
		.mutex = mutex
	};

	Scheduler *scheduler = scheduler_init(Scheduler_Desc {
		.worker_count = 4,
		.initial_task_queue_capacity = 1
	});

	for (U32 i = 0; i < TASK_COUNT; ++i)
	{
		scheduler_submit(scheduler, Scheduler_Task {
			.function = _scheduler_test_delayed_task,
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

TESTER_TEST("[CORE]: Scheduler Parallel For Auto Chunk Size")
{
	constexpr U32 ITEM_COUNT = 257;
	constexpr U32 EXPECTED_INDEX_SUM = ITEM_COUNT * (ITEM_COUNT - 1) / 2;

	bool visited[ITEM_COUNT] = {};
	Platform_Mutex *mutex = platform_mutex_init();
	Scheduler_Test_Parallel_For_Context context = {
		.mutex = mutex,
		.visited = visited
	};

	Scheduler *scheduler = scheduler_init(Scheduler_Desc {
		.worker_count = 2,
		.initial_task_queue_capacity = 16
	});

	scheduler_parallel_for(scheduler, Scheduler_Parallel_For_Desc {
		.count = ITEM_COUNT,
		.function = _scheduler_test_parallel_for,
		.data = &context
	});

	platform_mutex_lock(mutex);
	U32 visited_count = context.visited_count;
	U32 index_sum = context.index_sum;
	platform_mutex_unlock(mutex);

	for (U32 i = 0; i < ITEM_COUNT; ++i)
		TESTER_CHECK(visited[i]);
	TESTER_CHECK(visited_count == ITEM_COUNT);
	TESTER_CHECK(index_sum == EXPECTED_INDEX_SUM);

	scheduler_deinit(scheduler);
	platform_mutex_deinit(mutex);
}

TESTER_TEST("[CORE]: Scheduler Parallel For Small Chunk Stress")
{
	constexpr U32 ITEM_COUNT = 1024;
	constexpr U32 EXPECTED_INDEX_SUM = ITEM_COUNT * (ITEM_COUNT - 1) / 2;

	bool visited[ITEM_COUNT] = {};
	Platform_Mutex *mutex = platform_mutex_init();
	Scheduler_Test_Parallel_For_Context context = {
		.mutex = mutex,
		.visited = visited
	};

	Scheduler *scheduler = scheduler_init(Scheduler_Desc {
		.worker_count = 4,
		.initial_task_queue_capacity = ITEM_COUNT
	});

	scheduler_parallel_for(scheduler, Scheduler_Parallel_For_Desc {
		.count = ITEM_COUNT,
		.chunk_size = 1,
		.function = _scheduler_test_parallel_for,
		.data = &context
	});

	platform_mutex_lock(mutex);
	U32 visited_count = context.visited_count;
	U32 index_sum = context.index_sum;
	platform_mutex_unlock(mutex);

	for (U32 i = 0; i < ITEM_COUNT; ++i)
		TESTER_CHECK(visited[i]);
	TESTER_CHECK(visited_count == ITEM_COUNT);
	TESTER_CHECK(index_sum == EXPECTED_INDEX_SUM);

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

struct Scheduler_Test_Stealing_Context
{
	Scheduler *scheduler;
	Platform_Mutex *mutex;
	Platform_Condition_Variable *condition_variable;
	U32 blocked_worker_index;
	U32 finished_count;
	U32 stolen_task_count;
	U32 first_stolen_task_index;
	bool block_started;
	bool release;
	bool blocking_finished;
};

struct Scheduler_Test_Stealing_Task
{
	Scheduler_Test_Stealing_Context *context;
	U32 queued_worker_index;
	U32 index;
};

inline static void
_scheduler_test_stealing_blocking_task(void *data)
{
	Scheduler_Test_Stealing_Context *context = (Scheduler_Test_Stealing_Context *)data;
	U32 worker_index = scheduler_get_current_worker_index(context->scheduler);

	platform_mutex_lock(context->mutex);
	context->blocked_worker_index = worker_index;
	context->block_started = true;
	platform_condition_variable_signal(context->condition_variable);
	while (!context->release)
		platform_condition_variable_wait(context->condition_variable, context->mutex);
	context->blocking_finished = true;
	platform_mutex_unlock(context->mutex);
}

inline static void
_scheduler_test_stealing_task(void *data)
{
	Scheduler_Test_Stealing_Task *task = (Scheduler_Test_Stealing_Task *)data;
	Scheduler_Test_Stealing_Context *context = task->context;
	U32 worker_index = scheduler_get_current_worker_index(context->scheduler);

	platform_mutex_lock(context->mutex);
	++context->finished_count;
	if (task->queued_worker_index == context->blocked_worker_index && worker_index != context->blocked_worker_index)
	{
		if (context->first_stolen_task_index == U32_MAX)
			context->first_stolen_task_index = task->index;
		++context->stolen_task_count;
	}
	platform_mutex_unlock(context->mutex);
}

TESTER_TEST("[CORE]: Scheduler Steals From Blocked Worker")
{
	constexpr U32 WORKER_COUNT = 2;
	constexpr U32 TASK_COUNT = 64;

	Platform_Mutex *mutex = platform_mutex_init();
	Platform_Condition_Variable *condition_variable = platform_condition_variable_init();
	Scheduler *scheduler = scheduler_init(Scheduler_Desc {
		.worker_count = WORKER_COUNT,
		.initial_task_queue_capacity = TASK_COUNT + 1
	});
	Scheduler_Group *group = scheduler_group_init(scheduler);
	Scheduler_Test_Stealing_Context context = {
		.scheduler = scheduler,
		.mutex = mutex,
		.condition_variable = condition_variable,
		.blocked_worker_index = U32_MAX,
		.first_stolen_task_index = U32_MAX
	};

	scheduler_submit(scheduler, Scheduler_Task {
		.function = _scheduler_test_stealing_blocking_task,
		.data = &context
	});

	platform_mutex_lock(mutex);
	while (!context.block_started)
		platform_condition_variable_wait(condition_variable, mutex);
	U32 blocked_worker_index = context.blocked_worker_index;
	platform_mutex_unlock(mutex);

	TESTER_CHECK(blocked_worker_index < WORKER_COUNT);

	Scheduler_Test_Stealing_Task task_data[TASK_COUNT];
	Scheduler_Task tasks[TASK_COUNT];
	for (U32 i = 0; i < TASK_COUNT; ++i)
	{
		task_data[i] = Scheduler_Test_Stealing_Task {
			.context = &context,
			.queued_worker_index = (1 + i) % WORKER_COUNT,
			.index = i
		};
		tasks[i] = Scheduler_Task {
			.function = _scheduler_test_stealing_task,
			.data = &task_data[i]
		};
	}

	scheduler_submit(scheduler, slice_from(tasks), group);
	scheduler_wait_group(scheduler, group);

	platform_mutex_lock(mutex);
	U32 finished_count = context.finished_count;
	U32 stolen_task_count = context.stolen_task_count;
	U32 first_stolen_task_index = context.first_stolen_task_index;
	bool blocking_finished = context.blocking_finished;
	context.release = true;
	platform_condition_variable_signal(condition_variable);
	platform_mutex_unlock(mutex);

	U32 expected_first_stolen_task_index = TASK_COUNT - 1;
	while ((1 + expected_first_stolen_task_index) % WORKER_COUNT != blocked_worker_index)
		--expected_first_stolen_task_index;

	TESTER_CHECK(finished_count == TASK_COUNT);
	TESTER_CHECK(stolen_task_count > 0);
	TESTER_CHECK(first_stolen_task_index == expected_first_stolen_task_index);
	TESTER_CHECK(!blocking_finished);

	scheduler_wait_all(scheduler);

	scheduler_group_deinit(scheduler, group);
	scheduler_deinit(scheduler);
	platform_condition_variable_deinit(condition_variable);
	platform_mutex_deinit(mutex);
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
	scheduler_submit(scheduler, slice_from(tasks), group);

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

TESTER_TEST("[CORE]: Scheduler Worker Wait Group Multi Worker")
{
	constexpr U32 TASK_COUNT = 128;

	Platform_Mutex *mutex = platform_mutex_init();
	Scheduler *scheduler = scheduler_init(Scheduler_Desc {
		.worker_count = 3,
		.initial_task_queue_capacity = TASK_COUNT
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

TESTER_TEST("[CORE]: Arena_Allocator_Page_Growth")
{
	//
	// Cross several page boundaries within one node, then reuse its committed
	// pages after resetting a mark and clearing the arena.
	//
	U64 page_size = platform_virtual_memory_get_page_size();
	memory::Arena_Allocator *arena = memory::arena_allocator_init(page_size * 8);
	DEFER(memory::arena_allocator_deinit(arena));
	Memory_Block base = memory::arena_allocator_allocate(arena, 16, 1);
	((U8 *)base.data)[0] = 42;
	memory::Arena_Allocator_Mark mark = memory::arena_allocator_mark(arena);
	U64 size = page_size * 4 + 31;
	bool contiguous = true;
	for (U64 i = 0; i < size; ++i)
	{
		Memory_Block byte = memory::arena_allocator_allocate(arena, 1, 1);
		contiguous = contiguous && byte.data == (U8 *)base.data + 16 + i;
		*(U8 *)byte.data = (U8)i;
	}
	TESTER_CHECK(contiguous);
	TESTER_CHECK(memory::arena_allocator_mark(arena).head == mark.head);
	TESTER_CHECK(memory::arena_allocator_get_used(arena) == 16 + size);
	bool matches = true;
	for (U64 i = 0; i < size; ++i)
		matches = matches && ((U8 *)base.data)[16 + i] == (U8)i;
	TESTER_CHECK(matches);

	memory::arena_allocator_reset_to_mark(arena, mark);
	Memory_Block reused = memory::arena_allocator_allocate(arena, size, 1);
	TESTER_CHECK(reused.data == (U8 *)base.data + 16);
	TESTER_CHECK(((U8 *)base.data)[0] == 42);
	((U8 *)reused.data)[size - 1] = 99;
	TESTER_CHECK(((U8 *)reused.data)[size - 1] == 99);

	memory::arena_allocator_clear(arena);
	Memory_Block after_clear = memory::arena_allocator_allocate(arena, page_size * 6, 1);
	TESTER_CHECK(after_clear.data == base.data);
	((U8 *)after_clear.data)[page_size * 6 - 1] = 123;
	TESTER_CHECK(((U8 *)after_clear.data)[page_size * 6 - 1] == 123);
	Memory_Block aligned = memory::arena_allocator_allocate(arena, 64, 64);
	TESTER_CHECK((U64)aligned.data % 64 == 0);
	((U8 *)aligned.data)[63] = 255;
	TESTER_CHECK(((U8 *)aligned.data)[63] == 255);
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

TESTER_TEST("[CORE]: JSON String Serialization")
{
	const char controls[] =
		"\x00\x01\x02\x03\x04\x05\x06\x07"
		"\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
		"\x10\x11\x12\x13\x14\x15\x16\x17"
		"\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f";
	struct
	{
		Slice<const char> input;
		const char *expected;
	} cases[] = {
		{slice_from(""), R"("")"},
		{slice_from("plain / text"), R"("plain / text")"},
		{slice_from("\"\\\b\f\n\r\t"), R"("\"\\\b\f\n\r\t")"},
		{slice_from("\\n"), R"("\\n")"},
		{slice_from("ends\\"), R"("ends\\")"},
		{slice_from("\xc3\xa9 \xe4\xb8\xad \xf0\x9f\x98\x80"), "\"\xc3\xa9 \xe4\xb8\xad \xf0\x9f\x98\x80\""},
		{Slice<const char>(controls, sizeof(controls) - 1),
			"\"\\u0000\\u0001\\u0002\\u0003\\u0004\\u0005\\u0006\\u0007"
			"\\b\\t\\n\\u000b\\f\\r\\u000e\\u000f"
			"\\u0010\\u0011\\u0012\\u0013\\u0014\\u0015\\u0016\\u0017"
			"\\u0018\\u0019\\u001a\\u001b\\u001c\\u001d\\u001e\\u001f\""}
	};
	for (const auto &entry : cases)
	{
		JSON_Value root = json_value_init_as_object();
		DEFER(json_value_deinit(root));
		JSON_Value value = json_value_init_as_string();
		string_append(value.as_string, entry.input);
		JSON_Value object = json_value_init_as_object();
		json_value_object_insert(object, value.as_string, value);
		JSON_Value array = json_value_init_as_array();
		array_push(array.as_array, json_value_copy(value));
		array_push(array.as_array, object);
		json_value_object_insert(root, "items", array);

		String expected = string_from("{\n\t\"items\": [\n\t\t");
		DEFER(string_deinit(expected));
		string_append(expected, entry.expected);
		string_append(expected, ",\n\t\t{\n\t\t\t");
		string_append(expected, entry.expected);
		string_append(expected, ": ");
		string_append(expected, entry.expected);
		string_append(expected, "\n\t\t}\n\t]\n}");

		auto [output, error] = json_value_to_string(root);
		DEFER(string_deinit(output));
		TESTER_CHECK(error == false);
		TESTER_CHECK(output == expected);
		TESTER_CHECK(output.data[output.count] == '\0');
	}
}

struct JSON_Test_Allocator : memory::Allocator
{
	U64 bytes;

	Memory_Block
	allocate(U64 size, U64 alignment) override
	{
		Memory_Block block = memory::allocate(size, alignment);
		bytes += block.size;
		return block;
	}

	void
	deallocate(Memory_Block block) override
	{
		validate(bytes >= block.size);
		bytes -= block.size;
		memory::deallocate(block);
	}
};

TESTER_TEST("[CORE]: JSON Strict Input")
{
	const char *invalid[] = {
		"", " ", "n", "nul", "nullx", "tru", "True", "falsex",
		"+1", "01", "-01", "-", ".1", "1.", "1e", "1e+", "1e-", "1.e1",
		"0x10", "nan", "NaN", "inf", "Infinity", "-inf", "1e309", "-1e309",
		"// comment", "/* comment */null", "null//comment", "\vnull", "null\f",
		"null true", "{} []", "[", "[1", "[1,]", "[,1]", "[1 2]",
		"{", "{x:1}", "{\"x\" 1}", "{\"x\":}", "{\"x\":1,}", "{\"x\":1 \"y\":2}",
		"\"", "\"\\", "\"\\q\"", "\"\\u\"", "\"\\u000\"", "\"\\u00x0\"", "\"\n\"",
		"\"\\ud800\"", "\"\\ud800\\u0000\"", "\"\\ud800\\ud800\"", "\"\\udc00\"",
		"\"\x80\"", "\"\xc0\x80\"", "\"\xc2\"", "\"\xe0\x80\x80\"", "\"\xe2\x82\"",
		"\"\xed\xa0\x80\"", "\"\xf0\x80\x80\x80\"", "\"\xf4\x90\x80\x80\"", "\"\xf5\x80\x80\x80\"",
		"[\"owned\",{\"key\":[\"nested\"]},]", "{\"owned\":[\"nested\"],\"pending\":}",
		"{\"owned\":[\"nested\"]} trailing"
	};
	JSON_Test_Allocator allocator = {};
	for (const char *input : invalid)
	{
		auto [value, error] = json_value_from_string(input, &allocator);
		TESTER_CHECK(error == true);
		TESTER_CHECK(value.kind == JSON_VALUE_KIND_INVALID);
		json_value_deinit(value);
		TESTER_CHECK(allocator.bytes == 0);
	}
	const char *complete = R"({"key":["owned",true,false,null,-12.5e+2,{"text":"\uD83D\uDE00\\"}]})";
	U64 count = slice_from(complete).count;
	for (U64 length = 0; length < count; ++length)
	{
		Memory_Block buffer = memory::allocate(length, alignof(char));
		DEFER(memory::deallocate(buffer));
		if (length)
			::memcpy(buffer.data, complete, length);
		auto [value, error] = json_value_from_string(Slice<const char>((const char *)buffer.data, length), &allocator);
		TESTER_CHECK(error == true);
		json_value_deinit(value);
		TESTER_CHECK(allocator.bytes == 0);
	}
	const char null_in_string[] = {'"', 'a', '\0', 'b', '"'};
	const char null_after_value[] = {'n', 'u', 'l', 'l', '\0', 't', 'r', 'u', 'e'};
	Slice<const char> embedded_nulls[] = {
		Slice<const char>(null_in_string, sizeof(null_in_string)),
		Slice<const char>(null_after_value, sizeof(null_after_value))
	};
	for (auto input : embedded_nulls)
	{
		String text = string_init();
		DEFER(string_deinit(text));
		string_append(text, input);
		auto [value, error] = json_value_from_string(text, &allocator);
		TESTER_CHECK(error == true);
		json_value_deinit(value);
		TESTER_CHECK(allocator.bytes == 0);
	}
	const char bounded[] = {'t', 'r', 'u', 'e', 'x'};
	auto [value, error] = json_value_from_string(Slice<const char>(bounded, 4), &allocator);
	DEFER(json_value_deinit(value));
	TESTER_CHECK(error == false);
	TESTER_CHECK(value.kind == JSON_VALUE_KIND_BOOL && value.as_bool);
	auto [empty_value, empty_error] = json_value_from_string(nullptr, &allocator);
	TESTER_CHECK(empty_error == true);
	TESTER_CHECK(empty_value.kind == JSON_VALUE_KIND_INVALID);
}

TESTER_TEST("[CORE]: JSON Unicode")
{
	const char decoded[] = "\"\\/\b\f\n\r\t\0\x7f\xc2\x80\xdf\xbf\xe0\xa0\x80\xef\xbf\xbf\xf0\x90\x80\x80\xf4\x8f\xbf\xbf";
	const char *input = R"("\"\\\/\b\f\n\r\t\u0000\u007f\u0080\u07ff\u0800\uFFFF\uD800\uDC00\udbff\udfff")";
	auto [value, error] = json_value_from_string(input);
	DEFER(json_value_deinit(value));
	TESTER_CHECK(error == false);
	if (error)
		return;
	TESTER_CHECK(value.kind == JSON_VALUE_KIND_STRING);
	TESTER_CHECK(value.as_string.count == sizeof(decoded) - 1);
	TESTER_CHECK(::memcmp(value.as_string.data, decoded, sizeof(decoded) - 1) == 0);
	auto [encoded, encode_error] = json_value_to_string(value);
	DEFER(string_deinit(encoded));
	TESTER_CHECK(encode_error == false);
	auto [copy, copy_error] = json_value_from_string(encoded);
	DEFER(json_value_deinit(copy));
	TESTER_CHECK(copy_error == false);
	if (!copy_error)
		TESTER_CHECK(copy.as_string == value.as_string);

	JSON_Test_Allocator allocator = {};
	{
		auto [object, object_error] = json_value_from_string(R"({"x\u0000y":"first","\u0078\u0000y":["last"]})", &allocator);
		DEFER(json_value_deinit(object));
		TESTER_CHECK(object_error == false);
		if (object_error)
			return;
		TESTER_CHECK(object.as_object.count == 1);
		const char name[] = {'x', '\0', 'y'};
		String key = string_from(name, name + sizeof(name));
		DEFER(string_deinit(key));
		JSON_Value member = json_value_object_find(object, key);
		TESTER_CHECK(member.kind == JSON_VALUE_KIND_ARRAY);
		TESTER_CHECK(member.as_array[0].as_string == "last");
	}
	TESTER_CHECK(allocator.bytes == 0);
}

TESTER_TEST("[CORE]: JSON Numbers And Root Values")
{
	F64 numbers[] = {0.0, -0.0, 0.1, -0.1, 1.2345678901234567, -2147483648.0, 2147483647.0,
		1.0, -1.0, 4294967295.0, 9007199254740991.0, -9007199254740991.0,
		0x1p63, -0x1p63, 0x1p63 - 1024.0, -0x1p63 + 1024.0, 0x1p63 + 2048.0, -0x1p63 - 2048.0,
		1.0e20, 1.0e-20, DBL_MIN, DBL_MAX, -DBL_MAX, 0x1p-1074};
	for (F64 number : numbers)
	{
		JSON_Value value = json_value_init_as_number(number);
		auto [encoded, encode_error] = json_value_to_string(value);
		DEFER(string_deinit(encoded));
		TESTER_CHECK(encode_error == false);
		auto [decoded, decode_error] = json_value_from_string(encoded);
		DEFER(json_value_deinit(decoded));
		TESTER_CHECK(decode_error == false);
		TESTER_CHECK(decoded.kind == JSON_VALUE_KIND_NUMBER);
		if (!decode_error)
			TESTER_CHECK(::memcmp(&decoded.as_number, &number, sizeof(number)) == 0);
	}
	const char *roots[] = {"null", "true", "false", "\"text\"", "[]", "{}", " [1, true, null] \r\n\t"};
	for (const char *input : roots)
	{
		auto [value, error] = json_value_from_string(input);
		DEFER(json_value_deinit(value));
		TESTER_CHECK(error == false);
		auto [encoded, encode_error] = json_value_to_string(value);
		DEFER(string_deinit(encoded));
		TESTER_CHECK(encode_error == false);
		auto [decoded, decode_error] = json_value_from_string(encoded);
		DEFER(json_value_deinit(decoded));
		TESTER_CHECK(decode_error == false);
		TESTER_CHECK(decoded.kind == value.kind);
	}
	errno = ERANGE;
	auto [value, error] = json_value_from_string("1.25e+2");
	TESTER_CHECK(error == false);
	TESTER_CHECK(value.as_number == 125.0);
	String long_number = string_from("0.");
	DEFER(string_deinit(long_number));
	string_append(long_number, '0', 200);
	string_append(long_number, '1');
	auto [small, small_error] = json_value_from_string(long_number);
	TESTER_CHECK(small_error == false);
	TESTER_CHECK(small.as_number == 1e-201);
	auto [underflow, underflow_error] = json_value_from_string("1e-9999");
	TESTER_CHECK(underflow_error == false);
	TESTER_CHECK(underflow.as_number == 0.0);
}

TESTER_TEST("[CORE]: JSON Ownership")
{
	JSON_Test_Allocator allocator = {};
	{
		JSON_Value object = json_value_init_as_object(&allocator);
		DEFER(json_value_deinit(object));
		JSON_Value first = json_value_init_as_string(&allocator);
		string_append(first.as_string, "first allocation");
		json_value_object_insert(object, "name", first);
		JSON_Value second = json_value_init_as_array(&allocator);
		array_push(second.as_array, json_value_init_as_bool(true));
		json_value_object_insert(object, "name", second);
		auto *entry = hash_table_find(object.as_object, string_literal("name"));
		TESTER_CHECK(object.as_object.count == 1);
		TESTER_CHECK(entry->key.allocator == &allocator);
		TESTER_CHECK(entry->value.kind == JSON_VALUE_KIND_ARRAY);
	}
	TESTER_CHECK(allocator.bytes == 0);

	F64 invalid_numbers[] = {F64_INFINITY, F64_NEGATIVE_INFINITY, F64_NAN};
	for (F64 number : invalid_numbers)
	{
		JSON_Value value = json_value_init_as_number(number);
		auto [encoded, error] = json_value_to_string(value, &allocator);
		TESTER_CHECK(error == true);
		TESTER_CHECK(encoded.data == nullptr);
		TESTER_CHECK(allocator.bytes == 0);
	}
	{
		JSON_Value array = json_value_init_as_array(&allocator);
		DEFER(json_value_deinit(array));
		array_push(array.as_array, JSON_Value{});
		U64 bytes = allocator.bytes;
		auto [encoded, error] = json_value_to_string(array, &allocator);
		TESTER_CHECK(error == true);
		TESTER_CHECK(encoded.data == nullptr);
		TESTER_CHECK(allocator.bytes == bytes);
	}
	TESTER_CHECK(allocator.bytes == 0);
	{
		JSON_Value value = json_value_init_as_string(&allocator);
		DEFER(json_value_deinit(value));
		string_append(value.as_string, "\xc0\x80");
		U64 bytes = allocator.bytes;
		auto [encoded, error] = json_value_to_string(value, &allocator);
		TESTER_CHECK(error == true);
		TESTER_CHECK(encoded.data == nullptr);
		TESTER_CHECK(allocator.bytes == bytes);
		JSON_Value object = json_value_init_as_object(&allocator);
		DEFER(json_value_deinit(object));
		json_value_object_insert(object, value.as_string, json_value_init_as_bool(true));
		auto [object_encoded, object_error] = json_value_to_string(object, &allocator);
		TESTER_CHECK(object_error == true);
		TESTER_CHECK(object_encoded.data == nullptr);
	}
	TESTER_CHECK(allocator.bytes == 0);
}

TESTER_TEST("[CORE]: JSON Scratch Storage")
{
	memory::Arena_Allocator_Mark mark = memory::temp_allocator_mark();
	DEFER(memory::temp_allocator_reset_to_mark(mark));

	String number = string_from("0.");
	DEFER(string_deinit(number));
	string_append(number, '0', 200);
	string_append(number, '1');
	memory::Arena_Allocator_Mark before = memory::temp_allocator_mark();
	auto [scalar, scalar_error] = json_value_from_string(number, memory::temp_allocator());
	TESTER_CHECK(scalar_error == false);
	TESTER_CHECK(scalar.as_number == 1e-201);
	TESTER_CHECK(memory::temp_allocator_mark().arena_used == before.arena_used);

	const char *input = R"({"items":[{"name":"source"},[],{}],"replace":[1],"replace":{"ok":true}})";
	auto [value, error] = json_value_from_string(input, memory::temp_allocator());
	DEFER(json_value_deinit(value));
	TESTER_CHECK(error == false);
	JSON_Value copy = json_value_copy(value, memory::temp_allocator());
	DEFER(json_value_deinit(copy));
	auto [output, output_error] = json_value_to_string(copy, memory::temp_allocator());
	TESTER_CHECK(output_error == false);
	auto [expected, expected_error] = json_value_to_string(value);
	DEFER(string_deinit(expected));
	TESTER_CHECK(expected_error == false);

	Memory_Block reused = memory::allocate(memory::temp_allocator(), 16 * 1024, alignof(U64));
	::memset(reused.data, 0, reused.size);
	TESTER_CHECK(output == expected);
	JSON_Value items = json_value_object_find(copy, "items");
	TESTER_CHECK(items.as_array.count == 3);
	TESTER_CHECK(json_value_object_find(items.as_array[0], "name").as_string == "source");
	TESTER_CHECK(json_value_object_find(json_value_object_find(copy, "replace"), "ok").as_bool);

	JSON_Test_Allocator allocator = {};
	{
		memory::Arena_Allocator_Mark scope = memory::temp_allocator_mark();
		DEFER(memory::temp_allocator_reset_to_mark(scope));
		String filepath = platform_path_get_temp_directory(memory::temp_allocator());
		string_append(filepath, "test.core-json-scratch.json");
		DEFER(platform_path_delete_file(filepath));
		before = memory::temp_allocator_mark();
		Error write_error = json_value_to_file(value, filepath);
		TESTER_CHECK(write_error == false);
		TESTER_CHECK(memory::temp_allocator_mark().arena_used == before.arena_used);
		auto [loaded, load_error] = json_value_from_file(filepath, memory::temp_allocator());
		DEFER(json_value_deinit(loaded));
		TESTER_CHECK(load_error == false);
		JSON_Value loaded_items = json_value_object_find(loaded, "items");
		TESTER_CHECK(json_value_object_find(loaded_items.as_array[0], "name").as_string == "source");

		TESTER_CHECK(platform_path_write_file(filepath, number) == number.count);
		before = memory::temp_allocator_mark();
		auto [loaded_scalar, load_scalar_error] = json_value_from_file(filepath, &allocator);
		TESTER_CHECK(load_scalar_error == false);
		TESTER_CHECK(loaded_scalar.as_number == 1e-201);
		TESTER_CHECK(allocator.bytes == 0);
		TESTER_CHECK(memory::temp_allocator_mark().arena_used == before.arena_used);
	}

	before = memory::temp_allocator_mark();
	auto [invalid, invalid_error] = json_value_from_string("{\"items\":[1,", &allocator);
	TESTER_CHECK(invalid_error == true);
	TESTER_CHECK(invalid.kind == JSON_VALUE_KIND_INVALID);
	TESTER_CHECK(allocator.bytes == 0);
	TESTER_CHECK(memory::temp_allocator_mark().arena_used == before.arena_used);
}

TESTER_TEST("[CORE]: JSON Deep Values")
{
	constexpr U64 DEPTH = 20000;
	String input = string_init();
	DEFER(string_deinit(input));
	for (U64 i = 0; i < DEPTH; ++i)
		string_append(input, i % 2 == 0 ? "[" : "{\"child\":");

	string_append(input, "\"leaf\"");
	for (U64 i = DEPTH; i > 0; --i)
		string_append(input, i % 2 == 0 ? '}' : ']');

	JSON_Test_Allocator allocator = {};
	JSON_Test_Allocator copy_allocator = {};
	{
		JSON_Value copy = {};
		DEFER(json_value_deinit(copy));
		{
			auto [value, error] = json_value_from_string(input, &allocator);
			DEFER(json_value_deinit(value));
			TESTER_CHECK(error == false);
			copy = json_value_copy(value, &copy_allocator);
			TESTER_CHECK(copy.as_array.data != value.as_array.data);
		}

		TESTER_CHECK(allocator.bytes == 0);
		const JSON_Value *value = &copy;
		for (U64 i = 0; i < DEPTH; ++i)
		{
			if (i % 2 == 0)
			{
				TESTER_CHECK(value->kind == JSON_VALUE_KIND_ARRAY);
				TESTER_CHECK(value->as_array.count == 1);
				TESTER_CHECK(value->as_array.allocator == &copy_allocator);
				value = &value->as_array[0];
			}
			else
			{
				TESTER_CHECK(value->kind == JSON_VALUE_KIND_OBJECT);
				TESTER_CHECK(value->as_object.count == 1);
				TESTER_CHECK(value->as_object.slots.allocator == &copy_allocator);
				TESTER_CHECK(value->as_object.entries.allocator == &copy_allocator);
				const auto &entry = value->as_object.entries[0];
				TESTER_CHECK(entry.key == "child");
				TESTER_CHECK(entry.key.allocator == &copy_allocator);
				TESTER_CHECK(hash_table_find(value->as_object, entry.key)->value.kind == JSON_VALUE_KIND_ARRAY || i == DEPTH - 1);
				value = &entry.value;
			}
		}

		TESTER_CHECK(value->kind == JSON_VALUE_KIND_STRING);
		TESTER_CHECK(value->as_string == "leaf");
		TESTER_CHECK(value->as_string.allocator == &copy_allocator);
	}

	TESTER_CHECK(copy_allocator.bytes == 0);
	{
		String duplicate = string_from("{\"same\":");
		DEFER(string_deinit(duplicate));
		string_append(duplicate, input);
		string_append(duplicate, ",\"same\":true}");
		auto [value, error] = json_value_from_string(duplicate, &allocator);
		DEFER(json_value_deinit(value));
		TESTER_CHECK(error == false);
		TESTER_CHECK(value.as_object.count == 1);
		TESTER_CHECK(json_value_object_find(value, "same").as_bool == true);
	}

	TESTER_CHECK(allocator.bytes == 0);
	string_resize(input, input.count - 1);
	auto [truncated, truncated_error] = json_value_from_string(input, &allocator);
	TESTER_CHECK(truncated_error == true);
	TESTER_CHECK(truncated.kind == JSON_VALUE_KIND_INVALID);
	TESTER_CHECK(allocator.bytes == 0);
	string_clear(input);
	for (U64 i = 0; i < DEPTH; ++i)
		string_append(input, i % 2 == 0 ? "[" : "{\"child\":");

	auto [unfinished, unfinished_error] = json_value_from_string(input, &allocator);
	TESTER_CHECK(unfinished_error == true);
	TESTER_CHECK(unfinished.kind == JSON_VALUE_KIND_INVALID);
	TESTER_CHECK(allocator.bytes == 0);
}

TESTER_TEST("[CORE]: JSON Deep Output")
{
	constexpr U64 DEPTH = 2048;
	String input = string_init();
	DEFER(string_deinit(input));
	for (U64 i = 0; i < DEPTH; ++i)
		string_append(input, i % 2 == 0 ? "[" : "{\"child\":");

	string_append(input, "0");
	for (U64 i = DEPTH; i > 0; --i)
		string_append(input, i % 2 == 0 ? '}' : ']');

	JSON_Test_Allocator allocator = {};
	{
		auto [value, error] = json_value_from_string(input, &allocator);
		DEFER(json_value_deinit(value));
		TESTER_CHECK(error == false);
		{
			auto [encoded, encode_error] = json_value_to_string(value, &allocator);
			DEFER(string_deinit(encoded));
			TESTER_CHECK(encode_error == false);
			auto [decoded, decode_error] = json_value_from_string(encoded, &allocator);
			DEFER(json_value_deinit(decoded));
			TESTER_CHECK(decode_error == false);
			const JSON_Value *leaf = &decoded;
			for (U64 i = 0; i < DEPTH; ++i)
				leaf = i % 2 == 0 ? &leaf->as_array[0] : &leaf->as_object.entries[0].value;

			TESTER_CHECK(leaf->kind == JSON_VALUE_KIND_NUMBER);
			TESTER_CHECK(leaf->as_number == 0.0);
			auto [rewritten, rewrite_error] = json_value_to_string(decoded, &allocator);
			DEFER(string_deinit(rewritten));
			TESTER_CHECK(rewrite_error == false);
			TESTER_CHECK(rewritten == encoded);
		}

		JSON_Value *leaf = &value;
		for (U64 i = 0; i < DEPTH; ++i)
			leaf = i % 2 == 0 ? &leaf->as_array[0] : &leaf->as_object.entries[0].value;

		leaf->as_number = F64_INFINITY;
		U64 bytes = allocator.bytes;
		auto [failed, failure] = json_value_to_string(value, &allocator);
		TESTER_CHECK(failure == true);
		TESTER_CHECK(failed.data == nullptr);
		TESTER_CHECK(allocator.bytes == bytes);
	}

	TESTER_CHECK(allocator.bytes == 0);
}

TESTER_TEST("[CORE]: JSON Container Growth")
{
	String input = string_from("{");
	DEFER(string_deinit(input));
	for (U64 i = 0; i < 32; ++i)
	{
		if (i > 0)
			string_append(input, ',');

		String key = format("\"{}\":[", i);
		DEFER(string_deinit(key));
		string_append(input, key);
		for (U64 j = 0; j < 32; ++j)
		{
			if (j > 0)
				string_append(input, ',');

			string_append(input, "{\"leaf\":[\"value\",[],{}]}");
		}

		string_append(input, ']');
	}

	string_append(input, '}');
	JSON_Test_Allocator allocator = {};
	{
		auto [value, error] = json_value_from_string(input, &allocator);
		DEFER(json_value_deinit(value));
		TESTER_CHECK(error == false);
		JSON_Value copy = json_value_copy(value, &allocator);
		DEFER(json_value_deinit(copy));
		TESTER_CHECK(copy.as_object.count == 32);
		for (U64 i = 0; i < 32; ++i)
		{
			String key = format("{}", i);
			DEFER(string_deinit(key));
			JSON_Value array = json_value_object_find(copy, key);
			TESTER_CHECK(array.as_array.count == 32);
			for (const JSON_Value &element : array.as_array)
			{
				JSON_Value leaf = json_value_object_find(element, "leaf");
				TESTER_CHECK(leaf.as_array.count == 3);
				TESTER_CHECK(leaf.as_array[0].as_string == "value");
				TESTER_CHECK(leaf.as_array[1].as_array.count == 0);
				TESTER_CHECK(leaf.as_array[2].as_object.count == 0);
			}
		}

		auto [encoded, encode_error] = json_value_to_string(value, &allocator);
		DEFER(string_deinit(encoded));
		auto [copied, copy_error] = json_value_to_string(copy, &allocator);
		DEFER(string_deinit(copied));
		TESTER_CHECK(encode_error == false);
		TESTER_CHECK(copy_error == false);
		TESTER_CHECK(encoded == copied);
	}

	TESTER_CHECK(allocator.bytes == 0);
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