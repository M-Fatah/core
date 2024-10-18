// TODO:
#include "../../test/src/bin_serializer.h"
#include "../../test/src/jsn_serializer.h"

#include <doctest/doctest.h>

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

template <typename T>
inline static void
serialize(T &self, Game &data)
{
	serialize(self, {
		{"a", data.a},
		{"b", data.b},
		{"c", data.c},
		{"d", data.d},
		{"e", data.e},
		{"f", data.f},
		{"g", data.g}
	});
}

template <typename T>
inline static void
deserialize(T &self, Game &data)
{
	deserialize(self, {
		{"a", data.a},
		{"b", data.b},
		{"c", data.c},
		{"d", data.d},
		{"e", data.e},
		{"f", data.f},
		{"g", data.g}
	});
}

TEST_CASE("[CORE]: Binary_Serializer")
{
	Bin_Serializer serializer = bin_serializer_init();
	DEFER(bin_serializer_deinit(serializer));

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

		serialize(serializer, original_game);

		{
			Game new_game = {};
			DEFER(game_deinit(new_game));
			deserialize(serializer, new_game);

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
		// auto write_error = binary_serializer_to_file(serializer, "serialize_test.core");
		// binary_serializer_clear(serializer);

		// CHECK(write_error == false);
	}

	{
		// auto [deserializer, read_error] = binary_serializer_from_file("serialize_test.core");
		// DEFER(binary_serializer_deinit(deserializer));

		// CHECK(read_error == false);

		// Game new_game = game_init();
		// DEFER(game_deinit(new_game));
		// binary_serializer_deserialize(deserializer, new_game);

		// CHECK(new_game.a == original_game.a);
		// CHECK(new_game.b == original_game.b);
		// CHECK(new_game.c == original_game.c);
		// CHECK(new_game.d == original_game.d);

		// for (u64 i = 0; i < new_game.e.count; ++i)
		// 	CHECK(new_game.e[i] == original_game.e[i]);

		// CHECK(new_game.f == original_game.f);

		// for (const auto &[new_key, new_value] : new_game.g)
		// {
		// 	const auto &[original_key, original_value] = *hash_table_find(original_game.g, new_key);
		// 	CHECK(new_key == original_key);
		// 	CHECK(new_value == original_value);
		// }
	}

	// CHECK(platform_file_delete("serialize_test.core"));
}

TEST_CASE("[CORE]: JSON_Serializer")
{
	Jsn_Serializer serializer = jsn_serializer_init();
	DEFER(jsn_serializer_deinit(serializer));

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

		serialize(serializer, {"original_game", original_game});

		{
			const char *expected_json_string = R"""({"original_game":{"a":31,"b":37,"c":1.500000,"d":65,"e":[0.500000,1.500000,2.500000],"f":"Hello1","g":[{"key":"1","value":1.000000},{"key":"2","value":2.000000},{"key":"3","value":3.000000}]}})""";
			CHECK(serializer.buffer == expected_json_string);
		}

		{
			Game new_game = {};
			DEFER(game_deinit(new_game));
			deserialize(serializer, {"original_game", new_game});

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
}