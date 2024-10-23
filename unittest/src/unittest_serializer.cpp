#include <core/serialization/binary_serializer.h>
#include <core/serialization/json_serializer.h>

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
	i32 *h;
};

inline static Game
game_init(memory::Allocator *allocator = memory::heap_allocator())
{
	Game self = {};
	self.e = array_init<f32>(allocator);
	self.f = string_init(allocator);
	self.g = hash_table_init<String, f32>(allocator);
	self.h = memory::allocate<i32>(allocator);
	return self;
}

inline static void
game_deinit(Game &self)
{
	array_deinit(self.e);
	string_deinit(self.f);
	destroy(self.g);
	memory::deallocate(self.h);
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
		{"g", data.g},
		{"h", data.h}
	});
}

TEST_CASE("[CORE]: Binary_Serializer")
{
	Bin_Serializer serializer = bin_serializer_init();
	DEFER(bin_serializer_deinit(serializer));

	SUBCASE("Fundamental types")
	{
		i8   a1 = 1;
		i16  b1 = 2;
		i32  c1 = 3;
		i64  d1 = 4;
		u8   e1 = 5;
		u16  f1 = 6;
		u32  g1 = 7;
		u64  h1 = 8;
		f32  i1 = 9;
		f64  j1 = 10;
		char k1 = 'A';
		bool l1 = true;

		serialize(serializer, {"a1", a1});
		serialize(serializer, {"b1", b1});
		serialize(serializer, {"c1", c1});
		serialize(serializer, {"d1", d1});
		serialize(serializer, {"e1", e1});
		serialize(serializer, {"f1", f1});
		serialize(serializer, {"g1", g1});
		serialize(serializer, {"h1", h1});
		serialize(serializer, {"i1", i1});
		serialize(serializer, {"j1", j1});
		serialize(serializer, {"k1", k1});
		serialize(serializer, {"l1", l1});

	Bin_Deserializer deserializer = bin_deserializer_init(serializer.buffer);
	DEFER(bin_deserializer_deinit(deserializer));

		i8   a2 = 0;
		i16  b2 = 0;
		i32  c2 = 0;
		i64  d2 = 0;
		u8   e2 = 0;
		u16  f2 = 0;
		u32  g2 = 0;
		u64  h2 = 0;
		f32  i2 = 0;
		f64  j2 = 0;
		char k2 = 0;
		bool l2 = 0;

		serialize(deserializer, {"a1", a2});
		serialize(deserializer, {"b1", b2});
		serialize(deserializer, {"c1", c2});
		serialize(deserializer, {"d1", d2});
		serialize(deserializer, {"e1", e2});
		serialize(deserializer, {"f1", f2});
		serialize(deserializer, {"g1", g2});
		serialize(deserializer, {"h1", h2});
		serialize(deserializer, {"i1", i2});
		serialize(deserializer, {"j1", j2});
		serialize(deserializer, {"k1", k2});
		serialize(deserializer, {"l1", l2});

		CHECK(a1 == a2);
		CHECK(b1 == b2);
		CHECK(c1 == c2);
		CHECK(d1 == d2);
		CHECK(e1 == e2);
		CHECK(f1 == f2);
		CHECK(g1 == g2);
		CHECK(h1 == h2);
		CHECK(i1 == i2);
		CHECK(j1 == j2);
		CHECK(k1 == k2);
		CHECK(l1 == l2);
	}

	SUBCASE("Pointers")
	{
		i32 i1 = 5;
		i32 *a1 = &i1;

		serialize(serializer, {"a1", a1});


	Bin_Deserializer deserializer = bin_deserializer_init(serializer.buffer);
	DEFER(bin_deserializer_deinit(deserializer));

		i32 i2 = 0;
		i32 *a2 = &i2;

		serialize(deserializer, {"a1", a2});

		CHECK(*a1 == *a2);
	}

	SUBCASE("Arrays")
	{

		i32 a1[3]    = {1, 2, 3};
		Array<i8> b1 = array_from<i8>({1, 2, 3, 4, 5});
		DEFER(array_deinit(b1));

		serialize(serializer, {"a1", a1});
		serialize(serializer, {"b1", b1});

		i32 a2[3]    = {};
		Array<i8> b2 = {};
		DEFER(array_deinit(b2));

	Bin_Deserializer deserializer = bin_deserializer_init(serializer.buffer);
	DEFER(bin_deserializer_deinit(deserializer));

		serialize(deserializer, {"a1", a2});
		serialize(deserializer, {"b1", b2});

		for (u64 i = 0; i < count_of(a1); ++i)
		{
			CHECK(a1[i] == a2[i]);
			CHECK(b1[i] == b2[i]);
		}
	}

	SUBCASE("Strings")
	{
		const char *a1 = "Hello, World!";
		String b1 = string_from("Hello, World!");
		DEFER(string_deinit(b1));

		serialize(serializer, {"a1", a1});
		serialize(serializer, {"b1", b1});

		const char *a2 = {};
		String b2 = {};
		DEFER({
			memory::deallocate((void *)a2);
			string_deinit(b2);
		});

	Bin_Deserializer deserializer = bin_deserializer_init(serializer.buffer);
	DEFER(bin_deserializer_deinit(deserializer));

		serialize(deserializer, {"a1", a2});
		serialize(deserializer, {"b1", b2});

		CHECK(string_literal(a1) == a2);
		CHECK(b1 == b2);
	}

	SUBCASE("Hash tables")
	{
		Hash_Table<i32, String> a1 = hash_table_from<i32, String>({
			{1, string_literal("A")},
			{2, string_literal("B")},
			{3, string_literal("C")},
		});
		DEFER(hash_table_deinit(a1));

		serialize(serializer, {"a1", a1});

		Hash_Table<i32, String> a2 = {};
		DEFER(destroy(a2));

	Bin_Deserializer deserializer = bin_deserializer_init(serializer.buffer);
	DEFER(bin_deserializer_deinit(deserializer));

		serialize(deserializer, {"a1", a2});

		CHECK(a1.count    == a2.count);
		CHECK(a1.capacity == a2.capacity);

		for (u64 i = 0; i < a1.entries.count; ++i)
		{
			CHECK(a1.entries[i].key   == a2.entries[i].key);
			CHECK(a1.entries[i].value == a2.entries[i].value);
		}
	}

	SUBCASE("Blocks")
	{
		i32 i = 5;

		Block a1 = {&i, sizeof(i)};

		serialize(serializer, {"a1", a1});

		Block a2 = {};
		DEFER(memory::deallocate(a2.data));

	Bin_Deserializer deserializer = bin_deserializer_init(serializer.buffer);
	DEFER(bin_deserializer_deinit(deserializer));

		serialize(deserializer, {"a1", a2});

		CHECK(*((i32 *)a1.data) == *((i32 *)a2.data));
		CHECK(a1.size == a2.size);
	}

	SUBCASE("Structs")
	{
		Game original_game = game_init();
		DEFER(game_deinit(original_game));

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
		*original_game.h = 5;

		serialize(serializer, {"original_game", original_game});

		Game new_game = {};
		DEFER(game_deinit(new_game));

	Bin_Deserializer deserializer = bin_deserializer_init(serializer.buffer);
	DEFER(bin_deserializer_deinit(deserializer));
		serialize(deserializer, {"original_game", new_game});

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

		CHECK(*original_game.h == *new_game.h);
	}
}

TEST_CASE("[CORE]: JSON_Serializer")
{
	Jsn_Serializer serializer = jsn_serializer_init();
	DEFER(jsn_serializer_deinit(serializer));

	SUBCASE("Fundamental types")
	{
		i8   a1 = 1;
		i16  b1 = 2;
		i32  c1 = 3;
		i64  d1 = 4;
		u8   e1 = 5;
		u16  f1 = 6;
		u32  g1 = 7;
		u64  h1 = 8;
		f32  i1 = 9;
		f64  j1 = 10;
		char k1 = 'A';
		bool l1 = true;

		serialize(serializer, {"a1", a1});
		serialize(serializer, {"b1", b1});
		serialize(serializer, {"c1", c1});
		serialize(serializer, {"d1", d1});
		serialize(serializer, {"e1", e1});
		serialize(serializer, {"f1", f1});
		serialize(serializer, {"g1", g1});
		serialize(serializer, {"h1", h1});
		serialize(serializer, {"i1", i1});
		serialize(serializer, {"j1", j1});
		serialize(serializer, {"k1", k1});
		serialize(serializer, {"l1", l1});

		i8   a2 = 0;
		i16  b2 = 0;
		i32  c2 = 0;
		i64  d2 = 0;
		u8   e2 = 0;
		u16  f2 = 0;
		u32  g2 = 0;
		u64  h2 = 0;
		f32  i2 = 0;
		f64  j2 = 0;
		char k2 = 0;
		bool l2 = 0;

		Jsn_Deserializer deserializer = jsn_deserializer_init(serializer.buffer);
		DEFER(jsn_deserializer_deinit(deserializer));

		serialize(deserializer, {"a1", a2});
		serialize(deserializer, {"b1", b2});
		serialize(deserializer, {"c1", c2});
		serialize(deserializer, {"d1", d2});
		serialize(deserializer, {"e1", e2});
		serialize(deserializer, {"f1", f2});
		serialize(deserializer, {"g1", g2});
		serialize(deserializer, {"h1", h2});
		serialize(deserializer, {"i1", i2});
		serialize(deserializer, {"j1", j2});
		serialize(deserializer, {"k1", k2});
		serialize(deserializer, {"l1", l2});

		CHECK(a1 == a2);
		CHECK(b1 == b2);
		CHECK(c1 == c2);
		CHECK(d1 == d2);
		CHECK(e1 == e2);
		CHECK(f1 == f2);
		CHECK(g1 == g2);
		CHECK(h1 == h2);
		CHECK(i1 == i2);
		CHECK(j1 == j2);
		CHECK(k1 == k2);
		CHECK(l1 == l2);
	}

	SUBCASE("Pointers")
	{
		i32 i1 = 5;
		i32 *a1 = &i1;

		serialize(serializer, {"a1", a1});

		Jsn_Deserializer deserializer = jsn_deserializer_init(serializer.buffer);
		DEFER(jsn_deserializer_deinit(deserializer));

		i32 i2 = 0;
		i32 *a2 = &i2;

		serialize(deserializer, {"a1", a2});

		CHECK(*a1 == *a2);
	}

	SUBCASE("Arrays")
	{

		i32 a1[3]    = {1, 2, 3};
		Array<i8> b1 = array_from<i8>({1, 2, 3});
		DEFER(array_deinit(b1));

		serialize(serializer, {"a1", a1});
		serialize(serializer, {"b1", b1});

		Jsn_Deserializer deserializer = jsn_deserializer_init(serializer.buffer);
		DEFER(jsn_deserializer_deinit(deserializer));

		i32 a2[3]    = {};
		Array<i8> b2 = {};
		DEFER(array_deinit(b2));

		serialize(deserializer, {"a1", a2});
		serialize(deserializer, {"b1", b2});

		for (u64 i = 0; i < count_of(a1); ++i)
		{
			CHECK(a1[i] == a2[i]);
			CHECK(b1[i] == b2[i]);
		}
	}

	SUBCASE("Strings")
	{
		const char *a1 = "Hello, World!";
		String b1 = string_from("Hello, World!");
		DEFER(string_deinit(b1));

		serialize(serializer, {"a1", a1});
		serialize(serializer, {"b1", b1});

		Jsn_Deserializer deserializer = jsn_deserializer_init(serializer.buffer);
		DEFER(jsn_deserializer_deinit(deserializer));

		const char *a2 = {};
		String b2 = {};
		DEFER({
			memory::deallocate((void *)a2);
			string_deinit(b2);
		});

		serialize(deserializer, {"a1", a2});
		serialize(deserializer, {"b1", b2});

		CHECK(string_literal(a1) == a2);
		CHECK(b1 == b2);
	}

	SUBCASE("Hash tables")
	{
		Hash_Table<i32, String> a1 = hash_table_from<i32, String>({
			{1, string_literal("A")},
			{2, string_literal("B")},
			{3, string_literal("C")},
		});
		DEFER(hash_table_deinit(a1));

		serialize(serializer, {"a1", a1});

		Jsn_Deserializer deserializer = jsn_deserializer_init(serializer.buffer);
		DEFER(jsn_deserializer_deinit(deserializer));

		Hash_Table<i32, String> a2 = {};
		DEFER(destroy(a2));

		serialize(deserializer, {"a1", a2});

		CHECK(a1.count    == a2.count);
		CHECK(a1.capacity == a2.capacity);

		for (u64 i = 0; i < a1.entries.count; ++i)
		{
			CHECK(a1.entries[i].key   == a2.entries[i].key);
			CHECK(a1.entries[i].value == a2.entries[i].value);
		}
	}

	SUBCASE("Blocks")
	{
		i32 i = 5;

		Block a1 = {&i, sizeof(i)};

		serialize(serializer, {"a1", a1});

		Jsn_Deserializer deserializer = jsn_deserializer_init(serializer.buffer);
		DEFER(jsn_deserializer_deinit(deserializer));

		Block a2 = {};
		DEFER(memory::deallocate(a2.data));

		serialize(deserializer, {"a1", a2});

		CHECK(*((i32 *)a1.data) == *((i32 *)a2.data));
		CHECK(a1.size == a2.size);
	}

	SUBCASE("Structs")
	{
		Game original_game = game_init();
		DEFER(game_deinit(original_game));

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
		*original_game.h = 5;

		serialize(serializer, {"original_game", original_game});

		Jsn_Deserializer deserializer = jsn_deserializer_init(serializer.buffer);
		DEFER(jsn_deserializer_deinit(deserializer));

		Game new_game = {};
		DEFER(game_deinit(new_game));

		serialize(deserializer, {"original_game", new_game});

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

		CHECK(*original_game.h == *new_game.h);

		const char *expected_json_string = R"""({"original_game":{"a":31,"b":37,"c":1.500000,"d":65,"e":[0.500000,1.500000,2.500000],"f":"Hello1","g":[{"key":"1","value":1.000000},{"key":"2","value":2.000000},{"key":"3","value":3.000000}],"h":5}})""";
		CHECK(serializer.buffer == expected_json_string);
	}
}