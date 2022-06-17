#include <core/platform/platform.h>

#include <doctest/doctest.h>

TEST_CASE("[PLATFORM] memory")
{
	Platform_Allocator allocator = platform_allocator_init(1024 * 1024);
	CHECK(allocator.ptr != nullptr);
	CHECK(allocator.size == 1024 * 1024);
	CHECK(allocator.used == 0);

	Platform_Memory memory = platform_allocator_alloc(&allocator, 512);
	CHECK(allocator.used == 512);
	CHECK(memory.ptr == allocator.ptr + 512);
	CHECK(memory.size == 512);

	platform_allocator_deinit(&allocator);
}

TEST_CASE("[PLATFORM] file")
{
	u32 write_data[1024] = {};
	for (u32 i = 0; i < 1024; ++i)
		write_data[i] = i;

	Platform_Memory write_mem = {};
	write_mem.ptr  = (u8 *)write_data;
	write_mem.size = sizeof(write_data);

	const char *filepath = "test.platform";

	u64 written_size = platform_file_write(filepath, write_mem);
	CHECK(written_size == write_mem.size);

	u64 file_size = platform_file_size(filepath);
	CHECK(file_size == write_mem.size);

	u32 read_data[1024] = {};
	Platform_Memory read_mem = {};
	read_mem.ptr  = (u8 *)read_data;
	read_mem.size = sizeof(read_data);

	u64 read_size = platform_file_read(filepath, read_mem);
	CHECK(read_size == written_size);
	CHECK(read_size == read_mem.size);

	bool same = true;
	for (u32 i = 0; i < 1024; ++i)
	{
		if (read_data[i] != write_data[i])
		{
			same = false;
			break;
		}
	}
	CHECK(same == true);
	CHECK(platform_file_delete("test.platform"));
}

TEST_CASE("[PLATFORM] time")
{
	platform_sleep_set_period(1);
	u64 begin_time = platform_query_microseconds();
	platform_sleep(16);
	u64 end_time = platform_query_microseconds();

	f32 delta_time = (end_time - begin_time) * MICROSECOND_TO_MILLISECOND;

	CHECK(delta_time == doctest::Approx(16).epsilon(0.1));
}