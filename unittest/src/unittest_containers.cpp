#include <core/defer.h>
#include <core/containers/array.h>
#include <core/containers/hash_set.h>
#include <core/containers/hash_table.h>
#include <core/containers/stack_array.h>
#include <core/containers/string.h>

#include <doctest/doctest.h>

TEST_CASE("[CONTAINERS]: Array")
{
	SUBCASE("init")
	{
		{
			auto array = array_init<i32>();
			DEFER(array_deinit(array));

			CHECK(array.data == nullptr);
			CHECK(array.count == 0);
			CHECK(array.capacity == 0);
			CHECK(array.allocator != nullptr);
		}

		{
			auto array = array_with_capacity<i32>(100);
			DEFER(array_deinit(array));

			CHECK(array.data != nullptr);
			CHECK(array.count == 0);
			CHECK(array.capacity == 100);
			CHECK(array.allocator != nullptr);
		}

		{
			auto array = array_with_count<i32>(100);
			DEFER(array_deinit(array));

			CHECK(array.data != nullptr);
			CHECK(array.count == 100);
			CHECK(array.capacity == 100);
			CHECK(array.allocator != nullptr);
		}

		{
			auto array = array_from({1, 2, 3, 4, 5});
			DEFER(array_deinit(array));

			CHECK(array.data != nullptr);
			CHECK(array.count == 5);
			CHECK(array.capacity == 5);
			CHECK(array.allocator != nullptr);
			CHECK(array[0] == 1);
			CHECK(array[1] == 2);
			CHECK(array[2] == 3);
			CHECK(array[3] == 4);
			CHECK(array[4] == 5);
		}
	}

	SUBCASE("copy")
	{
		auto array1 = array_from({1, 2, 3});
		DEFER(array_deinit(array1));

		auto array2 = array_copy(array1);
		DEFER(array_deinit(array2));

		CHECK(array2.count == array1.count);
		CHECK(array2.capacity == array1.capacity);
		for (size_t i = 0; i < array2.count; ++i)
			CHECK(array2[i] == array1[i]);
	}

	SUBCASE("fill")
	{
		auto array = array_with_count<f32>(50);
		DEFER(array_deinit(array));

		array_fill(array, 5.0f);
		for (size_t i = 0; i < array.count; ++i)
			CHECK(array[i] == 5.0f);
	}

	SUBCASE("reserve")
	{
		auto array = array_with_capacity<i32>(10);
		DEFER(array_deinit(array));

		CHECK(array.data != nullptr);
		CHECK(array.capacity == 10);
		array_reserve(array, 20);
		CHECK(array.data != nullptr);
		CHECK(array.capacity == 30);
	}

	SUBCASE("push/pop/last")
	{
		auto array = array_init<i32>();
		DEFER(array_deinit(array));

		for (size_t i = 0; i < 100; ++i)
			array_push(array, i32(i));
		CHECK(array.count == 100);

		for (size_t i = 0; i < array.count; ++i)
			CHECK(array[i] == i);

		CHECK(array_last(array) == 99);

		for (i32 i = 0; i < 100; ++i)
			CHECK(array_pop(array) == 99 - i);

		CHECK(array.count == 0);
	}

	SUBCASE("append")
	{
		auto array1 = array_from({0, 1, 2, 3, 4});
		DEFER(array_deinit(array1));

		auto array2 = array_from({5, 6, 7, 8, 9});
		DEFER(array_deinit(array2));

		array_append(array1, array2);
		CHECK(array1.count == 10);

		for (i32 i = 0; i < (i32)array1.count; ++i)
			CHECK(array1[i] == i);
	}

	SUBCASE("iterators")
	{
		auto array = array_from<i32>({0, 1, 2, 3, 4});
		DEFER(array_deinit(array));

		CHECK(begin(array) == array.data);
		CHECK(end(array) == array.data + array.count);

		i32 i = 0;
		for (auto v : array)
			CHECK(v == i++);

		i = 0;
		for (const auto v: array)
			CHECK(v == i++);

		for (auto &v : array)
			v++;

		i = 1;
		for (const auto &v: array)
			CHECK(v == i++);
	}

	SUBCASE("clone")
	{
		auto array1 = array_init<Array<i32>>();
		DEFER(destroy(array1));
		array_push(array1, array_from<i32>({1, 2, 3}));

		auto array2 = clone(array1);
		DEFER(destroy(array2));

		for (size_t i = 0; i < array2.count; ++i)
		{
			CHECK(array2.count == array1.count);
			for (size_t j = 0; j < array2[i].count; ++j)
			{
				auto v1 = array1[i][j];
				auto v2 = array2[i][j];
				CHECK(v1 == v2);
			}
		}
	}

	SUBCASE("destroy")
	{
		auto v = array_init<Array<i32>>();
		DEFER(destroy(v));

		array_push(v, array_from<i32>({1, 2, 3}));
		array_push(v, array_from<i32>({4, 5, 6}));
	}
}

TEST_CASE("[CONTAINERS]: Stack_Array")
{
	SUBCASE("init")
	{
		{
			Stack_Array<i32, 4> array{};
			CHECK(array.count == 0);
		}

		{
			Stack_Array array{{1, 2, 3}};
			for (u64 i = 0; i < array.count; ++i)
				CHECK(array[i] == i + 1);
			CHECK(array.count == 3);
		}

		{
			Stack_Array<i32, 3> array{{1, 2, 3}};
			for (u64 i = 0; i < array.count; ++i)
				CHECK(array[i] == i + 1);
			CHECK(array.count == 3);
		}
	}
}

TEST_CASE("[CONTAINERS]: String")
{
	SUBCASE("init")
	{
		auto s = string_init();
		CHECK(s.count == 0);
		CHECK(s.capacity == 1);
		CHECK(*s.data == '\0');
		string_deinit(s);

		auto c_string = "Hello, World!";
		s = string_from(c_string);
		CHECK(s.count == 13);
		CHECK(s.capacity == 14);
		for (u64 i = 0; i < s.count; ++i)
			CHECK(s[i] == c_string[i]);
		CHECK(s.data[s.count] == '\0');
		string_deinit(s);

		s = string_literal(c_string);
		CHECK(s.allocator == nullptr);
		CHECK(s.count == 13);
		CHECK(s.capacity == 14);
		for (u64 i = 0; i < s.count; ++i)
			CHECK(s[i] == c_string[i]);
		CHECK(s.data[s.count] == '\0');
		string_deinit(s);

		auto c_string_empty = "";
		s = string_from(c_string_empty);
		CHECK(s.count == 0);
		CHECK(s.capacity == 1);
		CHECK(s.data[s.count] == '\0');
		string_deinit(s);

		s = string_literal(c_string_empty);
		CHECK(s.allocator == nullptr);
		CHECK(s.count == 0);
		CHECK(s.capacity == 1);
		CHECK(s.data[s.count] == '\0');
		string_deinit(s);

		auto literal = "Hello, World!";
		s = string_from(literal, literal + 13);
		CHECK(s.count == 13);
		CHECK(s.capacity == 14);
		for (size_t i = 0; i < 13; ++i)
			CHECK(s[i] == literal[i]);
		string_deinit(s);
	}

	SUBCASE("copy")
	{
		auto s1 = string_from("Copy");
		DEFER(string_deinit(s1));

		auto s2 = string_copy(s1);
		DEFER(string_deinit(s2));

		CHECK(s2.count == s1.count);
		CHECK(s2.capacity == s1.capacity);
		for (size_t i = 0; i < s2.count; ++i)
			CHECK(s2[i] == s1[i]);
	}

	SUBCASE("append")
	{
		auto s1 = "Hello, ";
		auto s2 = "World";
		auto s3 = string_from("!");

		auto s = string_init();
		string_append(s, s1);
		string_append(s, s2);
		string_append(s, s3);

		CHECK(s.count == 13);
		CHECK(s.capacity == 14);

		auto expected = "Hello, World!";
		for (size_t i = 0; i < s.count; ++i)
			CHECK(s[i] == expected[i]);
		CHECK(s.data[s.count] == '\0');

		string_deinit(s);
		string_deinit(s3);
	}

	SUBCASE("to lower/ to upper")
	{
		auto s = string_from("HELLO");
		DEFER(string_deinit(s));

		CHECK(string_to_lowercase(s) == "hello");
		CHECK(string_to_uppercase(s) == "HELLO");
	}

	SUBCASE("find/contains")
	{
		auto s = string_from("Hello World!");
		DEFER(string_deinit(s));

		CHECK(string_find_first_of(s, string_literal("Hello")) == 0);
		CHECK(string_find_first_of(s, string_literal("World!")) == 6);
		CHECK(string_find_first_of(s, string_literal("olleH")) == -1);

		CHECK(string_find_first_of(s, 'H', 0) == 0);
		CHECK(string_find_first_of(s, 'W', 0) == 6);

		CHECK(string_find_last_of(string_literal("Hello, World. World"), string_literal("Hello")) == 0);
		CHECK(string_find_last_of(string_literal("Hello, World. World Hello"), string_literal("World")) == 14);
		CHECK(string_find_last_of(string_literal("Hello, World. World Hello"), string_literal("o")) == 24);
		CHECK(string_find_last_of(s, 'l') == 9);
		CHECK(string_find_last_of(s, '!') == 11);

		CHECK(string_contains(s, string_literal("Hello")));
		CHECK(string_contains(s, string_literal("World!")));
		CHECK(string_contains(s, string_literal("hello"), true));
		CHECK(string_contains(s, string_literal("world!"), true));
	}

	SUBCASE("trim")
	{
		auto s1 = string_from(" Hello, World! ");
		DEFER(string_deinit(s1));

		string_trim(s1, string_literal(" "));

		auto expected = "Hello, World!";
		CHECK(s1.count == 13);
		CHECK(s1.capacity == 14);
		for (size_t i = 0; i < s1.count; ++i)
			CHECK(s1[i] == expected[i]);
		CHECK(s1.data[s1.count] == '\0');

		// ====================== //
		string_trim_left(s1, string_literal("H"));
		CHECK(s1.count == 12);
		CHECK(s1.capacity == 13);
		for (size_t i = 1; i < s1.count; ++i)
			CHECK(s1[i - 1] == expected[i]);
		CHECK(s1.data[s1.count] == '\0');

		// ====================== //
		string_trim_right(s1, string_literal("!"));
		CHECK(s1.count == 11);
		CHECK(s1.capacity == 12);
		for (size_t i = 1; i < s1.count; ++i)
			CHECK(s1[i - 1] == expected[i]);
		CHECK(s1.data[s1.count] == '\0');

		// ====================== //
		auto s2 = string_from(" Hello, World! ");
		DEFER(string_deinit(s2));

		string_trim_whitespace(s2);

		CHECK(s2.count == 13);
		CHECK(s2.capacity == 14);
		for (size_t i = 0; i < s2.count; ++i)
			CHECK(s2[i] == expected[i]);
		CHECK(s2.data[s2.count] == '\0');
	}

	SUBCASE("split")
	{
		{
			auto s = string_from("Hello World!");
			DEFER(string_deinit(s));

			auto splits = string_split(s, ' ');
			DEFER(destroy(splits));

			CHECK(splits.count == 2);
			CHECK(splits[0] == "Hello");
			CHECK(splits[1] == "World!");
		}

		{
			auto s = string_from("Hello, to,, the, new, World!");
			DEFER(string_deinit(s));

			auto splits = string_split(s, ',', false);
			DEFER(destroy(splits));

			CHECK(splits.count == 6);
			CHECK(splits[0] == "Hello");
			CHECK(splits[1] == " to");
			CHECK(splits[2] == "");
			CHECK(splits[3] == " the");
			CHECK(splits[4] == " new");
			CHECK(splits[5] == " World!");
		}

		{
			auto s = string_from("Hello, to,, the, new, World!");
			DEFER(string_deinit(s));

			auto splits = string_split(s, ',');
			DEFER(destroy(splits));

			CHECK(splits.count == 5);
			CHECK(splits[0] == "Hello");
			CHECK(splits[1] == " to");
			CHECK(splits[2] == " the");
			CHECK(splits[3] == " new");
			CHECK(splits[4] == " World!");
		}

		{
			auto s = string_from("Hello,split Worldsplit!");
			DEFER(string_deinit(s));

			auto splits = string_split(s, string_literal("split"));
			DEFER(destroy(splits));

			CHECK(splits.count == 3);
			CHECK(splits[0] == "Hello,");
			CHECK(splits[1] == " World");
			CHECK(splits[2] == "!");
		}
	}

	SUBCASE("replace")
	{
		auto s = string_from("Helloxxx, xxxWorld!xxx", memory::temp_allocator());
		DEFER(string_deinit(s));

		string_replace(s, string_literal("xxx"), string_literal(""));

		auto expected = "Hello, World!";
		CHECK(s.count == 13);
		for (size_t i = 0; i < s.count; ++i)
			CHECK(s[i] == expected[i]);
		CHECK(s.data[s.count] == '\0');

		auto s2 = string_from("Helloxxx, xxxWorld!xxx", memory::temp_allocator());
		string_replace_first_occurance(s2, "xxx", "", 5);
		CHECK(s2 == "Hello, xxxWorld!xxx");
		string_replace_first_occurance(s2, "xxx", "", 10);
		CHECK(s2 == "Hello, xxxWorld!");
		string_replace_first_occurance(s2, "xxx", "");
		CHECK(s2 == "Hello, World!");
	}

	SUBCASE("starts with")
	{
		auto s = string_from("Hello, World!");
		DEFER(string_deinit(s));

		CHECK(string_starts_with(s, string_literal("Hello")) == true);
		CHECK(string_starts_with(s, string_literal("ello")) == false);

		CHECK(string_starts_with(s, "Hello") == true);
		CHECK(string_starts_with(s, "ello") == false);

		CHECK(string_starts_with("Hello, World!", s) == true);
		CHECK(string_starts_with("World, Hello!", s) == false);

		CHECK(string_starts_with("Hello", "H") == true);
		CHECK(string_starts_with("Hello", "e") == false);
	}

	SUBCASE("ends with")
	{
		auto s = string_from("Hello, World!");
		DEFER(string_deinit(s));

		CHECK(string_ends_with(s, string_literal("World!")) == true);
		CHECK(string_ends_with(s, string_literal("Hello")) == false);

		CHECK(string_ends_with(s, "World!") == true);
		CHECK(string_ends_with(s, "Hello") == false);

		CHECK(string_ends_with("Hello, World!", s) == true);
		CHECK(string_ends_with("World, Hello!", s) == false);

		CHECK(string_ends_with("Hello", "o") == true);
		CHECK(string_ends_with("Hello", "H") == false);
	}

	SUBCASE("clear")
	{
		auto s = string_from("Hello, World!");
		DEFER(string_deinit(s));

		string_clear(s);

		CHECK(s.count == 0);
		CHECK(s.data[s.count] == '\0');
	}

	SUBCASE("equal")
	{
		auto s1 = string_from("Hello");
		auto s2 = string_from("World");
		auto s3 = string_from("Hello");
		auto s4 = string_from("");

		CHECK(s1 != s2);
		CHECK(s1 == s3);
		CHECK(s1 == "Hello");
		CHECK(s1 != "World");
		CHECK("Hello" == s1);
		CHECK("World" != s1);
		CHECK(s4 == "");
		CHECK(s4 != "Hello");

		string_deinit(s1);
		string_deinit(s2);
		string_deinit(s3);
		string_deinit(s4);
	}
}

struct Foo
{
	i32 x;

	inline bool
	operator==(const Foo &other) const
	{
		return x == other.x;
	}

	inline bool
	operator!=(const Foo &other) const
	{
		return !operator==(other);
	}
};

inline static u64
hash(const Foo &value)
{
	return hash_fnv_x32(&value.x, sizeof(value.x));
}

inline static Foo
clone(const Foo &other, memory::Allocator *)
{
	return other;
}

inline static void
destroy(Foo &) {}

TEST_CASE("[CONTAINERS]: Hash_Table")
{
	SUBCASE("init")
	{
		{
			Hash_Table<const char *, const char *> table = hash_table_init<const char *, const char *>();
			DEFER(hash_table_deinit(table));

			CHECK(table.count    == 0);
			CHECK(table.capacity == 0);

			CHECK(table.slots.data     == nullptr);
			CHECK(table.slots.count    == 0);
			CHECK(table.slots.capacity == 0);
		}

		{
			auto table = hash_table_init_with_capacity<i32, const char *>(62, memory::temp_allocator());
			CHECK(table.count == 0);
			CHECK(table.capacity == 64);

			CHECK(table.slots.data     != nullptr);
			CHECK(table.slots.count    == 64);
			CHECK(table.slots.capacity == 64);

			hash_table_insert(table, 1, (const char *)"Hello");
			hash_table_insert(table, 2, (const char *)"World!");

			CHECK(table.count == 2);
			CHECK(table.capacity == 64);
		}

		{
			auto table = hash_table_init_from<i32, const char *>({ {1, "Hello"}, {2, "World!"} }, memory::temp_allocator());

			CHECK(table.count == 2);
			CHECK(table.capacity == 8);

			CHECK(table.slots.data     != nullptr);
			CHECK(table.slots.count    == 8);
			CHECK(table.slots.capacity == 8);

			CHECK(table.entries[0].key == 1);
			CHECK(::strcmp(table.entries[0].value, "Hello") == 0);

			CHECK(table.entries[1].key == 2);
			CHECK(::strcmp(table.entries[1].value, "World!") == 0);
		}

		{
			Hash_Table<i32, i32> table = {};
			DEFER(hash_table_deinit(table));

			hash_table_insert(table, 1, 1);
			hash_table_insert(table, 2, 2);

			CHECK(table.count == 2);
			CHECK(table.capacity == 8);

			CHECK(table.slots.data     != nullptr);
			CHECK(table.slots.count    == 8);
			CHECK(table.slots.capacity == 8);

			CHECK(table.entries[0].key == 1);
			CHECK(table.entries[0].value == 1);

			CHECK(table.entries[1].key == 2);
			CHECK(table.entries[1].value == 2);
		}
	}

	SUBCASE("const char *")
	{
		Hash_Table<const char *, const char *> table = hash_table_init<const char *, const char *>();
		DEFER(hash_table_deinit(table));

		CHECK(table.count    == 0);
		CHECK(table.capacity == 0);

		const char *key   = "Hello";
		const char *value = "World!";
		hash_table_insert(table, key, value);

		CHECK(table.entries.count == 1);

		CHECK(table.count == 1);
		CHECK(hash_table_find(table, key) != nullptr);
		CHECK(hash_table_find(table, value) == nullptr);

		CHECK(::strcmp(hash_table_find(table, key)->key, "Hello") == 0);
		CHECK(::strcmp(hash_table_find(table, key)->value, "World!") == 0);

		const char *dumb_value = "new world!";
		hash_table_insert(table, key, dumb_value);

		CHECK(table.entries.count == 1);

		CHECK(table.count == 1);
		CHECK(::strcmp(hash_table_find(table, key)->key, "Hello") == 0);
		CHECK(::strcmp(hash_table_find(table, key)->value, "World!") != 0);
		CHECK(::strcmp(hash_table_find(table, key)->value, "new world!") == 0);

		const char *key2   = "Hi";
		const char *value2 = "Again";
		hash_table_insert(table, key2, value2);
		CHECK(table.count == 2);

		CHECK(table.entries.count == 2);

		CHECK(hash_table_find(table, key) != nullptr);
		CHECK(hash_table_find(table, key2) != nullptr);
		CHECK(hash_table_find(table, value2) == nullptr);

		CHECK(hash_table_remove(table, key));
		CHECK(table.count == 1);
		CHECK(hash_table_find(table, key) == nullptr);

		CHECK(table.entries.count == 1);

		CHECK(hash_table_find(table, key2) != nullptr);
		CHECK(hash_table_remove(table, key2));
		CHECK(table.count == 0);
		CHECK(hash_table_find(table, key2) == nullptr);

		CHECK(table.entries.count == 0);
	}

	SUBCASE("i32, Foo")
	{
		Hash_Table<i32, Foo> table = hash_table_init<i32, Foo>();
		DEFER(hash_table_deinit(table));

		i32 key = 1;
		Foo value = Foo{1};

		hash_table_insert(table, key, value);

		CHECK(table.count == 1);
		CHECK(hash_table_find(table, key) != nullptr);

		CHECK(hash_table_find(table, key)->key == 1);
		CHECK(hash_table_find(table, key)->value == Foo{1});

		hash_table_insert(table, key, Foo{3});

		CHECK(table.count == 1);
		CHECK(hash_table_find(table, key)->key == 1);
		CHECK(hash_table_find(table, key)->value != Foo{1});
		CHECK(hash_table_find(table, key)->value == Foo{3});

		i32 key2   = 2;
		Foo value2 = Foo{2};
		hash_table_insert(table, key2, value2);
		CHECK(table.count == 2);

		CHECK(hash_table_find(table, key) != nullptr);
		CHECK(hash_table_find(table, key2) != nullptr);

		CHECK(hash_table_remove(table, key));
		CHECK(table.count == 1);
		CHECK(hash_table_find(table, key) == nullptr);
		CHECK(hash_table_find(table, key2) != nullptr);
		CHECK(hash_table_remove(table, key2));
		CHECK(table.count == 0);
		CHECK(hash_table_find(table, key2) == nullptr);
	}

	SUBCASE("reserve")
	{
		Hash_Table<i32, i32> table = hash_table_init<i32, i32>(memory::temp_allocator());

		hash_table_reserve(table, 100);

		CHECK(table.count == 0);
		CHECK(table.capacity == 128);

		CHECK(table.slots.data     != nullptr);
		CHECK(table.slots.count    == 128);
		CHECK(table.slots.capacity == 128);

		hash_table_reserve(table, 100);

		CHECK(table.count == 0);
		CHECK(table.capacity == 128);

		CHECK(table.slots.data     != nullptr);
		CHECK(table.slots.count    == 128);
		CHECK(table.slots.capacity == 128);

		for (i32 i = 0; i < 50; ++i)
			hash_table_insert(table, i, i);

		hash_table_reserve(table, 100);

		CHECK(table.count == 50);
		CHECK(table.capacity == 256);

		CHECK(table.slots.data     != nullptr);
		CHECK(table.slots.count    == 256);
		CHECK(table.slots.capacity == 256);
	}

	SUBCASE("remove")
	{
		Hash_Table<i32, i32> table = {};
		DEFER(hash_table_deinit(table));

		CHECK(hash_table_remove(table, 0) == false);
		hash_table_insert(table, 1, 1);
		CHECK(hash_table_remove(table, 1) == true);
	}

	SUBCASE("resize")
	{
		Hash_Table<i32, f32> table = hash_table_init<i32, f32>(memory::temp_allocator());

		for (i32 i = 0; i < 100; ++i)
			hash_table_insert(table, i, i + 0.5f);

		CHECK(table.count == 100);
		CHECK(table.capacity == 256);

		i32 j = 0;
		for (const auto &entry : table)
			CHECK(entry.key == j++);

		for (i32 i = 0; i < 100; ++i)
		{
			auto pair = hash_table_find(table, i);
			CHECK(pair != nullptr);
			CHECK(pair->key == i);
			CHECK(pair->value == (i + 0.5f));
		}

		for (i32 i = 0; i < 100; ++i)
		{
			CHECK(hash_table_remove(table, i) == true);
			CHECK(hash_table_find(table, i) == nullptr);
			CHECK(table.count == (99 - i));
		}

		for (i32 i = 0; i < 100; ++i)
		{
			CHECK(hash_table_remove(table, i) == false);
			CHECK(hash_table_find(table, i) == nullptr);
		}

		CHECK(table.count == 0);
		CHECK(table.capacity == 8);
	}

	SUBCASE("clear")
	{
		Hash_Table<i32, i32> table = hash_table_init<i32, i32>(memory::temp_allocator());

		for (i32 i = 0; i < 10; ++i)
			hash_table_insert(table, i, i + 1);

		CHECK(table.count == 10);
		CHECK(table.capacity == 16);

		i32 j = 0;
		for (auto &entry : table)
		{
			CHECK(entry.key == j++);
			CHECK(entry.value == j);
		}

		hash_table_clear(table);

		CHECK(table.count == 0);
		CHECK(table.capacity == 16);

		for (i32 i = 0; i < 10; ++i)
			CHECK(hash_table_find(table, i) == nullptr);
	}

	SUBCASE("copy/clone/destroy")
	{
		Hash_Table<i32, i32> table1 = hash_table_init<i32, i32>();
		DEFER(destroy(table1));

		CHECK(table1.count    == 0);
		CHECK(table1.capacity == 0);

		for (i32 i = 0; i < 10; ++i)
			hash_table_insert(table1, i, i + 1);

		CHECK(table1.count    == 10);
		CHECK(table1.capacity == 16);

		auto table2 = hash_table_copy(table1);
		DEFER(destroy(table2));

		CHECK(table2.count == 10);
		CHECK(table2.capacity == 16);

		i32 j = 0;
		for (auto &entry : table2)
		{
			CHECK(entry.key == j++);
			CHECK(entry.value == j);
		}

		auto table3 = hash_table_init<Foo, i32>();
		DEFER(destroy(table3));

		for (i32 i = 0; i < 10; ++i)
			hash_table_insert(table3, Foo{i}, i + 10);

		CHECK(table3.count == 10);
		CHECK(table3.capacity == 16);

		i32 k = 0;
		for (const auto &entry : table3)
		{
			CHECK(entry.key == Foo{k});
			CHECK(entry.value == k + 10);
			++k;
		}

		auto table4 = clone(table3);
		DEFER(destroy(table4));

		CHECK(table4.count == 10);
		CHECK(table4.capacity == 16);

		k = 0;
		for (const auto &entry : table4)
		{
			CHECK(entry.key == Foo{k});
			CHECK(entry.value == k + 10);
			++k;
		}
	}

	SUBCASE("user defined key [Foo]")
	{
		Hash_Table<Foo, i32> table = hash_table_init<Foo, i32>(memory::temp_allocator());

		CHECK(table.count    == 0);
		CHECK(table.capacity == 0);

		for (i32 i = 0; i < 10; ++i)
			hash_table_insert(table, Foo{i}, i + 1);

		CHECK(table.count == 10);
		CHECK(table.capacity == 16);

		i32 j = 0;
		for (const auto &entry : table)
		{
			CHECK(entry.key == Foo{j++});
			CHECK(entry.value == j);
		}
	}
}

TEST_CASE("[CONTAINERS]: Hash_Set")
{
	SUBCASE("init")
	{
		{
			Hash_Set<const char *> set = hash_set_init<const char *>();
			DEFER(hash_set_deinit(set));

			CHECK(set.count    == 0);
			CHECK(set.capacity == 0);

			CHECK(set.slots.data     == nullptr);
			CHECK(set.slots.count    == 0);
			CHECK(set.slots.capacity == 0);
		}

		{
			auto set = hash_set_init_with_capacity<i32>(62, memory::temp_allocator());
			CHECK(set.count == 0);
			CHECK(set.capacity == 64);

			CHECK(set.slots.data     != nullptr);
			CHECK(set.slots.count    == 64);
			CHECK(set.slots.capacity == 64);

			hash_set_insert(set, 1);
			hash_set_insert(set, 2);

			CHECK(set.count == 2);
			CHECK(set.capacity == 64);
		}

		{
			auto set = hash_set_init_from<i32>({1, 2}, memory::temp_allocator());

			CHECK(set.count == 2);
			CHECK(set.capacity == 8);

			CHECK(set.slots.data     != nullptr);
			CHECK(set.slots.count    == 8);
			CHECK(set.slots.capacity == 8);

			CHECK(set.entries[0].key == 1);
			CHECK(set.entries[1].key == 2);
		}

		{
			Hash_Set<i32> set = {};
			DEFER(hash_set_deinit(set));

			hash_set_insert(set, 1);
			hash_set_insert(set, 2);

			CHECK(set.count == 2);
			CHECK(set.capacity == 8);

			CHECK(set.slots.data     != nullptr);
			CHECK(set.slots.count    == 8);
			CHECK(set.slots.capacity == 8);

			CHECK(set.entries[0].key == 1);
			CHECK(set.entries[1].key == 2);
		}
	}

	SUBCASE("const char *")
	{
		Hash_Set<const char *> set = hash_set_init<const char *>();
		DEFER(hash_set_deinit(set));

		CHECK(set.count    == 0);
		CHECK(set.capacity == 0);

		const char *key = "Hello";
		hash_set_insert(set, key);

		CHECK(set.entries.count == 1);

		CHECK(set.count == 1);
		auto *k1 = hash_set_find(set, key);
		CHECK(k1 != nullptr);
		CHECK(*k1 != nullptr);
		CHECK(*k1 == key);
		CHECK(*k1 == string_literal("Hello"));

		hash_set_insert(set, key);

		CHECK(set.entries.count == 1);

		CHECK(set.count == 1);
		CHECK(*hash_set_find(set, key) == string_literal("Hello"));

		const char *key2 = "Hi";
		hash_set_insert(set, key2);
		CHECK(set.count == 2);

		CHECK(set.entries.count == 2);

		const char *k2 = "hi";
		CHECK(hash_set_find(set, key) != nullptr);
		CHECK(hash_set_find(set, key2) != nullptr);
		CHECK(hash_set_find(set, k2) == nullptr);

		CHECK(hash_set_remove(set, key));
		CHECK(set.count == 1);
		CHECK(hash_set_find(set, key) == nullptr);

		CHECK(set.entries.count == 1);

		CHECK(hash_set_find(set, key2) != nullptr);
		CHECK(hash_set_remove(set, key2));
		CHECK(set.count == 0);
		CHECK(hash_set_find(set, key2) == nullptr);

		CHECK(set.entries.count == 0);
	}

	SUBCASE("i32, Foo")
	{
		Hash_Set<i32> set = hash_set_init<i32>();
		DEFER(hash_set_deinit(set));

		i32 key = 1;
		hash_set_insert(set, key);

		CHECK(set.count == 1);
		CHECK(hash_set_find(set, key) != nullptr);

		CHECK(hash_set_find(set, key) != nullptr);
		CHECK(*hash_set_find(set, key) == 1);

		hash_set_insert(set, key);
		CHECK(set.count == 1);
		CHECK(hash_set_find(set, key) != nullptr);
		CHECK(*hash_set_find(set, key) == 1);

		i32 key2   = 2;
		hash_set_insert(set, key2);
		CHECK(set.count == 2);

		CHECK(hash_set_find(set, key) != nullptr);
		CHECK(hash_set_find(set, key2) != nullptr);

		CHECK(hash_set_remove(set, key));
		CHECK(set.count == 1);
		CHECK(hash_set_find(set, key) == nullptr);
		CHECK(hash_set_find(set, key2) != nullptr);
		CHECK(hash_set_remove(set, key2));
		CHECK(set.count == 0);
		CHECK(hash_set_find(set, key2) == nullptr);
	}

	SUBCASE("reserve")
	{
		Hash_Set<i32> set = hash_set_init<i32>(memory::temp_allocator());

		hash_set_reserve(set, 100);

		CHECK(set.count == 0);
		CHECK(set.capacity == 128);

		CHECK(set.slots.data     != nullptr);
		CHECK(set.slots.count    == 128);
		CHECK(set.slots.capacity == 128);

		hash_set_reserve(set, 100);

		CHECK(set.count == 0);
		CHECK(set.capacity == 128);

		CHECK(set.slots.data     != nullptr);
		CHECK(set.slots.count    == 128);
		CHECK(set.slots.capacity == 128);

		for (i32 i = 0; i < 50; ++i)
			hash_set_insert(set, i);

		hash_set_reserve(set, 100);

		CHECK(set.count == 50);
		CHECK(set.capacity == 256);

		CHECK(set.slots.data     != nullptr);
		CHECK(set.slots.count    == 256);
		CHECK(set.slots.capacity == 256);
	}

	SUBCASE("remove")
	{
		Hash_Set<i32> set = {};
		DEFER(hash_set_deinit(set));

		CHECK(hash_set_remove(set, 0) == false);
		hash_set_insert(set, 1);
		CHECK(hash_set_remove(set, 1) == true);
	}

	SUBCASE("resize")
	{
		Hash_Set<i32> set = hash_set_init<i32>(memory::temp_allocator());

		for (i32 i = 0; i < 100; ++i)
			hash_set_insert(set, i);

		CHECK(set.count == 100);
		CHECK(set.capacity == 256);

		i32 j = 0;
		for (const auto &entry : set)
			CHECK(entry == j++);

		for (i32 i = 0; i < 100; ++i)
		{
			auto entry = hash_set_find(set, i);
			CHECK(entry != nullptr);
			CHECK(*entry == i);
		}

		for (i32 i = 0; i < 100; ++i)
		{
			CHECK(hash_set_remove(set, i) == true);
			CHECK(hash_set_find(set, i) == nullptr);
			CHECK(set.count == (99 - i));
		}

		for (i32 i = 0; i < 100; ++i)
		{
			CHECK(hash_set_remove(set, i) == false);
			CHECK(hash_set_find(set, i) == nullptr);
		}

		CHECK(set.count == 0);
		CHECK(set.capacity == 8);
	}

	SUBCASE("clear")
	{
		Hash_Set<i32> set = hash_set_init<i32>(memory::temp_allocator());

		for (i32 i = 0; i < 10; ++i)
			hash_set_insert(set, i);

		CHECK(set.count == 10);
		CHECK(set.capacity == 16);

		i32 j = 0;
		for (const auto &entry : set)
			CHECK(entry == j++);

		hash_set_clear(set);

		CHECK(set.count == 0);
		CHECK(set.capacity == 16);

		for (i32 i = 0; i < 10; ++i)
			CHECK(hash_set_find(set, i) == nullptr);
	}

	SUBCASE("copy/clone/destroy")
	{
		Hash_Set<i32> set1 = hash_set_init<i32>();
		DEFER(destroy(set1));

		CHECK(set1.count    == 0);
		CHECK(set1.capacity == 0);

		for (i32 i = 0; i < 10; ++i)
			hash_set_insert(set1, i);

		CHECK(set1.count    == 10);
		CHECK(set1.capacity == 16);

		auto set2 = hash_set_copy(set1);
		DEFER(destroy(set2));

		CHECK(set2.count == 10);
		CHECK(set2.capacity == 16);

		i32 j = 0;
		for (const auto &entry : set2)
			CHECK(entry == j++);

		auto set3 = hash_set_init<Foo>();
		DEFER(destroy(set3));

		for (i32 i = 0; i < 10; ++i)
			hash_set_insert(set3, Foo{i});

		CHECK(set3.count == 10);
		CHECK(set3.capacity == 16);

		i32 k = 0;
		for (const auto &entry : set3)
		{
			CHECK(entry == Foo{k});
			++k;
		}

		auto set4 = clone(set3);
		DEFER(destroy(set4));

		CHECK(set4.count == 10);
		CHECK(set4.capacity == 16);

		k = 0;
		for (const auto &entry : set4)
		{
			CHECK(entry == Foo{k});
			++k;
		}
	}

	SUBCASE("user defined entry [Foo]")
	{
		Hash_Set<Foo> table = hash_set_init<Foo>(memory::temp_allocator());

		CHECK(table.count    == 0);
		CHECK(table.capacity == 0);

		for (i32 i = 0; i < 10; ++i)
			hash_set_insert(table, Foo{i});

		CHECK(table.count == 10);
		CHECK(table.capacity == 16);

		i32 j = 0;
		for (const auto &entry : table)
			CHECK(entry == Foo{j++});
	}
}