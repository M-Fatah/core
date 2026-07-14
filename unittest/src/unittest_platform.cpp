#include <core/tester.h>
#include <core/platform/platform.h>

TESTER_TEST("[PLATFORM] virtual memory")
{
	U64 page_size = platform_virtual_memory_get_page_size();
	TESTER_CHECK(page_size > 0);
	TESTER_CHECK((page_size & (page_size - 1)) == 0);

	Memory_Block block = platform_virtual_memory_reserve(page_size);
	TESTER_CHECK(block.data != nullptr);
	TESTER_CHECK(block.size == page_size);

	TESTER_CHECK(platform_virtual_memory_commit(block));
	U8 *data = (U8 *)block.data;
	data[0] = 1;
	data[page_size - 1] = 2;
	TESTER_CHECK(data[0] == 1);
	TESTER_CHECK(data[page_size - 1] == 2);

	TESTER_CHECK(platform_virtual_memory_decommit(block));
	TESTER_CHECK(platform_virtual_memory_commit(block));
	platform_virtual_memory_release(block);
}

TESTER_TEST("[PLATFORM] callstack")
{
	constexpr U32 CALLSTACK_FRAME_COUNT = 16;
	void *callstack[CALLSTACK_FRAME_COUNT] = {};
	U32 frame_count = platform_callstack_capture(callstack, CALLSTACK_FRAME_COUNT);

	#if DEBUG
		TESTER_CHECK(frame_count > 0);

		Platform_Callstack_Frame frames[CALLSTACK_FRAME_COUNT] = {};
		platform_callstack_resolve(callstack, frames, frame_count);

		bool symbol_found = false;
		bool line_found = false;
		for (U32 i = 0; i < frame_count; ++i)
		{
			TESTER_CHECK(frames[i].address == callstack[i]);
			symbol_found = symbol_found || frames[i].symbol_found;
			line_found = line_found || frames[i].line_found;
		}
		TESTER_CHECK(symbol_found);
		#if PLATFORM_WINDOWS
			TESTER_CHECK(line_found);
		#endif
	#else
		TESTER_CHECK(frame_count == 0);
	#endif
}

struct Platform_Thread_Test_Context
{
	U32 value;
};

inline static void
_platform_thread_test_entry(void *data)
{
	Platform_Thread_Test_Context *context = (Platform_Thread_Test_Context *)data;
	platform_thread_sleep(0);
	context->value = 42;
}

TESTER_TEST("[PLATFORM] thread")
{
	Platform_Thread_Test_Context context = {};
	Platform_Thread *thread = platform_thread_init(Platform_Thread_Desc {
		.function = _platform_thread_test_entry,
		.data = &context,
		.name = "CoreTest"
	});

	platform_thread_join(thread);
	TESTER_CHECK(context.value == 42);
	platform_thread_deinit(thread);
	TESTER_CHECK(platform_get_logical_processor_count() > 0);
}

struct Platform_Mutex_Test_Context
{
	Platform_Mutex *mutex;
	U32 *counter;
	U32 iteration_count;
};

inline static void
_platform_mutex_test_entry(void *data)
{
	Platform_Mutex_Test_Context *context = (Platform_Mutex_Test_Context *)data;
	for (U32 i = 0; i < context->iteration_count; ++i)
	{
		platform_mutex_lock(context->mutex);
		*context->counter += 1;
		platform_mutex_unlock(context->mutex);
	}
}

TESTER_TEST("[PLATFORM] mutex")
{
	constexpr U32 THREAD_COUNT = 4;
	constexpr U32 ITERATION_COUNT = 10000;

	Platform_Mutex *mutex = platform_mutex_init();
	U32 counter = 0;
	Platform_Thread *threads[THREAD_COUNT] = {};
	Platform_Mutex_Test_Context contexts[THREAD_COUNT] = {};

	for (U32 i = 0; i < THREAD_COUNT; ++i)
	{
		contexts[i] = Platform_Mutex_Test_Context {
			.mutex = mutex,
			.counter = &counter,
			.iteration_count = ITERATION_COUNT
		};
		threads[i] = platform_thread_init(Platform_Thread_Desc {
			.function = _platform_mutex_test_entry,
			.data = &contexts[i]
		});
	}

	for (U32 i = 0; i < THREAD_COUNT; ++i)
		platform_thread_deinit(threads[i]);

	TESTER_CHECK(counter == THREAD_COUNT * ITERATION_COUNT);
	platform_mutex_deinit(mutex);
}

struct Platform_Condition_Variable_Signal_Test_Context
{
	Platform_Mutex *mutex;
	Platform_Condition_Variable *waiting_condition_variable;
	Platform_Condition_Variable *ready_condition_variable;
	bool waiting;
	bool ready;
	bool woke;
};

inline static void
_platform_condition_variable_signal_test_entry(void *data)
{
	Platform_Condition_Variable_Signal_Test_Context *context = (Platform_Condition_Variable_Signal_Test_Context *)data;

	platform_mutex_lock(context->mutex);
	context->waiting = true;
	platform_condition_variable_signal(context->waiting_condition_variable);
	while (!context->ready)
		platform_condition_variable_wait(context->ready_condition_variable, context->mutex);
	context->woke = true;
	platform_mutex_unlock(context->mutex);
}

TESTER_TEST("[PLATFORM] condition variable signal")
{
	Platform_Mutex *mutex = platform_mutex_init();
	Platform_Condition_Variable *waiting_condition_variable = platform_condition_variable_init();
	Platform_Condition_Variable *ready_condition_variable = platform_condition_variable_init();
	Platform_Condition_Variable_Signal_Test_Context context = {
		.mutex = mutex,
		.waiting_condition_variable = waiting_condition_variable,
		.ready_condition_variable = ready_condition_variable
	};

	Platform_Thread *thread = platform_thread_init(Platform_Thread_Desc {
		.function = _platform_condition_variable_signal_test_entry,
		.data = &context
	});

	platform_mutex_lock(mutex);
	while (!context.waiting)
		platform_condition_variable_wait(waiting_condition_variable, mutex);
	context.ready = true;
	platform_condition_variable_signal(ready_condition_variable);
	platform_mutex_unlock(mutex);

	platform_thread_deinit(thread);
	TESTER_CHECK(context.woke);
	platform_condition_variable_deinit(ready_condition_variable);
	platform_condition_variable_deinit(waiting_condition_variable);
	platform_mutex_deinit(mutex);
}

struct Platform_Condition_Variable_Broadcast_Test_Context
{
	Platform_Mutex *mutex;
	Platform_Condition_Variable *waiting_condition_variable;
	Platform_Condition_Variable *ready_condition_variable;
	U32 *waiting_count;
	U32 *wake_count;
	bool *ready;
};

inline static void
_platform_condition_variable_broadcast_test_entry(void *data)
{
	Platform_Condition_Variable_Broadcast_Test_Context *context = (Platform_Condition_Variable_Broadcast_Test_Context *)data;

	platform_mutex_lock(context->mutex);
	*context->waiting_count += 1;
	platform_condition_variable_signal(context->waiting_condition_variable);
	while (!*context->ready)
		platform_condition_variable_wait(context->ready_condition_variable, context->mutex);
	*context->wake_count += 1;
	platform_mutex_unlock(context->mutex);
}

TESTER_TEST("[PLATFORM] condition variable broadcast")
{
	constexpr U32 THREAD_COUNT = 4;

	Platform_Mutex *mutex = platform_mutex_init();
	Platform_Condition_Variable *waiting_condition_variable = platform_condition_variable_init();
	Platform_Condition_Variable *ready_condition_variable = platform_condition_variable_init();
	Platform_Thread *threads[THREAD_COUNT] = {};
	Platform_Condition_Variable_Broadcast_Test_Context contexts[THREAD_COUNT] = {};
	U32 waiting_count = 0;
	U32 wake_count = 0;
	bool ready = false;

	for (U32 i = 0; i < THREAD_COUNT; ++i)
	{
		contexts[i] = Platform_Condition_Variable_Broadcast_Test_Context {
			.mutex = mutex,
			.waiting_condition_variable = waiting_condition_variable,
			.ready_condition_variable = ready_condition_variable,
			.waiting_count = &waiting_count,
			.wake_count = &wake_count,
			.ready = &ready
		};
		threads[i] = platform_thread_init(Platform_Thread_Desc {
			.function = _platform_condition_variable_broadcast_test_entry,
			.data = &contexts[i]
		});
	}

	platform_mutex_lock(mutex);
	while (waiting_count != THREAD_COUNT)
		platform_condition_variable_wait(waiting_condition_variable, mutex);
	ready = true;
	platform_condition_variable_broadcast(ready_condition_variable);
	platform_mutex_unlock(mutex);

	for (U32 i = 0; i < THREAD_COUNT; ++i)
		platform_thread_deinit(threads[i]);

	TESTER_CHECK(wake_count == THREAD_COUNT);
	platform_condition_variable_deinit(ready_condition_variable);
	platform_condition_variable_deinit(waiting_condition_variable);
	platform_mutex_deinit(mutex);
}

TESTER_TEST("[PLATFORM] path utilities")
{
	String executable_path = platform_path_get_executable_path(memory::temp_allocator());
	TESTER_CHECK(executable_path.count > 0);
	TESTER_CHECK(platform_path_is_file(executable_path));
	TESTER_CHECK(executable_path.count == string_literal(executable_path.data).count);
	TESTER_CHECK(executable_path.data[executable_path.count] == '\0');

	String module_path = platform_path_get_current_module_path(memory::temp_allocator());
	TESTER_CHECK(module_path.count > 0);
	TESTER_CHECK(platform_path_is_file(module_path));
	TESTER_CHECK(module_path.count == string_literal(module_path.data).count);
	TESTER_CHECK(module_path.data[module_path.count] == '\0');

	String working_directory = platform_path_get_current_working_directory(memory::temp_allocator());
	TESTER_CHECK(working_directory.count > 0);
	TESTER_CHECK(working_directory.count == string_literal(working_directory.data).count);
	TESTER_CHECK(working_directory.data[working_directory.count] == '\0');

	String absolute_path = platform_path_get_absolute(".", memory::temp_allocator());
	TESTER_CHECK(absolute_path.count > 0);
	TESTER_CHECK(absolute_path.count == string_literal(absolute_path.data).count);
	TESTER_CHECK(absolute_path.data[absolute_path.count] == '\0');

	TESTER_CHECK(platform_path_get_file_name("folder\\sub/file.txt", memory::temp_allocator()) == "file.txt");
	TESTER_CHECK(platform_path_get_file_name("file.txt", memory::temp_allocator()) == "file.txt");
	TESTER_CHECK(platform_path_get_file_name("folder/", memory::temp_allocator()).count == 0);
	TESTER_CHECK(platform_path_get_file_name(String{}, memory::temp_allocator()).count == 0);

	String missing_env = platform_environment_variable_get("__CORE_UNITTEST_MISSING_ENVIRONMENT_VARIABLE__", memory::temp_allocator());
	TESTER_CHECK(missing_env.count == 0);
}

TESTER_TEST("[PLATFORM] file")
{
	U32 write_data[1024] = {};
	for (U32 i = 0; i < 1024; ++i)
		write_data[i] = i;

	Memory_Block write_mem = {(void *)write_data, sizeof(write_data)};

	String temp_directory = platform_path_get_temp_directory(memory::temp_allocator());
	String filepath = string_copy(temp_directory, memory::temp_allocator());
	string_append(filepath, "test.platform");
	String copy_filepath = string_copy(temp_directory, memory::temp_allocator());
	string_append(copy_filepath, "test_copy.platform");

	U64 written_size = platform_file_write(filepath.data, write_mem);
	TESTER_CHECK(written_size == write_mem.size);

	U64 file_size = platform_file_size(filepath.data);
	TESTER_CHECK(file_size == write_mem.size);

	U32 read_data[1024] = {};
	Memory_Block read_mem = {read_data, sizeof(read_data)};

	U64 read_size = platform_file_read(filepath.data, read_mem);
	TESTER_CHECK(read_size == written_size);
	TESTER_CHECK(read_size == read_mem.size);

	bool same = true;
	for (U32 i = 0; i < 1024; ++i)
	{
		if (read_data[i] != write_data[i])
		{
			same = false;
			break;
		}
	}
	TESTER_CHECK(same == true);

	bool copy_result = platform_file_copy(filepath.data, copy_filepath.data);
	TESTER_CHECK(copy_result == true);

	read_size = platform_file_read(copy_filepath.data, read_mem);
	TESTER_CHECK(read_size == written_size);
	TESTER_CHECK(read_size == read_mem.size);

	same = true;
	for (U32 i = 0; i < 1024; ++i)
	{
		if (read_data[i] != write_data[i])
		{
			same = false;
			break;
		}
	}
	TESTER_CHECK(same == true);

	TESTER_CHECK(platform_file_delete(filepath.data));
	TESTER_CHECK(platform_file_delete(copy_filepath.data));
}