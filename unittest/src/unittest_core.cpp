#include <core/json.h>
#include <core/logger.h>
#include <core/result.h>
#include <core/formatter.h>
#include <core/formatter.cpp>
#include <core/memory/memory.h>
#include <core/memory/pool_allocator.h>
#include <core/memory/arena_allocator.h>
#include <core/serialization/binary_serializer.h>
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

	arena_allocator_allocate(arena, 2048);

	CHECK(memory::arena_allocator_get_used_size(arena) == 2048);
	CHECK(memory::arena_allocator_get_peak_size(arena) == 2048);

	arena_allocator_clear(arena);

	CHECK(memory::arena_allocator_get_used_size(arena) == 0);
	CHECK(memory::arena_allocator_get_peak_size(arena) == 2048);
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

struct vec3
{
	f32 x, y, z;
};

inline static void
format(Formatter &self, const vec3 &value)
{
	format(self, "{{{}, {}, {}}}", value.x, value.y, value.z);
}

TEST_CASE("[CORE]: Formatter")
{
	Formatter formatter = {};
	formatter_format(formatter, "{}", vec3{1, 2, 3});
	CHECK(string_literal(formatter.buffer) == "{1, 2, 3}");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 1);
	formatter_clear(formatter);

	formatter_format(formatter, "{}/{}/{}/{}/{}/{}", "Hello", 'A', true, 1.5f, 3, vec3{4, 5, 6});
	CHECK(string_literal(formatter.buffer) == "Hello/A/true/1.5/3/{4, 5, 6}");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 6);
	CHECK(formatter.ctx->replacements_per_depth[0].fields[0].index == 0);
	CHECK(formatter.ctx->replacements_per_depth[0].fields[1].index == 1);
	CHECK(formatter.ctx->replacements_per_depth[0].fields[2].index == 2);
	CHECK(formatter.ctx->replacements_per_depth[0].fields[3].index == 3);
	CHECK(formatter.ctx->replacements_per_depth[0].fields[4].index == 4);
	CHECK(formatter.ctx->replacements_per_depth[0].fields[5].index == 5);
	formatter_clear(formatter);

	formatter_format(formatter, "{0}{2}{1}", "Hello, ", "!", "World");
	CHECK(string_literal(formatter.buffer) == "Hello, World!");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 3);
	formatter_clear(formatter);

	formatter_format(formatter, "{0}{1}{2}", "Hello, ", "World", "!");
	CHECK(string_literal(formatter.buffer) == "Hello, World!");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 3);
	formatter_clear(formatter);

	// TODO: Should this assert?
	formatter_format(formatter, "{0}{1}{2}", "Hello, ", "World");
	CHECK(string_literal(formatter.buffer) == "Hello, World");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 3);
	formatter_clear(formatter);

	const char *positional_arg_fmt = "{0}{2}{1}";
	formatter_format(formatter, positional_arg_fmt, "Hello, ", "!", "World");
	CHECK(string_literal(formatter.buffer) == "Hello, World!");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 3);
	formatter_clear(formatter);

	formatter_format(formatter, "{0}{2}{1}", 0, 2, 1);
	CHECK(string_literal(formatter.buffer) == "012");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 3);
	formatter_clear(formatter);

	formatter_format(formatter, "{0}{1}{3}{2}", "Hello, ", 0, 2, 1);
	CHECK(string_literal(formatter.buffer) == "Hello, 012");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 4);
	formatter_clear(formatter);

	formatter_format(formatter, "{}", "{ \"name\": \"n\" }");
	CHECK(string_literal(formatter.buffer) == "{ \"name\": \"n\" }");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 1);
	formatter_clear(formatter);

	formatter_format(formatter, "{{ \"name\": \"n\" }}");
	CHECK(string_literal(formatter.buffer) == "{ \"name\": \"n\" }");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 0);
	formatter_clear(formatter);

	i32 x = 1;
	formatter_format(formatter, "{}", &x);
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 1);
	formatter_clear(formatter);

	formatter_format(formatter, "{}", "Hello");
	CHECK(string_literal(formatter.buffer) == "Hello");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 1);
	formatter_clear(formatter);

	formatter_format(formatter, "{}", 'A');
	CHECK(string_literal(formatter.buffer) == "A");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 1);
	formatter_clear(formatter);

	formatter_format(formatter, "{}", true);
	CHECK(string_literal(formatter.buffer) == "true");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 1);
	formatter_clear(formatter);

	formatter_format(formatter, "{}", 1.5f);
	CHECK(string_literal(formatter.buffer) == "1.5");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 1);
	formatter_clear(formatter);

	formatter_format(formatter, "{}", -1.5f);
	CHECK(string_literal(formatter.buffer) == "-1.5");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 1);
	formatter_clear(formatter);

	formatter_format(formatter, "{}", 3);
	CHECK(string_literal(formatter.buffer) == "3");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 1);
	formatter_clear(formatter);

	formatter_format(formatter, "{}", -3);
	CHECK(string_literal(formatter.buffer) == "-3");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 1);
	formatter_clear(formatter);

	formatter_format(formatter, "{}", vec3{1, 2, 3});
	CHECK(string_literal(formatter.buffer) == "{1, 2, 3}");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 1);
	formatter_clear(formatter);

	char test[] = "test";
	formatter_format(formatter, "{}", test);
	CHECK(string_literal(formatter.buffer) == "test");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 1);
	formatter_clear(formatter);

	vec3 array[2] = {{1, 2, 3}, {4, 5, 6}};
	formatter_format(formatter, "{}", array);
	CHECK(string_literal(formatter.buffer) == "[2] { {1, 2, 3}, {4, 5, 6} }");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 1);
	formatter_clear(formatter);

	const char *array_of_strings[2] = {"Hello", "World"};
	formatter_format(formatter, "{}", array_of_strings);
	CHECK(string_literal(formatter.buffer) == "[2] { Hello, World }");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 1);
	formatter_clear(formatter);

	formatter_format(formatter, "{}", array_from<i32>({1, 2, 3}, memory::temp_allocator()));
	CHECK(string_literal(formatter.buffer) == "[3] { 1, 2, 3 }");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 1);
	formatter_clear(formatter);

	formatter_format(formatter, "{}", hash_table_from<i32, const char *>({{1, "1"}, {2, "2"}, {3, "3"}}, memory::temp_allocator()));
	CHECK(string_literal(formatter.buffer) == "[3] { 1: 1, 2: 2, 3: 3 }");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 1);
	formatter_clear(formatter);

	formatter_format(formatter, "{}{}{}{}{}", 1, 2, 3);
	CHECK(string_literal(formatter.buffer) == "123");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 5);
	formatter_clear(formatter);

	formatter_format(formatter, "{}{}{}{}{}", 1, 2, 3, "{}", 4);
	CHECK(string_literal(formatter.buffer) == "123{}4");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 5);
	formatter_clear(formatter);

	formatter_format(formatter, "A", "B");
	CHECK(string_literal(formatter.buffer) == "A");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 0);
	formatter_clear(formatter);

	formatter_format(formatter, "{}A", "B");
	CHECK(string_literal(formatter.buffer) == "BA");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 1);
	formatter_clear(formatter);

	formatter_format(formatter, "{}", "A", "B", "C");
	CHECK(string_literal(formatter.buffer) == "A");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 1);
	formatter_clear(formatter);

	const char *fmt_string = "{}/{}";
	formatter_format(formatter, fmt_string, "A", "B");
	CHECK(string_literal(formatter.buffer) == "A/B");
	CHECK(formatter.ctx->replacements_per_depth[0].field_count == 2);
	formatter_clear(formatter);
}

TEST_CASE("[CORE]: JSON")
{
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
			LOG_ERROR("{}", error.message.data);

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
			LOG_ERROR("{}", error.message.data);

		auto value_copy        = clone(value, memory::temp_allocator());
		auto [value_string, _] = json_value_to_string(value_copy, memory::temp_allocator());
		CHECK(value_string == json);
	}
}

struct Game
{
	u64 a;
	u64 b;
	f32 c;
	char d;
	Array<f32> e;
	String f;
	Hash_Table<String, f32> g;
};

inline static Game
game_init(memory::Allocator *allocator = memory::heap_allocator())
{
	Game self = {};
	self.e = array_init<f32>(allocator);
	self.f = string_init(allocator);
	self.g = hash_table_init<String, f32>(allocator);
	return self;
}

inline static void
game_deinit(Game &self)
{
	array_deinit(self.e);
	string_deinit(self.f);
	destroy(self.g);
	self = {};
}

inline static void
serialize(Serializer *self, const Game &data)
{
	serialize(self, data.a);
	serialize(self, data.b);
	serialize(self, data.c);
	serialize(self, data.d);
	serialize(self, data.e);
	serialize(self, data.f);
	serialize(self, data.g);
}

inline static void
deserialize(Serializer *self, Game &data)
{
	deserialize(self, data.a);
	deserialize(self, data.b);
	deserialize(self, data.c);
	deserialize(self, data.d);
	deserialize(self, data.e);
	deserialize(self, data.f);
	deserialize(self, data.g);
}

TEST_CASE("[CORE]: Binary_Serializer")
{
	Binary_Serializer *serializer = binary_serializer_init();
	DEFER(binary_serializer_deinit(serializer));

	Game original_game = game_init();
	DEFER(game_deinit(original_game));

	{
		// Set some arbitrary values.
		{
			original_game.a = 31;
			original_game.b = 37;
			original_game.c = 1.5f;
			original_game.d = 'A';
			array_push(original_game.e, 0.5f);
			array_push(original_game.e, 1.5f);
			array_push(original_game.e, 2.5f);
			string_append(original_game.f, "Hello1");
			hash_table_insert(original_game.g, string_literal("1"), 1.0f);
			hash_table_insert(original_game.g, string_literal("2"), 2.0f);
			hash_table_insert(original_game.g, string_literal("3"), 3.0f);
		}

		binary_serializer_serialize(serializer, original_game);

		{
			Game new_game = game_init();
			DEFER(game_deinit(new_game));
			binary_serializer_deserialize(serializer, new_game);

			CHECK(new_game.a == original_game.a);
			CHECK(new_game.b == original_game.b);
			CHECK(new_game.c == original_game.c);
			CHECK(new_game.d == original_game.d);

			for (u64 i = 0; i < new_game.e.count; ++i)
				CHECK(new_game.e[i] == original_game.e[i]);

			CHECK(new_game.f == original_game.f);

			for (const auto &[new_key, new_value] : new_game.g)
			{
				const auto &[original_key, original_value] = *hash_table_find(original_game.g, new_key);
				CHECK(new_key == original_key);
				CHECK(new_value == original_value);
			}
		}
	}

	{
		auto write_error = binary_serializer_to_file(serializer, "serialize_test.core");
		binary_serializer_clear(serializer);

		CHECK(write_error == false);
	}

	{
		auto [deserializer, read_error] = binary_serializer_from_file("serialize_test.core");
		DEFER(binary_serializer_deinit(deserializer));

		CHECK(read_error == false);

		Game new_game = game_init();
		DEFER(game_deinit(new_game));
		binary_serializer_deserialize(deserializer, new_game);

		CHECK(new_game.a == original_game.a);
		CHECK(new_game.b == original_game.b);
		CHECK(new_game.c == original_game.c);
		CHECK(new_game.d == original_game.d);

		for (u64 i = 0; i < new_game.e.count; ++i)
			CHECK(new_game.e[i] == original_game.e[i]);

		CHECK(new_game.f == original_game.f);

		for (const auto &[new_key, new_value] : new_game.g)
		{
			const auto &[original_key, original_value] = *hash_table_find(original_game.g, new_key);
			CHECK(new_key == original_key);
			CHECK(new_value == original_value);
		}
	}

	CHECK(platform_file_delete("serialize_test.core"));
}