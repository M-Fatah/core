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