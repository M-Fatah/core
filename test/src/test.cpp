#include <core/defines.h>
#include <core/logger.h>
#include <core/reflect.h>
#include <core/containers/string.h>
#include <core/containers/hash_table.h>
#include <core/platform/platform.h>

#include "serialization.cpp"

/*
	TODO:
	- [x] Serialize binary blobs.
		- [-] Use annotation tags.
		- [x] Define Blob struct for the user to use it.
	- [x] Array of arrays.
	- [x] Array elements.
	- [ ] Overload for custom serialization.
*/

template <typename S, typename T>
inline static void
to(S &serializer, Array<T> &data)
{
	to(serializer, {
		{"count", data.count},
		{"data", data.data, data.count}
	});
}

template <typename S, typename K, typename V>
inline static void
to(S &serializer, Hash_Table_Entry<K, V> &data)
{
	to(serializer, {
		{"key", data.key},
		{"value", data.value}
	});
}

template <typename S, typename K, typename V>
inline static void
to(S &serializer, Hash_Table<K, V> &data)
{
	to(serializer, {
		{"count", data.count},
		{"entries", data.entries}
	});
}

template <typename S>
inline static void
to(S &serializer, String &data)
{
	to(serializer, data.data);
}

struct Foo1
{
	char a;
	bool b;
	const char *c[2];
	i32 *d;
	Blob e;
	Array<i32> f;
	String g;
	Array<Array<i32>> h;
	Hash_Table<i32, String> i;
};

TYPE_OF(Foo1, a, b, c, d, e, f, g, h, i);

struct Foo2
{
	char a;
	bool b;
	const char *c[2];
	i32 *d;
	Blob e;
	Array<i32> f;
	String g;
	Array<Array<i32>> h;
	Hash_Table<i32, String> i;
};

template <typename S>
inline static void
to(S &serializer, Foo2 &data)
{
	to(serializer, {
		{"a", data.a},
		{"b", data.b},
		{"c", data.c},
		{"d", data.d},
		{"e", data.e},
		{"f", data.f},
		{"g", data.g},
		{"h", data.h},
		{"i", data.i}
	});
}

i32
main()
{
	char a = 'A';
	bool b = true;

	i32 d = 7;
	Array<i32> f = array_from({1, 2, 3}, memory::temp_allocator());
	String g = string_literal("Shit");
	Array<Array<i32>> h = array_from({f}, memory::temp_allocator());
	Hash_Table<i32, String> i = hash_table_from<i32, String>({{1, string_literal("1")}}, memory::temp_allocator());

#if 0
	{
		Foo1 f1 = {a, b, {"Hello", "World"}, &d, {(u8 *)&d, 1}, f, g, h, i};
		print(value_of(f1));
		::printf("\n");
	}
#else
	{
		Foo1 f1 = {a, b, {"Hello", "World"}, &d, {(u8 *)&d, 1}, f, g, h, i};
		Foo2 f2 = {a, b, {"Hello", "World"}, &d, {(u8 *)&d, 1}, f, g, h, i};

		i32 array1[] = {1, 2, 3};
		Array<i32> array2 = array_from<i32>({1, 2, 3}, memory::temp_allocator());

		// NOTE: Binary.
		{
			Bin_Serializer bin = {};

			// Struct.
			// to(bin, {"a", a});
			// to(bin, {"b", b});
			// to(bin, a);
			// to(bin, b);

			// Array.
			// to(bin, {"array1", array1}); // Generic (using reflection).
			// to(bin, {"array2", array2}); // Custom overload.

			// Struct.
			// to(bin, f1);                 // TODO: Find a way to prevent this usage. // NOTE: Current workaround is to auto generate a name.
			// to(bin, f2);                 // TODO: Find a way to prevent this usage. // NOTE: Current workaround is to auto generate a name.
			// to(bin, f);                  // TODO: Find a way to prevent this usage. // NOTE: Current workaround is to auto generate a name.
			// to(bin, g);                  // TODO: Find a way to prevent this usage. // NOTE: Current workaround is to auto generate a name.
			// to(bin, h);                  // TODO: Find a way to prevent this usage. // NOTE: Current workaround is to auto generate a name.
			// to(bin, i);                  // TODO: Find a way to prevent this usage. // NOTE: Current workaround is to auto generate a name.
			to(bin, {"f1", f1});         // Generic (using reflection).
			to(bin, {"f2", f2});         // Custom overload.

			::printf("%s\n", bin.buffer.data);
		}

		// NOTE: JSON.
		{
			// ::printf("JSON:\n");
			// Jsn_Serializer jsn = {};
			// to(jsn, f1);
			// to(jsn, {"f1", f1});
			// to(jsn, i);
			// to(jsn, {"i", i});
		}

		[[maybe_unused]] int x = 0;
	}
#endif


	// Jsn_Serializer jsn = {};
	// to(jsn, {"f1", f1});

	return 0;
}