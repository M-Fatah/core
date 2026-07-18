#include <core/tester.h>
#include <core/defer.h>
#include <core/platform/platform.h>

inline static bool
_platform_test_bytes_equal(const char *lhs, const char *rhs, U64 size)
{
	for (U64 i = 0; i < size; ++i)
		if (lhs[i] != rhs[i])
			return false;
	return true;
}

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
	String empty_filepath = string_copy(temp_directory, memory::temp_allocator());
	string_append(empty_filepath, "test_empty.platform");
	String missing_filepath = string_copy(temp_directory, memory::temp_allocator());
	string_append(missing_filepath, "test_missing.platform");
	if (platform_path_is_valid(missing_filepath))
		TESTER_CHECK(platform_path_delete_file(missing_filepath));
	TESTER_CHECK(!platform_path_is_valid(missing_filepath));

	U64 written_size = platform_path_write_file(filepath.data, write_mem);
	TESTER_CHECK(written_size == write_mem.size);
	TESTER_CHECK(platform_path_is_valid(filepath));
	TESTER_CHECK(platform_path_is_file(filepath));

	U64 file_size = platform_path_get_file_size(filepath);
	TESTER_CHECK(file_size == write_mem.size);

	String read_content = platform_path_read_file(filepath.data);
	DEFER(string_deinit(read_content));
	TESTER_CHECK(read_content.allocator == memory::heap_allocator());
	TESTER_CHECK(read_content.count == written_size);
	TESTER_CHECK(read_content.data[read_content.count] == '\0');
	TESTER_CHECK(_platform_test_bytes_equal(read_content.data, (const char *)write_data, written_size));

	U8 copy_padding[sizeof(write_data) + 1] = {};
	Memory_Block copy_padding_mem = {copy_padding, sizeof(copy_padding)};
	TESTER_CHECK(platform_path_write_file(copy_filepath, copy_padding_mem) == copy_padding_mem.size);

	bool copy_result = platform_path_copy_file(filepath, copy_filepath);
	TESTER_CHECK(copy_result == true);

	String copy_content = platform_path_read_file(copy_filepath.data, memory::temp_allocator());
	DEFER(string_deinit(copy_content));
	TESTER_CHECK(copy_content.allocator == memory::temp_allocator());
	TESTER_CHECK(copy_content.count == written_size);
	TESTER_CHECK(_platform_test_bytes_equal(copy_content.data, (const char *)write_data, written_size));

	constexpr char TRUNCATED_CONTENT[] = "short";
	U64 truncated_size = platform_path_write_file(copy_filepath.data, TRUNCATED_CONTENT);
	TESTER_CHECK(truncated_size == sizeof(TRUNCATED_CONTENT) - 1);

	String truncated_content = platform_path_read_file(copy_filepath.data);
	DEFER(string_deinit(truncated_content));
	TESTER_CHECK(truncated_content.count == truncated_size);
	TESTER_CHECK(_platform_test_bytes_equal(truncated_content.data, TRUNCATED_CONTENT, truncated_size));

	U64 empty_size = platform_path_write_file(empty_filepath, Memory_Block{});
	TESTER_CHECK(empty_size == 0);
	TESTER_CHECK(platform_path_is_valid(empty_filepath));
	TESTER_CHECK(platform_path_get_file_size(empty_filepath) == 0);

	String empty_content = platform_path_read_file(empty_filepath.data);
	DEFER(string_deinit(empty_content));
	TESTER_CHECK(empty_content.count == 0);
	TESTER_CHECK(empty_content.data[0] == '\0');

	String missing_content = platform_path_read_file(missing_filepath.data);
	DEFER(string_deinit(missing_content));
	TESTER_CHECK(missing_content.count == 0);
	TESTER_CHECK(missing_content.data[0] == '\0');

	TESTER_CHECK(platform_path_delete_file(filepath));
	TESTER_CHECK(platform_path_delete_file(copy_filepath));
	TESTER_CHECK(platform_path_delete_file(empty_filepath));
	TESTER_CHECK(!platform_path_is_valid(filepath));
	TESTER_CHECK(!platform_path_is_valid(copy_filepath));
	TESTER_CHECK(!platform_path_is_valid(empty_filepath));
}

TESTER_TEST("[PLATFORM] file handle")
{
	constexpr char INITIAL_DATA[] = "0123456789";
	constexpr char REPLACEMENT_DATA[] = "AB";
	constexpr char APPEND_DATA[] = "XY";
	constexpr U64 INITIAL_DATA_SIZE = sizeof(INITIAL_DATA) - 1;
	constexpr U64 REPLACEMENT_DATA_SIZE = sizeof(REPLACEMENT_DATA) - 1;
	constexpr U64 APPEND_DATA_SIZE = sizeof(APPEND_DATA) - 1;

	String temp_directory = platform_path_get_temp_directory(memory::temp_allocator());
	String filepath = string_copy(temp_directory, memory::temp_allocator());
	string_append(filepath, "test_handle.platform");
	platform_path_delete_file(filepath);
	DEFER(platform_path_delete_file(filepath););

	Platform_File_Handle file = platform_file_open(filepath, PLATFORM_FILE_MODE_READ);
	TESTER_CHECK(file == PLATFORM_FILE_HANDLE_INVALID);
	if (file != PLATFORM_FILE_HANDLE_INVALID)
		platform_file_close(file);

	file = platform_file_open(filepath, PLATFORM_FILE_MODE_WRITE);
	TESTER_CHECK(file != PLATFORM_FILE_HANDLE_INVALID);
	if (file == PLATFORM_FILE_HANDLE_INVALID)
		return;
	TESTER_CHECK(platform_file_size(file) == 0);
	TESTER_CHECK(platform_file_tell(file) == 0);
	TESTER_CHECK(platform_file_write(file, INITIAL_DATA, INITIAL_DATA_SIZE) == INITIAL_DATA_SIZE);
	TESTER_CHECK(platform_file_tell(file) == INITIAL_DATA_SIZE);
	TESTER_CHECK(platform_file_size(file) == INITIAL_DATA_SIZE);
	platform_file_close(file);

	file = platform_file_open(filepath, PLATFORM_FILE_MODE_READ);
	TESTER_CHECK(file != PLATFORM_FILE_HANDLE_INVALID);
	if (file == PLATFORM_FILE_HANDLE_INVALID)
		return;
	TESTER_CHECK(platform_file_size(file) == INITIAL_DATA_SIZE);
	char read_data[INITIAL_DATA_SIZE] = {};
	TESTER_CHECK(platform_file_read(file, read_data, 4) == 4);
	TESTER_CHECK(_platform_test_bytes_equal(read_data, "0123", 4));
	TESTER_CHECK(platform_file_tell(file) == 4);
	TESTER_CHECK(platform_file_seek(file, 2, PLATFORM_FILE_SEEK_ORIGIN_CURRENT));
	TESTER_CHECK(platform_file_tell(file) == 6);
	TESTER_CHECK(platform_file_read(file, read_data, 2) == 2);
	TESTER_CHECK(_platform_test_bytes_equal(read_data, "67", 2));
	TESTER_CHECK(platform_file_seek(file, -2, PLATFORM_FILE_SEEK_ORIGIN_END));
	TESTER_CHECK(platform_file_tell(file) == INITIAL_DATA_SIZE - 2);
	TESTER_CHECK(platform_file_read(file, read_data, 2) == 2);
	TESTER_CHECK(_platform_test_bytes_equal(read_data, "89", 2));
	TESTER_CHECK(platform_file_seek(file, 0, PLATFORM_FILE_SEEK_ORIGIN_BEGIN));
	TESTER_CHECK(platform_file_read(file, read_data, INITIAL_DATA_SIZE) == INITIAL_DATA_SIZE);
	TESTER_CHECK(_platform_test_bytes_equal(read_data, INITIAL_DATA, INITIAL_DATA_SIZE));
	TESTER_CHECK(platform_file_read(file, read_data, 1) == 0);
	platform_file_close(file);

	file = platform_file_open(filepath, PLATFORM_FILE_MODE_READ_WRITE);
	TESTER_CHECK(file != PLATFORM_FILE_HANDLE_INVALID);
	if (file == PLATFORM_FILE_HANDLE_INVALID)
		return;
	TESTER_CHECK(platform_file_seek(file, 4, PLATFORM_FILE_SEEK_ORIGIN_BEGIN));
	TESTER_CHECK(platform_file_write(file, REPLACEMENT_DATA, REPLACEMENT_DATA_SIZE) == REPLACEMENT_DATA_SIZE);
	TESTER_CHECK(platform_file_tell(file) == 6);
	TESTER_CHECK(platform_file_size(file) == INITIAL_DATA_SIZE);
	TESTER_CHECK(platform_file_seek(file, 0, PLATFORM_FILE_SEEK_ORIGIN_BEGIN));
	TESTER_CHECK(platform_file_read(file, read_data, INITIAL_DATA_SIZE) == INITIAL_DATA_SIZE);
	TESTER_CHECK(_platform_test_bytes_equal(read_data, "0123AB6789", INITIAL_DATA_SIZE));
	platform_file_close(file);

	file = platform_file_open(filepath, PLATFORM_FILE_MODE_APPEND);
	TESTER_CHECK(file != PLATFORM_FILE_HANDLE_INVALID);
	if (file == PLATFORM_FILE_HANDLE_INVALID)
		return;
	TESTER_CHECK(platform_file_write(file, APPEND_DATA, APPEND_DATA_SIZE) == APPEND_DATA_SIZE);
	TESTER_CHECK(platform_file_size(file) == INITIAL_DATA_SIZE + APPEND_DATA_SIZE);
	platform_file_close(file);

	file = platform_file_open(filepath, PLATFORM_FILE_MODE_READ);
	TESTER_CHECK(file != PLATFORM_FILE_HANDLE_INVALID);
	if (file == PLATFORM_FILE_HANDLE_INVALID)
		return;
	char final_data[INITIAL_DATA_SIZE + APPEND_DATA_SIZE] = {};
	TESTER_CHECK(platform_file_read(file, final_data, sizeof(final_data)) == sizeof(final_data));
	TESTER_CHECK(_platform_test_bytes_equal(final_data, "0123AB6789XY", sizeof(final_data)));
	platform_file_close(file);

	file = platform_file_open(filepath, PLATFORM_FILE_MODE_WRITE);
	TESTER_CHECK(file != PLATFORM_FILE_HANDLE_INVALID);
	if (file == PLATFORM_FILE_HANDLE_INVALID)
		return;
	TESTER_CHECK(platform_file_size(file) == 0);
	platform_file_close(file);
}

TESTER_TEST("[PLATFORM] clipboard item description")
{
	U8 source[] = {1, 2, 3};
	Platform_Clipboard_Item_Desc item {
		.media_type = string_literal(PLATFORM_CLIPBOARD_MEDIA_TYPE_BINARY),
		.data = slice_from(source)
	};
	TESTER_CHECK(item.media_type == PLATFORM_CLIPBOARD_MEDIA_TYPE_BINARY);
	TESTER_CHECK(item.media_type.allocator == nullptr);
	TESTER_CHECK(item.data.data == source);
	TESTER_CHECK(item.data.count == count_of(source));
	TESTER_CHECK(item.data[0] == source[0]);
	TESTER_CHECK(item.data[2] == source[2]);
}

TESTER_TEST("[PLATFORM] window description")
{
	Platform_Window_Desc desc {
		.width = 1280,
		.height = 720,
		.title = "Core"
	};
	TESTER_CHECK(desc.width == 1280);
	TESTER_CHECK(desc.height == 720);
	TESTER_CHECK(desc.title == string_literal("Core"));
}