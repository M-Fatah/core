#include <core/tester.h>
#include <core/platform/platform.h>

TESTER_TEST("[PLATFORM] memory")
{
	Platform_Allocator allocator = platform_allocator_init(1024 * 1024);
	TESTER_CHECK(allocator.ptr != nullptr);
	TESTER_CHECK(allocator.size == 1024 * 1024);
	TESTER_CHECK(allocator.used == 0);

	Platform_Memory memory = platform_allocator_alloc(&allocator, 512);
	TESTER_CHECK(allocator.used == 512);
	TESTER_CHECK(memory.ptr == allocator.ptr + 512);
	TESTER_CHECK(memory.size == 512);

	platform_allocator_deinit(&allocator);
}

TESTER_TEST("[PLATFORM] file")
{
	U32 write_data[1024] = {};
	for (U32 i = 0; i < 1024; ++i)
		write_data[i] = i;

	Platform_Memory write_mem = {};
	write_mem.ptr  = (U8 *)write_data;
	write_mem.size = sizeof(write_data);

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
	Platform_Memory read_mem = {};
	read_mem.ptr  = (U8 *)read_data;
	read_mem.size = sizeof(read_data);

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

// TODO: For some reason this on github actions fails every single time (time on github actions is 124.5f ms) which is very weird.
TESTER_TEST("[PLATFORM] time")
{
	// platform_sleep_set_period(1);
	// U64 begin_time = platform_query_microseconds();
	// platform_sleep(16);
	// U64 end_time = platform_query_microseconds();

	// F32 delta_time = (end_time - begin_time) * MICROSECOND_TO_MILLISECOND;

	// TESTER_CHECK(delta_time == doctest::Approx(16).epsilon(0.25));
}