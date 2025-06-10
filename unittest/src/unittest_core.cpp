#include <core/json.h>
#include <core/base64.h>
#include <core/log.h>
#include <core/result.h>
#include <core/memory/memory.h>
#include <core/memory/pool_allocator.h>
#include <core/memory/arena_allocator.h>
#include <core/platform/platform.h>

#include <doctest/doctest.h>

TEST_CASE("[CORE]: Arena_Allocator")
{
	memory::Arena_Allocator *arena = memory::arena_allocator_init(1024);
	DEFER(memory::arena_allocator_deinit(arena));

	void *a = memory::arena_allocator_allocate(arena, 4);
	void *b = memory::arena_allocator_allocate(arena, 8);

	CHECK(a != nullptr);
	CHECK(b != nullptr);

	CHECK(memory::arena_allocator_get_used_size(arena) == 12);
	CHECK(memory::arena_allocator_get_peak_size(arena) == 12);

	arena_allocator_clear(arena);

	CHECK(memory::arena_allocator_get_used_size(arena) == 0);
	CHECK(memory::arena_allocator_get_peak_size(arena) == 12);

	// TODO: Make arena dynamic again! (no pun intended).
	// arena_allocator_allocate(arena, 2048);

	// CHECK(memory::arena_allocator_get_used_size(arena) == 2048);
	// CHECK(memory::arena_allocator_get_peak_size(arena) == 2048);

	// arena_allocator_clear(arena);

	// CHECK(memory::arena_allocator_get_used_size(arena) == 0);
	// CHECK(memory::arena_allocator_get_peak_size(arena) == 2048);
}

TEST_CASE("[CORE]: Pool_Allocator")
{
	struct Entity
	{
		f32 x, y, z;
	};

	memory::Pool_Allocator *pool = memory::pool_allocator_init(sizeof(Entity), 10);
	DEFER(memory::pool_allocator_deinit(pool));

	Entity *e1 = (Entity *)memory::pool_allocator_allocate(pool);
	CHECK(e1 != nullptr);
	*e1 = Entity{1.0f, 2.0f, 3.0f};
	memory::pool_allocator_deallocate(pool, e1);

	Entity *e2 = (Entity *)memory::pool_allocator_allocate(pool);
	CHECK(e2 == e1);

	Entity *e3 = (Entity *)memory::pool_allocator_allocate(pool);
	memory::pool_allocator_deallocate(pool, e3);
	memory::pool_allocator_deallocate(pool, e2);

	Entity *p4 = (Entity *)memory::pool_allocator_allocate(pool);
	CHECK(p4 == e2);

	Entity *p5 = (Entity *)memory::pool_allocator_allocate(pool);
	CHECK(p5 == e3);
}

inline static Result<i32>
_result_test_with_default_error_pseudo_disk_read(bool success)
{
	if (success)
		return 1;
	return Error{"Could not read from disk."};
}

enum class PSEUDO_DISK_READ_RESULT_CODE { OK, NOT_OK };

inline static Result<i32, PSEUDO_DISK_READ_RESULT_CODE>
_result_test_with_custom_error_pseudo_disk_read(bool success)
{
	if (success)
		return 1;
	return PSEUDO_DISK_READ_RESULT_CODE::NOT_OK;
}

TEST_CASE("[CORE]: Result")
{
	SUBCASE("default error - success")
	{
		auto [result, error] = _result_test_with_default_error_pseudo_disk_read(true);
		CHECK(error == false);
		CHECK(result == 1);
	}

	SUBCASE("default error - failure")
	{
		auto [result, error] = _result_test_with_default_error_pseudo_disk_read(false);
		CHECK(error == true);
	}

	SUBCASE("custom error - success")
	{
		auto [result, error] = _result_test_with_custom_error_pseudo_disk_read(true);
		CHECK(error == PSEUDO_DISK_READ_RESULT_CODE::OK);
		CHECK(result == 1);
	}

	SUBCASE("custom error - failure")
	{
		auto [result, error] = _result_test_with_custom_error_pseudo_disk_read(false);
		CHECK(error == PSEUDO_DISK_READ_RESULT_CODE::NOT_OK);
	}
}

TEST_CASE("[CORE]: JSON")
{
	// TODO: Add json_value_object_find().
	SUBCASE("parse string")
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

		CHECK(error == false);
		CHECK(value.kind == JSON_VALUE_KIND_OBJECT);
		CHECK(value.as_object.count == 7);

		{
			auto name_entry = hash_table_find(value.as_object, string_literal("name"));
			CHECK(name_entry != nullptr);
			CHECK(name_entry->key == "name");
			CHECK(name_entry->value.kind == JSON_VALUE_KIND_STRING);
			CHECK(name_entry->value.as_string == "Mist");
		}
		{
			auto nil_entry = hash_table_find(value.as_object, string_literal("nil"));
			CHECK(nil_entry != nullptr);
			CHECK(nil_entry->key == "nil");
			CHECK(nil_entry->value.kind == JSON_VALUE_KIND_NULL);
		}
		{
			auto right_entry = hash_table_find(value.as_object, string_literal("right"));
			CHECK(right_entry != nullptr);
			CHECK(right_entry->key == "right");
			CHECK(right_entry->value.kind == JSON_VALUE_KIND_BOOL);
			CHECK(right_entry->value.as_bool == true);
		}
		{
			auto wrong_entry = hash_table_find(value.as_object, string_literal("wrong"));
			CHECK(wrong_entry != nullptr);
			CHECK(wrong_entry->key == "wrong");
			CHECK(wrong_entry->value.kind == JSON_VALUE_KIND_BOOL);
			CHECK(wrong_entry->value.as_bool == false);
		}
		{
			auto number_entry = hash_table_find(value.as_object, string_literal("number"));
			CHECK(number_entry != nullptr);
			CHECK(number_entry->key == "number");
			CHECK(number_entry->value.kind == JSON_VALUE_KIND_NUMBER);
			CHECK(number_entry->value.as_number == 123.456);
		}
		{
			auto array_entry = hash_table_find(value.as_object, string_literal("array"));
			CHECK(array_entry != nullptr);
			CHECK(array_entry->key == "array");
			CHECK(array_entry->value.as_array.count == 2);
			CHECK(array_entry->value.as_array[0].kind == JSON_VALUE_KIND_NUMBER);
			CHECK(array_entry->value.as_array[0].as_number == 1);
			CHECK(array_entry->value.as_array[1].kind == JSON_VALUE_KIND_BOOL);
			CHECK(array_entry->value.as_array[1].as_bool == false);
		}
		{
			auto sub_object_entry = hash_table_find(value.as_object, string_literal("sub_object"));
			CHECK(sub_object_entry != nullptr);
			CHECK(sub_object_entry->key == "sub_object");
			CHECK(sub_object_entry->value.kind == JSON_VALUE_KIND_OBJECT);

			auto sub_object_name_entry = hash_table_find(sub_object_entry->value.as_object, string_literal("name"));
			CHECK(sub_object_name_entry != nullptr);
			CHECK(sub_object_name_entry->key == "name");
			CHECK(sub_object_name_entry->value.kind == JSON_VALUE_KIND_STRING);
			CHECK(sub_object_name_entry->value.as_string == "sub_object");
		}
	}

	SUBCASE("clone")
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
		CHECK(value_string == json);
	}
}

TEST_CASE("Base64")
{
	SUBCASE("Encode")
	{
		String result = base64_encode("Hello");
		DEFER(string_deinit(result));
		CHECK(result == "SGVsbG8=");
	}

	SUBCASE("Decode")
	{
		String result = base64_decode("SGVsbG8=");
		DEFER(string_deinit(result));
		CHECK(result == "Hello");
	}
}