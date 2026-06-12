#include <core/tester.h>
#include <core/json.h>
#include <core/base64.h>
#include <core/log.h>
#include <core/result.h>
#include <core/memory/memory.h>
#include <core/memory/pool_allocator.h>
#include <core/memory/arena_allocator.h>
#include <core/memory/virtual_allocator.h>
#include <core/platform/platform.h>

TESTER_TEST("[CORE]: Arena_Allocator")
{
	memory::Arena_Allocator *arena = memory::arena_allocator_init(1024);
	DEFER(memory::arena_allocator_deinit(arena));

	Memory_Block a = memory::arena_allocator_allocate(arena, 4, 1);
	Memory_Block b = memory::arena_allocator_allocate(arena, 8, 1);

	TESTER_CHECK(a.data != nullptr);
	TESTER_CHECK(b.data != nullptr);

	TESTER_CHECK(memory::arena_allocator_get_used_size(arena) == 12);
	TESTER_CHECK(memory::arena_allocator_get_peak_size(arena) == 12);

	arena_allocator_clear(arena);

	TESTER_CHECK(memory::arena_allocator_get_used_size(arena) == 0);
	TESTER_CHECK(memory::arena_allocator_get_peak_size(arena) == 12);

	memory::arena_allocator_allocate(arena, 2048, 1);

	TESTER_CHECK(memory::arena_allocator_get_used_size(arena) == 2048);
	TESTER_CHECK(memory::arena_allocator_get_peak_size(arena) == 2048);

	arena_allocator_clear(arena);

	TESTER_CHECK(memory::arena_allocator_get_used_size(arena) == 0);
	TESTER_CHECK(memory::arena_allocator_get_peak_size(arena) == 2048);
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
}

TESTER_TEST("[CORE]: Virtual_Allocator")
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

	memory::Virtual_Allocator *allocator = memory::virtual_allocator_init();
	DEFER(memory::virtual_allocator_deinit(allocator));

	Memory_Block block = memory::virtual_allocator_allocate(allocator, page_size + 16, 16);
	DEFER(memory::virtual_allocator_deallocate(allocator, block));
	TESTER_CHECK(block.data != nullptr);
	TESTER_CHECK(block.size == page_size + 16);
	TESTER_CHECK(((U64)block.data & 15) == 0);
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