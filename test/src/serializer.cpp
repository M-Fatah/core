#include <core/defines.h>
#include <core/json.h>
#include <core/containers/array.h>
#include <core/containers/string.h>
#include <core/containers/hash_table.h>

#include "bin_serializer.h"
#include "jsn_serializer.h"

/*
	TODO:
	- [x] Fundamental types.
	- [x] Pointers.
	- [x] Arrays.
	- [x] Strings.
	- [x] Hash tables.
	- [x] Structs.
		- [x] Nested structed.
	- [ ] Blobs.
		- [ ] Need to be serialized as base64 string in json serializer.
	- [ ] Allocator.
		- [x] Binary serializer.
		- [ ] Json serializer.
	- [ ] Versioning.
	- [ ] Arena backing memory.
	- [ ] VirtualAlloc?
	- [ ] Collapse serialization and deserialization into one function.
	- [ ] Either we assert that the user should use serialized pairs, or generate names for omitted types.
	- [ ] What happens if the user serializes multiple entries with the same name in jsn and name dependent serializers.
*/


/////////////////////////////////////////////////////////////////////
inline static void
binary_serialization_test_fundamentals()
{
	Bin_Serializer bin = bin_serializer_init();
	DEFER(bin_serializer_deinit(bin));

	i8   a1 = 1;
	i16  b1 = 2;
	i32  c1 = 3;
	i64  d1 = 4;
	serialize(bin, a1);
	serialize(bin, b1);
	serialize(bin, c1);
	serialize(bin, d1);

	u8   e1 = 5;
	u16  f1 = 6;
	u32  g1 = 7;
	u64  h1 = 8;
	serialize(bin, e1);
	serialize(bin, f1);
	serialize(bin, g1);
	serialize(bin, h1);

	f32  i1 = 9;
	f64  j1 = 10;
	serialize(bin, i1);
	serialize(bin, j1);

	char k1 = 'A';
	bool l1 = true;
	serialize(bin, k1);
	serialize(bin, l1);

	i8   a2 = 0;
	i16  b2 = 0;
	i32  c2 = 0;
	i64  d2 = 0;
	deserialize(bin, a2);
	deserialize(bin, b2);
	deserialize(bin, c2);
	deserialize(bin, d2);

	u8   e2 = 0;
	u16  f2 = 0;
	u32  g2 = 0;
	u64  h2 = 0;
	deserialize(bin, e2);
	deserialize(bin, f2);
	deserialize(bin, g2);
	deserialize(bin, h2);

	f32  i2 = 0;
	f64  j2 = 0;
	deserialize(bin, i2);
	deserialize(bin, j2);

	char k2 = 0;
	bool l2 = 0;
	deserialize(bin, k2);
	deserialize(bin, l2);
}

inline static void
binary_serialization_test_arrays()
{
	Bin_Serializer bin = bin_serializer_init();
	DEFER(bin_serializer_deinit(bin));

	i32 a1[3] = {1, 2, 3};

	serialize(bin, a1);

	Array<i8> b1 = array_from<i8>({1, 2, 3, 4, 5}, memory::temp_allocator());

	serialize(bin, b1);

	i32 a2[3] = {};

	deserialize(bin, a2);

	Array<i8> b2 = array_init<i8 >(memory::temp_allocator());

	deserialize(bin, b2);
}

inline static void
binary_serialization_test_strings()
{
	Bin_Serializer bin = bin_serializer_init();
	DEFER(bin_serializer_deinit(bin));

	String a1 = string_from("Hello, World!", memory::temp_allocator());

	serialize(bin, a1);

	String a2 = string_init(memory::temp_allocator());

	deserialize(bin, a2);
}

inline static void
binary_serialization_test_hash_tables()
{
	Bin_Serializer bin = bin_serializer_init();
	DEFER(bin_serializer_deinit(bin));

	Hash_Table<i32, String> a1 = hash_table_from<i32, String>(
		{
			{1, string_from("A", memory::temp_allocator())},
			{2, string_from("B", memory::temp_allocator())},
			{3, string_from("C", memory::temp_allocator())},
		},
		memory::temp_allocator()
	);

	serialize(bin, a1);

	Hash_Table<i32, String> a2 = hash_table_init<i32, String>(memory::temp_allocator());

	deserialize(bin, a2);
}

struct Test
{
	i32 a;
	f32 b;
	char c;
	i32 d[3];
	bool e;
	Array<i32> f;
	String g;
	Hash_Table<i32, String> h;
	i32 *i;
};

template <typename S>
inline static void
serialize(S &self, Test &data) // TODO: Removing const here solves all the compilation errors for ADL.
{
	serialize(self, {
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

template <typename S>
inline static void
deserialize(S &self, Test &data)
{
	deserialize(self, {
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

struct Test2
{
	i32 a;
	Test b;
};

template <typename S>
inline static void
serialize(S &self, Test2 &data) // TODO: Removing const here solves all the compilation errors for ADL.
{
	serialize(self, {
		{"a", data.a},
		{"b", data.b}
	});
}

template <typename S>
inline static void
deserialize(S &self, Test2 &data)
{
	deserialize(self, {
		{"a", data.a},
		{"b", data.b}
	});
}

inline static void
binary_serialization_test_structs()
{
	Bin_Serializer bin = bin_serializer_init();
	DEFER(bin_serializer_deinit(bin));

	i32 i = 5;

	Test t1 = {
		.a = 1,
		.b = 1.5f,
		.c = 'C',
		.d = {1, 2, 3},
		.e = true,
		.f = array_from<i32>({1, 2, 3}, memory::temp_allocator()),
		.g = string_from(memory::temp_allocator(), "bitch"),
		.h = hash_table_from<i32, String>({{1, string_literal("one")}, {2, string_literal("two")}, {3, string_literal("three")}}),
		.i = &i
	};

	Test t2 = {};
	// serialize(bin, t1);
	// deserialize(bin, t2);

	serialize(bin, {"t1", t1});

	deserialize(bin, {"t1", t2});

	Test2 t21 = {
		.a = 1,
		.b = t1
	};

	serialize(bin, {"t21", t21});

	Test2 t22 = {};

	deserialize(bin, {"t21", t22});

	[[maybe_unused]] i32 xxx = 0;
}

inline static void
json_serialization_test_structs()
{
	Jsn_Serializer jsn = jsn_serializer_init();
	DEFER(jsn_serializer_deinit(jsn));

	i32 i = 5;

	Test t1 = {
		.a = 1,
		.b = 1.5f,
		.c = 'C',
		.d = {1, 2, 3},
		.e = true,
		.f = array_from<i32>({1, 2, 3}, memory::temp_allocator()),
		.g = string_from(memory::temp_allocator(), "bitch"),
		.h = hash_table_from<i32, String>({{1, string_literal("one")}, {2, string_literal("two")}, {3, string_literal("three")}}),
		.i = &i
	};

	serialize(jsn, {"t1", t1});

	Test2 t21 = {
		.a = 1,
		.b = t1
	};

	serialize(jsn, {"t21", t21});

	// TODO: Just for ba3basa.
	String ttt = string_from(memory::temp_allocator(), "{{ {} }}", jsn.buffer);
	string_clear(jsn.buffer);
	string_append(jsn.buffer, ttt);
	::printf("%s", jsn.buffer.data);

	i32 ii = 0;
	Test t2 = {};
	t2.f = array_init<i32>(memory::temp_allocator()); // TODO: Should we init first or let serializer do it for us?
	t2.g = string_init(memory::temp_allocator()); // TODO: Should we init first or let serializer do it for us?
	t2.h = hash_table_init<i32, String>(memory::temp_allocator()); // TODO: Should we init first or let serializer do it for us?
	t2.i = &i; // TODO: Should we init first or let serializer do it for us?

	Test2 t22 = {};
	t22.b.f = array_init<i32>(memory::temp_allocator()); // TODO: Should we init first or let serializer do it for us?
	t22.b.g = string_init(memory::temp_allocator()); // TODO: Should we init first or let serializer do it for us?
	t22.b.h = hash_table_init<i32, String>(memory::temp_allocator()); // TODO: Should we init first or let serializer do it for us?
	t22.b.i = &ii; // TODO: Should we init first or let serializer do it for us?

	deserialize(jsn, {"t1", t2});
	deserialize(jsn, {"t21", t22});

	[[maybe_unused]] i32 xxx = 0;
}