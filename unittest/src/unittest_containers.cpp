#include <core/tester.h>
#include <core/defer.h>
#include <core/containers/array.h>
#include <core/containers/hash_set.h>
#include <core/containers/hash_table.h>
#include <core/containers/stack_array.h>
#include <core/containers/string.h>
#include <core/containers/string_interner.h>

TESTER_TEST("[CONTAINERS]: Array")
{
	// ("init")
	{
		{
			auto array = array_init<i32>();
			DEFER(array_deinit(array));

			TESTER_CHECK(array.data == nullptr);
			TESTER_CHECK(array.count == 0);
			TESTER_CHECK(array.capacity == 0);
			TESTER_CHECK(array.allocator != nullptr);
		}

		{
			auto array = array_init_with_capacity<i32>(100);
			DEFER(array_deinit(array));

			TESTER_CHECK(array.data != nullptr);
			TESTER_CHECK(array.count == 0);
			TESTER_CHECK(array.capacity == 100);
			TESTER_CHECK(array.allocator != nullptr);
		}

		{
			auto array = array_init_with_count<i32>(100);
			DEFER(array_deinit(array));

			TESTER_CHECK(array.data != nullptr);
			TESTER_CHECK(array.count == 100);
			TESTER_CHECK(array.capacity == 100);
			TESTER_CHECK(array.allocator != nullptr);
		}

		{
			auto array = array_init_from({1, 2, 3, 4, 5});
			DEFER(array_deinit(array));

			TESTER_CHECK(array.data != nullptr);
			TESTER_CHECK(array.count == 5);
			TESTER_CHECK(array.capacity == 5);
			TESTER_CHECK(array.allocator != nullptr);
			TESTER_CHECK(array[0] == 1);
			TESTER_CHECK(array[1] == 2);
			TESTER_CHECK(array[2] == 3);
			TESTER_CHECK(array[3] == 4);
			TESTER_CHECK(array[4] == 5);
		}
	}

	// ("copy")
	{
		auto array1 = array_init_from({1, 2, 3});
		DEFER(array_deinit(array1));

		auto array2 = array_copy(array1);
		DEFER(array_deinit(array2));

		TESTER_CHECK(array2.count == array1.count);
		TESTER_CHECK(array2.capacity == array1.capacity);
		for (u64 i = 0; i < array2.count; ++i)
			TESTER_CHECK(array2[i] == array1[i]);
	}

	// ("fill")
	{
		auto array = array_init_with_count<f32>(50);
		DEFER(array_deinit(array));

		array_fill(array, 5.0f);
		for (u64 i = 0; i < array.count; ++i)
			TESTER_CHECK(array[i] == 5.0f);
	}

	// ("reserve")
	{
		auto array = array_init_with_capacity<i32>(10);
		DEFER(array_deinit(array));

		TESTER_CHECK(array.data != nullptr);
		TESTER_CHECK(array.capacity == 10);
		array_reserve(array, 20);
		TESTER_CHECK(array.data != nullptr);
		TESTER_CHECK(array.capacity == 30);
	}

	// ("push/pop/last")
	{
		auto array = array_init<u64>();
		DEFER(array_deinit(array));

		for (u64 i = 0; i < 100; ++i)
			array_push(array, i);
		TESTER_CHECK(array.count == 100);

		for (u64 i = 0; i < array.count; ++i)
			TESTER_CHECK(array[i] == i);

		TESTER_CHECK(array_last(array) == 99);

		for (u64 i = 0; i < 100; ++i)
			TESTER_CHECK(array_pop(array) == 99 - i);

		TESTER_CHECK(array.count == 0);
	}

	// ("remove")
	{
		auto array = array_init<u64>();
		DEFER(array_deinit(array));

		for (u64 i = 0; i < 100; ++i)
			array_push(array, i);
		TESTER_CHECK(array.count == 100);

		array_remove(array, array.count - 1);
		TESTER_CHECK(array.count == 99);
		for (u64 i = 0; i < 99; ++i)
			TESTER_CHECK(array[i] == i);

		array_remove(array, 0);
		TESTER_CHECK(array[0] == 98);
		TESTER_CHECK(array.count == 98);

		array_remove_ordered(array, 0);
		for (u64 i = 0; i < 97; ++i)
			TESTER_CHECK(array[i] == i + 1);
		TESTER_CHECK(array.count == 97);
		array_remove(array, 0);

		array_remove_if(array, [](u64 element) {
			return element % 2 == 0;
		});
		TESTER_CHECK(array.count == 48);
	}

	// ("remove_ordered")
	{
		auto array = array_init<i32>();
		DEFER(array_deinit(array));

		for (u64 i = 0; i < 100; ++i)
			array_push(array, i32(i));
		TESTER_CHECK(array.count == 100);

		array_remove_ordered_if(array, [](i32 element) {
			return element % 2 == 0;
		});

		i32 j = 1;
		for (u64 i = 0; i < 50; ++i)
		{
			TESTER_CHECK(array[i] == j);
			j += 2;
		}
		TESTER_CHECK(array.count == 50);

		array_remove_ordered(array, 0);
		j = 3;
		for (u64 i = 0; i < 49; ++i)
		{
			TESTER_CHECK(array[i] == j);
			j += 2;
		}
		TESTER_CHECK(array.count == 49);
	}

	// ("append")
	{
		auto array1 = array_init_from({0, 1, 2, 3, 4});
		DEFER(array_deinit(array1));

		auto array2 = array_init_from({5, 6, 7, 8, 9});
		DEFER(array_deinit(array2));

		array_append(array1, array2);
		TESTER_CHECK(array1.count == 10);

		for (i32 i = 0; i < (i32)array1.count; ++i)
			TESTER_CHECK(array1[i] == i);
	}

	// ("iterators")
	{
		auto array = array_init_from<i32>({0, 1, 2, 3, 4});
		DEFER(array_deinit(array));

		TESTER_CHECK(begin(array) == array.data);
		TESTER_CHECK(end(array) == array.data + array.count);

		i32 i = 0;
		for (auto v : array)
			TESTER_CHECK(v == i++);

		i = 0;
		for (const auto v: array)
			TESTER_CHECK(v == i++);

		for (auto &v : array)
			v++;

		i = 1;
		for (const auto &v: array)
			TESTER_CHECK(v == i++);
	}

	// ("clone")
	{
		auto array1 = array_init<Array<i32>>();
		DEFER(destroy(array1));
		array_push(array1, array_init_from<i32>({1, 2, 3}));

		auto array2 = clone(array1);
		DEFER(destroy(array2));

		for (u64 i = 0; i < array2.count; ++i)
		{
			TESTER_CHECK(array2.count == array1.count);
			for (u64 j = 0; j < array2[i].count; ++j)
			{
				auto v1 = array1[i][j];
				auto v2 = array2[i][j];
				TESTER_CHECK(v1 == v2);
			}
		}
	}

	// ("destroy")
	{
		auto v = array_init<Array<i32>>();
		DEFER(destroy(v));

		array_push(v, array_init_from<i32>({1, 2, 3}));
		array_push(v, array_init_from<i32>({4, 5, 6}));
	}
}

TESTER_TEST("[CONTAINERS]: Stack_Array")
{
	// ("init")
	{
		{
			Stack_Array<u64, 4> array{};
			TESTER_CHECK(array.count == 0);
		}

		{
			Stack_Array array{{1, 2, 3}};
			for (u64 i = 0; i < array.count; ++i)
				TESTER_CHECK(array[i] == i32(i + 1));
			TESTER_CHECK(array.count == 3);
		}

		{
			Stack_Array<u64, 3> array{{1, 2, 3}};
			for (u64 i = 0; i < array.count; ++i)
				TESTER_CHECK(array[i] == i + 1);
			TESTER_CHECK(array.count == 3);
		}
	}
}

TESTER_TEST("[CONTAINERS]: String")
{
	// ("init")
	{
		auto s = string_init();
		TESTER_CHECK(s.count == 0);
		TESTER_CHECK(s.capacity == 1);
		TESTER_CHECK(*s.data == '\0');
		string_deinit(s);

		auto c_string = "Hello, World!";
		s = string_from(c_string);
		TESTER_CHECK(s.count == 13);
		TESTER_CHECK(s.capacity == 14);
		for (u64 i = 0; i < s.count; ++i)
			TESTER_CHECK(s[i] == c_string[i]);
		TESTER_CHECK(s.data[s.count] == '\0');
		string_deinit(s);

		s = string_literal(c_string);
		TESTER_CHECK(s.allocator == nullptr);
		TESTER_CHECK(s.count == 13);
		TESTER_CHECK(s.capacity == 14);
		for (u64 i = 0; i < s.count; ++i)
			TESTER_CHECK(s[i] == c_string[i]);
		TESTER_CHECK(s.data[s.count] == '\0');
		string_deinit(s);

		auto c_string_empty = "";
		s = string_from(c_string_empty);
		TESTER_CHECK(s.count == 0);
		TESTER_CHECK(s.capacity == 1);
		TESTER_CHECK(s.data[s.count] == '\0');
		string_deinit(s);

		s = string_literal(c_string_empty);
		TESTER_CHECK(s.allocator == nullptr);
		TESTER_CHECK(s.count == 0);
		TESTER_CHECK(s.capacity == 1);
		TESTER_CHECK(s.data[s.count] == '\0');
		string_deinit(s);

		auto literal = "Hello, World!";
		s = string_from(literal, literal + 13);
		TESTER_CHECK(s.count == 13);
		TESTER_CHECK(s.capacity == 14);
		for (u64 i = 0; i < 13; ++i)
			TESTER_CHECK(s[i] == literal[i]);
		string_deinit(s);
	}

	// ("copy")
	{
		auto s1 = string_from("Copy");
		DEFER(string_deinit(s1));

		auto s2 = string_copy(s1);
		DEFER(string_deinit(s2));

		TESTER_CHECK(s2.count == s1.count);
		TESTER_CHECK(s2.capacity == s1.capacity);
		for (u64 i = 0; i < s2.count; ++i)
			TESTER_CHECK(s2[i] == s1[i]);
	}

	// ("append")
	{
		auto s1 = "Hello, ";
		auto s2 = "World";
		auto s3 = string_from("!");

		auto s = string_init();
		string_append(s, s1);
		string_append(s, s2);
		string_append(s, s3);

		TESTER_CHECK(s.count == 13);
		TESTER_CHECK(s.capacity == 14);

		auto expected = "Hello, World!";
		for (u64 i = 0; i < s.count; ++i)
			TESTER_CHECK(s[i] == expected[i]);
		TESTER_CHECK(s.data[s.count] == '\0');

		string_deinit(s);
		string_deinit(s3);
	}

	// ("to lower/ to upper")
	{
		auto s = string_from("HELLO");
		DEFER(string_deinit(s));

		TESTER_CHECK(string_to_lowercase(s) == "hello");
		TESTER_CHECK(string_to_uppercase(s) == "HELLO");
	}

	// ("find/contains")
	{
		auto s = string_from("Hello World!");
		DEFER(string_deinit(s));

		TESTER_CHECK(string_find_first_of(s, string_literal("Hello")) == 0);
		TESTER_CHECK(string_find_first_of(s, string_literal("World!")) == 6);
		TESTER_CHECK(string_find_first_of(s, string_literal("olleH")) == U64_MAX);

		TESTER_CHECK(string_find_first_of(s, 'H', 0) == 0);
		TESTER_CHECK(string_find_first_of(s, 'W', 0) == 6);

		TESTER_CHECK(string_find_last_of(string_literal("Hello, World. World"), string_literal("Hello")) == 0);
		TESTER_CHECK(string_find_last_of(string_literal("Hello, World. World Hello"), string_literal("World")) == 14);
		TESTER_CHECK(string_find_last_of(string_literal("Hello, World. World Hello"), string_literal("o")) == 24);
		TESTER_CHECK(string_find_last_of(s, 'l') == 9);
		TESTER_CHECK(string_find_last_of(s, '!') == 11);

		TESTER_CHECK(string_contains(s, string_literal("Hello")));
		TESTER_CHECK(string_contains(s, string_literal("World!")));
		TESTER_CHECK(string_contains(s, string_literal("hello"), true));
		TESTER_CHECK(string_contains(s, string_literal("world!"), true));
	}

	// ("trim")
	{
		auto s1 = string_from(" Hello, World! ");
		DEFER(string_deinit(s1));

		string_trim(s1, string_literal(" "));

		auto expected = "Hello, World!";
		TESTER_CHECK(s1.count == 13);
		TESTER_CHECK(s1.capacity == 14);
		for (u64 i = 0; i < s1.count; ++i)
			TESTER_CHECK(s1[i] == expected[i]);
		TESTER_CHECK(s1.data[s1.count] == '\0');

		// ====================== //
		string_trim_left(s1, string_literal("H"));
		TESTER_CHECK(s1.count == 12);
		TESTER_CHECK(s1.capacity == 13);
		for (u64 i = 1; i < s1.count; ++i)
			TESTER_CHECK(s1[i - 1] == expected[i]);
		TESTER_CHECK(s1.data[s1.count] == '\0');

		// ====================== //
		string_trim_right(s1, string_literal("!"));
		TESTER_CHECK(s1.count == 11);
		TESTER_CHECK(s1.capacity == 12);
		for (u64 i = 1; i < s1.count; ++i)
			TESTER_CHECK(s1[i - 1] == expected[i]);
		TESTER_CHECK(s1.data[s1.count] == '\0');

		// ====================== //
		auto s2 = string_from(" Hello, World! ");
		DEFER(string_deinit(s2));

		string_trim_whitespace(s2);

		TESTER_CHECK(s2.count == 13);
		TESTER_CHECK(s2.capacity == 14);
		for (u64 i = 0; i < s2.count; ++i)
			TESTER_CHECK(s2[i] == expected[i]);
		TESTER_CHECK(s2.data[s2.count] == '\0');
	}

	// ("split")
	{
		{
			auto s = string_from("Hello World!");
			DEFER(string_deinit(s));

			auto splits = string_split(s, ' ');
			DEFER(destroy(splits));

			TESTER_CHECK(splits.count == 2);
			TESTER_CHECK(splits[0] == "Hello");
			TESTER_CHECK(splits[1] == "World!");
		}

		{
			auto s = string_from("Hello, to,, the, new, World!");
			DEFER(string_deinit(s));

			auto splits = string_split(s, ',', false);
			DEFER(destroy(splits));

			TESTER_CHECK(splits.count == 6);
			TESTER_CHECK(splits[0] == "Hello");
			TESTER_CHECK(splits[1] == " to");
			TESTER_CHECK(splits[2] == "");
			TESTER_CHECK(splits[3] == " the");
			TESTER_CHECK(splits[4] == " new");
			TESTER_CHECK(splits[5] == " World!");
		}

		{
			auto s = string_from("Hello, to,, the, new, World!");
			DEFER(string_deinit(s));

			auto splits = string_split(s, ',');
			DEFER(destroy(splits));

			TESTER_CHECK(splits.count == 5);
			TESTER_CHECK(splits[0] == "Hello");
			TESTER_CHECK(splits[1] == " to");
			TESTER_CHECK(splits[2] == " the");
			TESTER_CHECK(splits[3] == " new");
			TESTER_CHECK(splits[4] == " World!");
		}

		{
			auto s = string_from("Hello,split Worldsplit!");
			DEFER(string_deinit(s));

			auto splits = string_split(s, string_literal("split"));
			DEFER(destroy(splits));

			TESTER_CHECK(splits.count == 3);
			TESTER_CHECK(splits[0] == "Hello,");
			TESTER_CHECK(splits[1] == " World");
			TESTER_CHECK(splits[2] == "!");
		}
	}

	// ("replace")
	{
		auto s = string_from("Helloxxx, xxxWorld!xxx", memory::temp_allocator());
		DEFER(string_deinit(s));

		string_replace(s, string_literal("xxx"), string_literal(""));

		auto expected = "Hello, World!";
		TESTER_CHECK(s.count == 13);
		for (u64 i = 0; i < s.count; ++i)
			TESTER_CHECK(s[i] == expected[i]);
		TESTER_CHECK(s.data[s.count] == '\0');

		auto s2 = string_from("Helloxxx, xxxWorld!xxx", memory::temp_allocator());
		string_replace_first_occurance(s2, "xxx", "", 5);
		TESTER_CHECK(s2 == "Hello, xxxWorld!xxx");
		string_replace_first_occurance(s2, "xxx", "", 10);
		TESTER_CHECK(s2 == "Hello, xxxWorld!");
		string_replace_first_occurance(s2, "xxx", "");
		TESTER_CHECK(s2 == "Hello, World!");
	}

	// ("starts with")
	{
		auto s = string_from("Hello, World!");
		DEFER(string_deinit(s));

		TESTER_CHECK(string_starts_with(s, string_literal("Hello")) == true);
		TESTER_CHECK(string_starts_with(s, string_literal("ello")) == false);

		TESTER_CHECK(string_starts_with(s, "Hello") == true);
		TESTER_CHECK(string_starts_with(s, "ello") == false);

		TESTER_CHECK(string_starts_with("Hello, World!", s) == true);
		TESTER_CHECK(string_starts_with("World, Hello!", s) == false);

		TESTER_CHECK(string_starts_with("Hello", "H") == true);
		TESTER_CHECK(string_starts_with("Hello", "e") == false);
	}

	// ("ends with")
	{
		auto s = string_from("Hello, World!");
		DEFER(string_deinit(s));

		TESTER_CHECK(string_ends_with(s, string_literal("World!")) == true);
		TESTER_CHECK(string_ends_with(s, string_literal("Hello")) == false);

		TESTER_CHECK(string_ends_with(s, "World!") == true);
		TESTER_CHECK(string_ends_with(s, "Hello") == false);

		TESTER_CHECK(string_ends_with("Hello, World!", s) == true);
		TESTER_CHECK(string_ends_with("World, Hello!", s) == false);

		TESTER_CHECK(string_ends_with("Hello", "o") == true);
		TESTER_CHECK(string_ends_with("Hello", "H") == false);
	}

	// ("clear")
	{
		auto s = string_from("Hello, World!");
		DEFER(string_deinit(s));

		string_clear(s);

		TESTER_CHECK(s.count == 0);
		TESTER_CHECK(s.data[s.count] == '\0');
	}

	// ("equal")
	{
		auto s1 = string_from("Hello");
		auto s2 = string_from("World");
		auto s3 = string_from("Hello");
		auto s4 = string_from("");

		TESTER_CHECK(s1 != s2);
		TESTER_CHECK(s1 == s3);
		TESTER_CHECK(s1 == "Hello");
		TESTER_CHECK(s1 != "World");
		TESTER_CHECK("Hello" == s1);
		TESTER_CHECK("World" != s1);
		TESTER_CHECK(s4 == "");
		TESTER_CHECK(s4 != "Hello");

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

TESTER_TEST("[CONTAINERS]: Hash_Table")
{
	// ("init")
	{
		{
			Hash_Table<const char *, const char *> table = hash_table_init<const char *, const char *>();
			DEFER(hash_table_deinit(table));

			TESTER_CHECK(table.count    == 0);
			TESTER_CHECK(table.capacity == 0);

			TESTER_CHECK(table.slots.data     == nullptr);
			TESTER_CHECK(table.slots.count    == 0);
			TESTER_CHECK(table.slots.capacity == 0);
		}

		{
			auto table = hash_table_init_with_capacity<i32, const char *>(62, memory::temp_allocator());
			TESTER_CHECK(table.count == 0);
			TESTER_CHECK(table.capacity == 64);

			TESTER_CHECK(table.slots.data     != nullptr);
			TESTER_CHECK(table.slots.count    == 64);
			TESTER_CHECK(table.slots.capacity == 64);

			hash_table_insert(table, 1, (const char *)"Hello");
			hash_table_insert(table, 2, (const char *)"World!");

			TESTER_CHECK(table.count == 2);
			TESTER_CHECK(table.capacity == 64);
		}

		{
			auto table = hash_table_init_from<i32, const char *>({ {1, "Hello"}, {2, "World!"} }, memory::temp_allocator());

			TESTER_CHECK(table.count == 2);
			TESTER_CHECK(table.capacity == 8);

			TESTER_CHECK(table.slots.data     != nullptr);
			TESTER_CHECK(table.slots.count    == 8);
			TESTER_CHECK(table.slots.capacity == 8);

			TESTER_CHECK(table.entries[0].key == 1);
			TESTER_CHECK(::strcmp(table.entries[0].value, "Hello") == 0);

			TESTER_CHECK(table.entries[1].key == 2);
			TESTER_CHECK(::strcmp(table.entries[1].value, "World!") == 0);
		}

		{
			Hash_Table<i32, i32> table = {};
			DEFER(hash_table_deinit(table));

			hash_table_insert(table, 1, 1);
			hash_table_insert(table, 2, 2);

			TESTER_CHECK(table.count == 2);
			TESTER_CHECK(table.capacity == 8);

			TESTER_CHECK(table.slots.data     != nullptr);
			TESTER_CHECK(table.slots.count    == 8);
			TESTER_CHECK(table.slots.capacity == 8);

			TESTER_CHECK(table.entries[0].key == 1);
			TESTER_CHECK(table.entries[0].value == 1);

			TESTER_CHECK(table.entries[1].key == 2);
			TESTER_CHECK(table.entries[1].value == 2);
		}
	}

	// ("operator[]")
	{
		Hash_Table<i32, i32> table = {};
		DEFER(hash_table_deinit(table));

		hash_table_insert(table, 1, 1);
		hash_table_insert(table, 2, 2);

		table[1] = 3;

		TESTER_CHECK(table.count == 2);
		TESTER_CHECK(table.capacity == 8);
		TESTER_CHECK(hash_table_find(table, 1)->value == 3);

		TESTER_CHECK(table.entries[0].key == 1);
		TESTER_CHECK(table.entries[0].value == 3);

		TESTER_CHECK(table.entries[1].key == 2);
		TESTER_CHECK(table.entries[1].value == 2);

		TESTER_CHECK(table[1] == 3);

		i32 x = table[1];
		TESTER_CHECK(x == 3);

		const i32 &xx = table[1];
		TESTER_CHECK(xx == 3);

		i32 &y = table[1];
		y = 1;

		TESTER_CHECK(table.entries[0].key == 1);
		TESTER_CHECK(table.entries[0].value == 1);

		table[5] = 0;
		TESTER_CHECK(table[5] == 0);

		TESTER_CHECK(table.entries[2].key == 5);
		TESTER_CHECK(table.entries[2].value == 0);
	}

	// ("const char *")
	{
		Hash_Table<const char *, const char *> table = hash_table_init<const char *, const char *>();
		DEFER(hash_table_deinit(table));

		TESTER_CHECK(table.count    == 0);
		TESTER_CHECK(table.capacity == 0);

		const char *key   = "Hello";
		const char *value = "World!";
		hash_table_insert(table, key, value);

		TESTER_CHECK(table.entries.count == 1);

		TESTER_CHECK(table.count == 1);
		TESTER_CHECK(hash_table_find(table, key) != nullptr);
		TESTER_CHECK(hash_table_find(table, value) == nullptr);

		TESTER_CHECK(::strcmp(hash_table_find(table, key)->key, "Hello") == 0);
		TESTER_CHECK(::strcmp(hash_table_find(table, key)->value, "World!") == 0);

		const char *dumb_value = "new world!";
		hash_table_insert(table, key, dumb_value);

		TESTER_CHECK(table.entries.count == 1);

		TESTER_CHECK(table.count == 1);
		TESTER_CHECK(::strcmp(hash_table_find(table, key)->key, "Hello") == 0);
		TESTER_CHECK(::strcmp(hash_table_find(table, key)->value, "World!") != 0);
		TESTER_CHECK(::strcmp(hash_table_find(table, key)->value, "new world!") == 0);

		const char *key2   = "Hi";
		const char *value2 = "Again";
		hash_table_insert(table, key2, value2);
		TESTER_CHECK(table.count == 2);

		TESTER_CHECK(table.entries.count == 2);

		TESTER_CHECK(hash_table_find(table, key) != nullptr);
		TESTER_CHECK(hash_table_find(table, key2) != nullptr);
		TESTER_CHECK(hash_table_find(table, value2) == nullptr);

		TESTER_CHECK(hash_table_remove(table, key));
		TESTER_CHECK(table.count == 1);
		TESTER_CHECK(hash_table_find(table, key) == nullptr);

		TESTER_CHECK(table.entries.count == 1);

		TESTER_CHECK(hash_table_find(table, key2) != nullptr);
		TESTER_CHECK(hash_table_remove(table, key2));
		TESTER_CHECK(table.count == 0);
		TESTER_CHECK(hash_table_find(table, key2) == nullptr);

		TESTER_CHECK(table.entries.count == 0);
	}

	// ("i32, Foo")
	{
		Hash_Table<i32, Foo> table = hash_table_init<i32, Foo>();
		DEFER(hash_table_deinit(table));

		i32 key = 1;
		Foo value = Foo{1};

		hash_table_insert(table, key, value);

		TESTER_CHECK(table.count == 1);
		TESTER_CHECK(hash_table_find(table, key) != nullptr);

		TESTER_CHECK(hash_table_find(table, key)->key == 1);
		TESTER_CHECK(hash_table_find(table, key)->value == Foo{1});

		hash_table_insert(table, key, Foo{3});

		TESTER_CHECK(table.count == 1);
		TESTER_CHECK(hash_table_find(table, key)->key == 1);
		TESTER_CHECK(hash_table_find(table, key)->value != Foo{1});
		TESTER_CHECK(hash_table_find(table, key)->value == Foo{3});

		i32 key2   = 2;
		Foo value2 = Foo{2};
		hash_table_insert(table, key2, value2);
		TESTER_CHECK(table.count == 2);

		TESTER_CHECK(hash_table_find(table, key) != nullptr);
		TESTER_CHECK(hash_table_find(table, key2) != nullptr);

		TESTER_CHECK(hash_table_remove(table, key));
		TESTER_CHECK(table.count == 1);
		TESTER_CHECK(hash_table_find(table, key) == nullptr);
		TESTER_CHECK(hash_table_find(table, key2) != nullptr);
		TESTER_CHECK(hash_table_remove(table, key2));
		TESTER_CHECK(table.count == 0);
		TESTER_CHECK(hash_table_find(table, key2) == nullptr);
	}

	// ("reserve")
	{
		Hash_Table<i32, i32> table = hash_table_init<i32, i32>(memory::temp_allocator());

		hash_table_reserve(table, 100);

		TESTER_CHECK(table.count == 0);
		TESTER_CHECK(table.capacity == 128);

		TESTER_CHECK(table.slots.data     != nullptr);
		TESTER_CHECK(table.slots.count    == 128);
		TESTER_CHECK(table.slots.capacity == 128);

		hash_table_reserve(table, 100);

		TESTER_CHECK(table.count == 0);
		TESTER_CHECK(table.capacity == 128);

		TESTER_CHECK(table.slots.data     != nullptr);
		TESTER_CHECK(table.slots.count    == 128);
		TESTER_CHECK(table.slots.capacity == 128);

		for (i32 i = 0; i < 50; ++i)
			hash_table_insert(table, i, i);

		hash_table_reserve(table, 100);

		TESTER_CHECK(table.count == 50);
		TESTER_CHECK(table.capacity == 256);

		TESTER_CHECK(table.slots.data     != nullptr);
		TESTER_CHECK(table.slots.count    == 256);
		TESTER_CHECK(table.slots.capacity == 256);
	}

	// ("remove")
	{
		// ("unordered")
		{
			Hash_Table<i32, i32> table = {};
			DEFER(hash_table_deinit(table));

			TESTER_CHECK(hash_table_remove(table, 0) == false);
			hash_table_insert(table, 1, 1);
			TESTER_CHECK(hash_table_remove(table, 1) == true);
		}

		// ("ordered")
		{
			Hash_Table<i32, i32> table = {};
			DEFER(hash_table_deinit(table));
			hash_table_insert(table, 1, 1);
			hash_table_insert(table, 2, 2);
			hash_table_insert(table, 3, 3);
			hash_table_insert(table, 4, 4);
			hash_table_insert(table, 5, 5);
			hash_table_insert(table, 6, 6);

			TESTER_CHECK(hash_table_remove_ordered(table, 0) == false);
			TESTER_CHECK(hash_table_remove_ordered(table, 1) == true);
			TESTER_CHECK(hash_table_remove_ordered(table, 3) == true);
			TESTER_CHECK(hash_table_remove_ordered(table, 5) == true);

			TESTER_CHECK(table.entries.count == 3);

			TESTER_CHECK(table.entries[0].key   == 2);
			TESTER_CHECK(table.entries[0].value == 2);

			TESTER_CHECK(table.entries[1].key   == 4);
			TESTER_CHECK(table.entries[1].value == 4);

			TESTER_CHECK(table.entries[2].key   == 6);
			TESTER_CHECK(table.entries[2].value == 6);

			TESTER_CHECK(hash_table_find(table, 1) == nullptr);
			TESTER_CHECK(hash_table_find(table, 3) == nullptr);
			TESTER_CHECK(hash_table_find(table, 5) == nullptr);

			TESTER_CHECK(hash_table_find(table, 2) != nullptr);
			TESTER_CHECK(hash_table_find(table, 2)->key == 2);
			TESTER_CHECK(hash_table_find(table, 2)->value == 2);

			TESTER_CHECK(hash_table_find(table, 4) != nullptr);
			TESTER_CHECK(hash_table_find(table, 4)->key == 4);
			TESTER_CHECK(hash_table_find(table, 4)->value == 4);

			TESTER_CHECK(hash_table_find(table, 6) != nullptr);
			TESTER_CHECK(hash_table_find(table, 6)->key == 6);
			TESTER_CHECK(hash_table_find(table, 6)->value == 6);
		}
	}

	// ("resize")
	{
		Hash_Table<i32, f32> table = hash_table_init<i32, f32>(memory::temp_allocator());

		for (i32 i = 0; i < 100; ++i)
			hash_table_insert(table, i, i + 0.5f);

		TESTER_CHECK(table.count == 100);
		TESTER_CHECK(table.capacity == 256);

		i32 j = 0;
		for (const auto &entry : table)
			TESTER_CHECK(entry.key == j++);

		for (i32 i = 0; i < 100; ++i)
		{
			auto pair = hash_table_find(table, i);
			TESTER_CHECK(pair != nullptr);
			TESTER_CHECK(pair->key == i);
			TESTER_CHECK(pair->value == (i + 0.5f));
		}

		for (i32 i = 0; i < 100; ++i)
		{
			TESTER_CHECK(hash_table_remove(table, i) == true);
			TESTER_CHECK(hash_table_find(table, i) == nullptr);
			TESTER_CHECK(table.count == u64(99 - i));
		}

		for (i32 i = 0; i < 100; ++i)
		{
			TESTER_CHECK(hash_table_remove(table, i) == false);
			TESTER_CHECK(hash_table_find(table, i) == nullptr);
		}

		TESTER_CHECK(table.count == 0);
		TESTER_CHECK(table.capacity == 8);
	}

	// ("clear")
	{
		Hash_Table<i32, i32> table = hash_table_init<i32, i32>(memory::temp_allocator());

		for (i32 i = 0; i < 10; ++i)
			hash_table_insert(table, i, i + 1);

		TESTER_CHECK(table.count == 10);
		TESTER_CHECK(table.capacity == 16);

		i32 j = 0;
		for (auto &entry : table)
		{
			TESTER_CHECK(entry.key == j++);
			TESTER_CHECK(entry.value == j);
		}

		hash_table_clear(table);

		TESTER_CHECK(table.count == 0);
		TESTER_CHECK(table.capacity == 16);

		for (i32 i = 0; i < 10; ++i)
			TESTER_CHECK(hash_table_find(table, i) == nullptr);
	}

	// ("copy/clone/destroy")
	{
		Hash_Table<i32, i32> table1 = hash_table_init<i32, i32>();
		DEFER(destroy(table1));

		TESTER_CHECK(table1.count    == 0);
		TESTER_CHECK(table1.capacity == 0);

		for (i32 i = 0; i < 10; ++i)
			hash_table_insert(table1, i, i + 1);

		TESTER_CHECK(table1.count    == 10);
		TESTER_CHECK(table1.capacity == 16);

		auto table2 = hash_table_copy(table1);
		DEFER(destroy(table2));

		TESTER_CHECK(table2.count == 10);
		TESTER_CHECK(table2.capacity == 16);

		i32 j = 0;
		for (auto &entry : table2)
		{
			TESTER_CHECK(entry.key == j++);
			TESTER_CHECK(entry.value == j);
		}

		auto table3 = hash_table_init<Foo, i32>();
		DEFER(destroy(table3));

		for (i32 i = 0; i < 10; ++i)
			hash_table_insert(table3, Foo{i}, i + 10);

		TESTER_CHECK(table3.count == 10);
		TESTER_CHECK(table3.capacity == 16);

		i32 k = 0;
		for (const auto &entry : table3)
		{
			TESTER_CHECK(entry.key == Foo{k});
			TESTER_CHECK(entry.value == k + 10);
			++k;
		}

		auto table4 = clone(table3);
		DEFER(destroy(table4));

		TESTER_CHECK(table4.count == 10);
		TESTER_CHECK(table4.capacity == 16);

		k = 0;
		for (const auto &entry : table4)
		{
			TESTER_CHECK(entry.key == Foo{k});
			TESTER_CHECK(entry.value == k + 10);
			++k;
		}
	}

	// ("user defined key [Foo]")
	{
		Hash_Table<Foo, i32> table = hash_table_init<Foo, i32>(memory::temp_allocator());

		TESTER_CHECK(table.count    == 0);
		TESTER_CHECK(table.capacity == 0);

		for (i32 i = 0; i < 10; ++i)
			hash_table_insert(table, Foo{i}, i + 1);

		TESTER_CHECK(table.count == 10);
		TESTER_CHECK(table.capacity == 16);

		i32 j = 0;
		for (const auto &entry : table)
		{
			TESTER_CHECK(entry.key == Foo{j++});
			TESTER_CHECK(entry.value == j);
		}
	}
}

TESTER_TEST("[CONTAINERS]: Hash_Set")
{
	// ("init")
	{
		{
			Hash_Set<const char *> set = hash_set_init<const char *>();
			DEFER(hash_set_deinit(set));

			TESTER_CHECK(set.count    == 0);
			TESTER_CHECK(set.capacity == 0);

			TESTER_CHECK(set.slots.data     == nullptr);
			TESTER_CHECK(set.slots.count    == 0);
			TESTER_CHECK(set.slots.capacity == 0);
		}

		{
			auto set = hash_set_init_with_capacity<i32>(62, memory::temp_allocator());
			TESTER_CHECK(set.count == 0);
			TESTER_CHECK(set.capacity == 64);

			TESTER_CHECK(set.slots.data     != nullptr);
			TESTER_CHECK(set.slots.count    == 64);
			TESTER_CHECK(set.slots.capacity == 64);

			hash_set_insert(set, 1);
			hash_set_insert(set, 2);

			TESTER_CHECK(set.count == 2);
			TESTER_CHECK(set.capacity == 64);
		}

		{
			auto set = hash_set_init_from<i32>({1, 2}, memory::temp_allocator());

			TESTER_CHECK(set.count == 2);
			TESTER_CHECK(set.capacity == 8);

			TESTER_CHECK(set.slots.data     != nullptr);
			TESTER_CHECK(set.slots.count    == 8);
			TESTER_CHECK(set.slots.capacity == 8);

			TESTER_CHECK(set.entries[0].key == 1);
			TESTER_CHECK(set.entries[1].key == 2);
		}

		{
			Hash_Set<i32> set = {};
			DEFER(hash_set_deinit(set));

			hash_set_insert(set, 1);
			hash_set_insert(set, 2);

			TESTER_CHECK(set.count == 2);
			TESTER_CHECK(set.capacity == 8);

			TESTER_CHECK(set.slots.data     != nullptr);
			TESTER_CHECK(set.slots.count    == 8);
			TESTER_CHECK(set.slots.capacity == 8);

			TESTER_CHECK(set.entries[0].key == 1);
			TESTER_CHECK(set.entries[1].key == 2);
		}
	}

	// ("const char *")
	{
		Hash_Set<const char *> set = hash_set_init<const char *>();
		DEFER(hash_set_deinit(set));

		TESTER_CHECK(set.count    == 0);
		TESTER_CHECK(set.capacity == 0);

		const char *key = "Hello";
		hash_set_insert(set, key);

		TESTER_CHECK(set.entries.count == 1);

		TESTER_CHECK(set.count == 1);
		auto *k1 = hash_set_find(set, key);
		TESTER_CHECK(k1 != nullptr);
		TESTER_CHECK(*k1 != nullptr);
		TESTER_CHECK(*k1 == key);
		TESTER_CHECK(*k1 == string_literal("Hello"));

		hash_set_insert(set, key);

		TESTER_CHECK(set.entries.count == 1);

		TESTER_CHECK(set.count == 1);
		TESTER_CHECK(*hash_set_find(set, key) == string_literal("Hello"));

		const char *key2 = "Hi";
		hash_set_insert(set, key2);
		TESTER_CHECK(set.count == 2);

		TESTER_CHECK(set.entries.count == 2);

		const char *k2 = "hi";
		TESTER_CHECK(hash_set_find(set, key) != nullptr);
		TESTER_CHECK(hash_set_find(set, key2) != nullptr);
		TESTER_CHECK(hash_set_find(set, k2) == nullptr);

		TESTER_CHECK(hash_set_remove(set, key));
		TESTER_CHECK(set.count == 1);
		TESTER_CHECK(hash_set_find(set, key) == nullptr);

		TESTER_CHECK(set.entries.count == 1);

		TESTER_CHECK(hash_set_find(set, key2) != nullptr);
		TESTER_CHECK(hash_set_remove(set, key2));
		TESTER_CHECK(set.count == 0);
		TESTER_CHECK(hash_set_find(set, key2) == nullptr);

		TESTER_CHECK(set.entries.count == 0);
	}

	// ("i32, Foo")
	{
		Hash_Set<i32> set = hash_set_init<i32>();
		DEFER(hash_set_deinit(set));

		i32 key = 1;
		hash_set_insert(set, key);

		TESTER_CHECK(set.count == 1);
		TESTER_CHECK(hash_set_find(set, key) != nullptr);

		TESTER_CHECK(hash_set_find(set, key) != nullptr);
		TESTER_CHECK(*hash_set_find(set, key) == 1);

		hash_set_insert(set, key);
		TESTER_CHECK(set.count == 1);
		TESTER_CHECK(hash_set_find(set, key) != nullptr);
		TESTER_CHECK(*hash_set_find(set, key) == 1);

		i32 key2   = 2;
		hash_set_insert(set, key2);
		TESTER_CHECK(set.count == 2);

		TESTER_CHECK(hash_set_find(set, key) != nullptr);
		TESTER_CHECK(hash_set_find(set, key2) != nullptr);

		TESTER_CHECK(hash_set_remove(set, key));
		TESTER_CHECK(set.count == 1);
		TESTER_CHECK(hash_set_find(set, key) == nullptr);
		TESTER_CHECK(hash_set_find(set, key2) != nullptr);
		TESTER_CHECK(hash_set_remove(set, key2));
		TESTER_CHECK(set.count == 0);
		TESTER_CHECK(hash_set_find(set, key2) == nullptr);
	}

	// ("reserve")
	{
		Hash_Set<i32> set = hash_set_init<i32>(memory::temp_allocator());

		hash_set_reserve(set, 100);

		TESTER_CHECK(set.count == 0);
		TESTER_CHECK(set.capacity == 128);

		TESTER_CHECK(set.slots.data     != nullptr);
		TESTER_CHECK(set.slots.count    == 128);
		TESTER_CHECK(set.slots.capacity == 128);

		hash_set_reserve(set, 100);

		TESTER_CHECK(set.count == 0);
		TESTER_CHECK(set.capacity == 128);

		TESTER_CHECK(set.slots.data     != nullptr);
		TESTER_CHECK(set.slots.count    == 128);
		TESTER_CHECK(set.slots.capacity == 128);

		for (i32 i = 0; i < 50; ++i)
			hash_set_insert(set, i);

		hash_set_reserve(set, 100);

		TESTER_CHECK(set.count == 50);
		TESTER_CHECK(set.capacity == 256);

		TESTER_CHECK(set.slots.data     != nullptr);
		TESTER_CHECK(set.slots.count    == 256);
		TESTER_CHECK(set.slots.capacity == 256);
	}

	// ("remove")
	{
		// ("unordered")
		{
			Hash_Set<i32> set = {};
			DEFER(hash_set_deinit(set));

			TESTER_CHECK(hash_set_remove(set, 0) == false);
			hash_set_insert(set, 1);
			TESTER_CHECK(hash_set_remove(set, 1) == true);
		}

		// ("ordered")
		{
			Hash_Set<i32> set = {};
			DEFER(hash_set_deinit(set));
			hash_set_insert(set, 1);
			hash_set_insert(set, 2);
			hash_set_insert(set, 3);
			hash_set_insert(set, 4);
			hash_set_insert(set, 5);
			hash_set_insert(set, 6);

			TESTER_CHECK(hash_set_remove_ordered(set, 0) == false);
			TESTER_CHECK(hash_set_remove_ordered(set, 1) == true);
			TESTER_CHECK(hash_set_remove_ordered(set, 3) == true);
			TESTER_CHECK(hash_set_remove_ordered(set, 5) == true);

			TESTER_CHECK(set.entries.count == 3);

			TESTER_CHECK(set.entries[0].key == 2);
			TESTER_CHECK(set.entries[1].key == 4);
			TESTER_CHECK(set.entries[2].key == 6);

			TESTER_CHECK(hash_set_find(set, 1) == nullptr);
			TESTER_CHECK(hash_set_find(set, 3) == nullptr);
			TESTER_CHECK(hash_set_find(set, 5) == nullptr);

			TESTER_CHECK(hash_set_find(set, 2) != nullptr);
			TESTER_CHECK(*hash_set_find(set, 2) == 2);

			TESTER_CHECK(hash_set_find(set, 4) != nullptr);
			TESTER_CHECK(*hash_set_find(set, 4) == 4);

			TESTER_CHECK(hash_set_find(set, 6) != nullptr);
			TESTER_CHECK(*hash_set_find(set, 6) == 6);
		}
	}

	// ("resize")
	{
		Hash_Set<i32> set = hash_set_init<i32>(memory::temp_allocator());

		for (i32 i = 0; i < 100; ++i)
			hash_set_insert(set, i);

		TESTER_CHECK(set.count == 100);
		TESTER_CHECK(set.capacity == 256);

		i32 j = 0;
		for (const auto &entry : set)
			TESTER_CHECK(entry == j++);

		for (i32 i = 0; i < 100; ++i)
		{
			auto entry = hash_set_find(set, i);
			TESTER_CHECK(entry != nullptr);
			TESTER_CHECK(*entry == i);
		}

		for (i32 i = 0; i < 100; ++i)
		{
			TESTER_CHECK(hash_set_remove(set, i) == true);
			TESTER_CHECK(hash_set_find(set, i) == nullptr);
			TESTER_CHECK(set.count == u64(99 - i));
		}

		for (i32 i = 0; i < 100; ++i)
		{
			TESTER_CHECK(hash_set_remove(set, i) == false);
			TESTER_CHECK(hash_set_find(set, i) == nullptr);
		}

		TESTER_CHECK(set.count == 0);
		TESTER_CHECK(set.capacity == 8);
	}

	// ("clear")
	{
		Hash_Set<i32> set = hash_set_init<i32>(memory::temp_allocator());

		for (i32 i = 0; i < 10; ++i)
			hash_set_insert(set, i);

		TESTER_CHECK(set.count == 10);
		TESTER_CHECK(set.capacity == 16);

		i32 j = 0;
		for (const auto &entry : set)
			TESTER_CHECK(entry == j++);

		hash_set_clear(set);

		TESTER_CHECK(set.count == 0);
		TESTER_CHECK(set.capacity == 16);

		for (i32 i = 0; i < 10; ++i)
			TESTER_CHECK(hash_set_find(set, i) == nullptr);
	}

	// ("copy/clone/destroy")
	{
		Hash_Set<i32> set1 = hash_set_init<i32>();
		DEFER(destroy(set1));

		TESTER_CHECK(set1.count    == 0);
		TESTER_CHECK(set1.capacity == 0);

		for (i32 i = 0; i < 10; ++i)
			hash_set_insert(set1, i);

		TESTER_CHECK(set1.count    == 10);
		TESTER_CHECK(set1.capacity == 16);

		auto set2 = hash_set_copy(set1);
		DEFER(destroy(set2));

		TESTER_CHECK(set2.count == 10);
		TESTER_CHECK(set2.capacity == 16);

		i32 j = 0;
		for (const auto &entry : set2)
			TESTER_CHECK(entry == j++);

		auto set3 = hash_set_init<Foo>();
		DEFER(destroy(set3));

		for (i32 i = 0; i < 10; ++i)
			hash_set_insert(set3, Foo{i});

		TESTER_CHECK(set3.count == 10);
		TESTER_CHECK(set3.capacity == 16);

		i32 k = 0;
		for (const auto &entry : set3)
		{
			TESTER_CHECK(entry == Foo{k});
			++k;
		}

		auto set4 = clone(set3);
		DEFER(destroy(set4));

		TESTER_CHECK(set4.count == 10);
		TESTER_CHECK(set4.capacity == 16);

		k = 0;
		for (const auto &entry : set4)
		{
			TESTER_CHECK(entry == Foo{k});
			++k;
		}
	}

	// ("user defined entry [Foo]")
	{
		Hash_Set<Foo> table = hash_set_init<Foo>(memory::temp_allocator());

		TESTER_CHECK(table.count    == 0);
		TESTER_CHECK(table.capacity == 0);

		for (i32 i = 0; i < 10; ++i)
			hash_set_insert(table, Foo{i});

		TESTER_CHECK(table.count == 10);
		TESTER_CHECK(table.capacity == 16);

		i32 j = 0;
		for (const auto &entry : table)
			TESTER_CHECK(entry == Foo{j++});
	}
}

TESTER_TEST("[CONTAINERS]: String Interner")
{
	String_Interner interner = string_interner_init(memory::temp_allocator());
	DEFER(string_interner_deinit(interner));

	const char *s = string_interner_intern(interner, "STRING");
	TESTER_CHECK(s != nullptr);
	TESTER_CHECK(s == string_interner_intern(interner, "STRING"));

	const char *test_string = "This is a test STRING";
	const char *begin = test_string + 15;
	const char *end = begin + 6;
	TESTER_CHECK(s == string_interner_intern(interner, begin, end));
}