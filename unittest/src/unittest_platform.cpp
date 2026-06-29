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
		.data = &context
	});

	platform_thread_join(thread);
	TESTER_CHECK(context.value == 42);
	platform_thread_deinit(thread);
	TESTER_CHECK(platform_get_logical_processor_count() > 0);
}

TESTER_TEST("[PLATFORM] path utilities")
{
	String executable_path = platform_path_get_executable_path(memory::temp_allocator());
	TESTER_CHECK(executable_path.count > 0);
	TESTER_CHECK(platform_path_is_file(executable_path));

	String module_path = platform_path_get_current_module_path(memory::temp_allocator());
	TESTER_CHECK(module_path.count > 0);
	TESTER_CHECK(platform_path_is_file(module_path));

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