#include "bin_serializer.h"
#include "jsn_serializer.h"

/*
	TODO:
	- [x] Fundamental types.
	- [x] Pointers.
	- [x] Arrays.
	- [x] Strings.
		- [ ] C strings.
	- [x] Hash tables.
	- [x] Structs.
		- [x] Nested structed.
	- [ ] Blobs.
		- [ ] Need to be serialized as base64 string in json serializer.
	- [x] Allocator.
		- [x] Binary serializer.
		- [x] Json serializer.
	- [ ] Versioning.
	- [ ] Arena backing memory.
	- [ ] VirtualAlloc?
	- [ ] Collapse serialization and deserialization into one function.
		- [ ] This would require splitting the serializer into a reader and writer instead of one object.
				More maintenance on us but more user friendly.
	- [ ] Either we assert that the user should use serialized pairs, or generate names for omitted types.
		- [ ] What happens if the user used pairs in serialization but forgot to use it in deserialization.
	- [ ] What happens if the user serializes multiple entries with the same name in jsn and name dependent serializers.
		- [ ] Should we assert?
		- [ ] Should we print warning messages?
		- [ ] Should we override data?
	- [ ] Unit tests.

	- JSON serializer:
		- [ ] Should use JSON_Value instead of string buffer?
*/


/////////////////////////////////////////////////////////////////////
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

	::printf("%s", jsn.buffer.data);

	Test t2 = {};
	deserialize(jsn, {"t1", t2});

	Test2 t22 = {};
	deserialize(jsn, {"t21", t22});

	[[maybe_unused]] i32 xxx = 0;
}