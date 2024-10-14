#include <core/defines.h>
#include <core/json.h>
#include <core/containers/array.h>
#include <core/containers/string.h>
#include <core/containers/hash_table.h>

// TODO: Remove.
#include <string>

/*
	TODO:
	- [x] Fundamental types.
	- [x] Pointers.
	- [x] Arrays.
	- [x] Strings.
	- [x] Hash tables.
	- [x] Structs.
		- [x] Nested structed.
	- [ ] Allocator.
	- [ ] Versioning.
	- [ ] Arena backing memory.
	- [ ] VirtualAlloc?
	- [ ] Collapse serialization and deserialization into one function.
	- [ ] Either we assert that the user should use serialized pairs, or generate names for omitted types.
	- [ ] What happens if the user serializes multiple entries with the same name in jsn and name dependent serializers.
*/

template <typename S>
struct Serialization_Pair
{
	const char *name;
	void *data;
	void (*to)(S &serializer, const char *name, void *data);
	void (*from)(S &serializer, const char *name, void *data);

	template <typename T>
	Serialization_Pair(const char *name, T &data)
	{
		Serialization_Pair &self = *this;
		self.name = name;
		self.data = (void *)&data;
		self.to = +[](S &serializer, const char *, void *data) {
			T &d = *(T *)data;
			serialize(serializer, d);
		};
		self.from = +[](S &serializer, const char *, void *data) {
			T &d = *(T *)data;
			deserialize(serializer, d);
		};
	}
};

struct Bin_Serializer
{
	Array<u8> buffer;
	u64 s_offset;
	u64 d_offset;
	bool is_valid;
};

inline static Bin_Serializer
bin_serializer_init()
{
	return Bin_Serializer {
		.buffer = array_init<u8>(),
		.s_offset = 0,
		.d_offset = 0,
		.is_valid = false
	};
}

inline static void
bin_serializer_deinit(Bin_Serializer &self)
{
	array_deinit(self.buffer);
	self = {};
}

/////////////////////////////////////////////////////////////////////
inline static void
_bin_serializer_push(Bin_Serializer &self, const u8 *data, u64 data_size)
{
	// ASSERT(self.is_valid, "[SERIALIZER][BINARY]: Missing serialization name."); // TODO:
	for (u64 i = 0; i < data_size; ++i)
		array_push(self.buffer, data[i]);
	self.s_offset += data_size;
}

template <typename T>
requires (std::is_arithmetic_v<T>)
inline static void
serialize(Bin_Serializer &self, const T &data)
{
	_bin_serializer_push(self, (const u8 *)&data, sizeof(data));
}

template <typename T>
requires (std::is_pointer_v<T> && !std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
inline static void
serialize(Bin_Serializer& self, const T &data)
{
	serialize(self, *data);
}

template <typename T, u64 N>
inline static void
serialize(Bin_Serializer &self, const T (&data)[N])
{
	serialize(self, N);
	for (u64 i = 0; i < N; ++i)
		serialize(self, data[i]);
}

template <typename T>
inline static void
serialize(Bin_Serializer &self, const Array<T> &data)
{
	serialize(self, data.count);
	for (u64 i = 0; i < data.count; ++i)
		serialize(self, data[i]);
}

inline static void
serialize(Bin_Serializer &self, const String &data)
{
	serialize(self, data.count);
	for (u64 i = 0; i < data.count; ++i)
		serialize(self, data[i]);
}

template <typename K, typename V>
inline static void
serialize(Bin_Serializer &self, const Hash_Table<K, V> &data)
{
	serialize(self, data.count);
	for (const Hash_Table_Entry<const K, V> &entry : data)
	{
		serialize(self, entry.key);
		serialize(self, entry.value);
	}
}

inline static void
serialize(Bin_Serializer &self, Serialization_Pair<Bin_Serializer> pair)
{
	self.is_valid = true;
	pair.to(self, pair.name, pair.data);
	self.is_valid = false;
}

inline static void
serialize(Bin_Serializer &self, std::initializer_list<Serialization_Pair<Bin_Serializer>> pairs)
{
	self.is_valid = true;
	for (const Serialization_Pair<Bin_Serializer> &pair : pairs)
		pair.to(self, pair.name, (void *&)pair.data);
	self.is_valid = false;
}

/////////////////////////////////////////////////////////////////////
template <typename T>
requires (std::is_arithmetic_v<T>)
inline static void
deserialize(Bin_Serializer &self, T &data)
{
	u8 *d = (u8 *)&data;
	u64 data_size = sizeof(data);

	ASSERT(self.d_offset + data_size <= self.buffer.count, "[SERIALIZER][BINARY]: Trying to deserialize beyond buffer capacity.");
	for (u64 i = 0; i < data_size; ++i)
		d[i] = self.buffer[i + self.d_offset];
	self.d_offset += data_size;
}

template <typename T>
requires (std::is_pointer_v<T> && !std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
inline static void
deserialize(Bin_Serializer& self, const T &data)
{
	deserialize(self, *data);
}

template <typename T, u64 N>
inline static void
deserialize(Bin_Serializer &self, T (&data)[N])
{
	u64 count = 0;
	deserialize(self, count);
	ASSERT(count == N, "[SERIALIZER][BINARY]: Passed array count does not match the deserialized count.");
	for (u64 i = 0; i < count; ++i)
		deserialize(self, data[i]);
}

template <typename T>
inline static void
deserialize(Bin_Serializer &self, Array<T> &data)
{
	u64 count = 0;
	deserialize(self, count);
	array_resize(data, count);
	for (u64 i = 0; i < data.count; ++i)
		deserialize(self, data[i]);
}

inline static void
deserialize(Bin_Serializer &self, String &data)
{
	u64 count = 0;
	deserialize(self, count);
	string_resize(data, count);
	for (u64 i = 0; i < data.count; ++i)
		deserialize(self, data[i]);
}

template <typename K, typename V>
inline static void
deserialize(Bin_Serializer &self, Hash_Table<K, V> &data)
{
	u64 count = 0;
	deserialize(self, count);
	hash_table_clear(data);
	hash_table_resize(data, count);
	for (u64 i = 0; i < count; ++i)
	{
		K key   = {};
		V value = {};
		deserialize(self, key);
		deserialize(self, value);
		hash_table_insert(data, key, value);
	}
}

inline static void
deserialize(Bin_Serializer &self, Serialization_Pair<Bin_Serializer> pair)
{
	pair.from(self, pair.name, pair.data);
}

inline static void
deserialize(Bin_Serializer &self, std::initializer_list<Serialization_Pair<Bin_Serializer>> pairs)
{
	for (const Serialization_Pair<Bin_Serializer> &pair : pairs)
		pair.from(self, pair.name, (void *&)pair.data);
}

/////////////////////////////////////////////////////////////////////
struct Jsn_Serializer
{
	String buffer;
	JSON_Value value;
	Array<JSON_Value> values;
	// u64 s_offset;
	// u64 d_offset;
	bool is_valid;
};

inline static Jsn_Serializer
jsn_serializer_init()
{
	return Jsn_Serializer {
		.buffer = string_init(),
		.value = {},
		.values = array_init<JSON_Value>(),
		.is_valid = false
	};
}

inline static void
jsn_serializer_deinit(Jsn_Serializer &self)
{
	string_deinit(self.buffer);
	json_value_deinit(self.value);
	array_deinit(self.values);
	self = {};
}

/////////////////////////////////////////////////////////////////////
template <typename T>
requires (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
inline static void
serialize(Jsn_Serializer &self, const T &data)
{
	string_append(self.buffer, std::to_string(data).c_str());
}

inline static void
serialize(Jsn_Serializer &self, const bool data)
{
	string_append(self.buffer, data ? "true" : "false");
}

template <typename T>
requires (std::is_pointer_v<T> && !std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
inline static void
serialize(Jsn_Serializer& self, const T &data)
{
	serialize(self, *data);
}

template <typename T, u64 N>
inline static void
serialize(Jsn_Serializer &self, const T (&data)[N])
{
	string_append(self.buffer, "[");
	for (u64 i = 0; i < N; ++i)
	{
		if (i > 0)
			string_append(self.buffer, ", ");
		serialize(self, data[i]);
	}
	string_append(self.buffer, "]");
}

template <typename T>
inline static void
serialize(Jsn_Serializer &self, const Array<T> &data)
{
	string_append(self.buffer, "[");
	for (u64 i = 0; i < data.count; ++i)
	{
		if (i > 0)
			string_append(self.buffer, ", ");
		serialize(self, data[i]);
	}
	string_append(self.buffer, "]");
}

inline static void
serialize(Jsn_Serializer &self, const String &data)
{
	string_append(self.buffer, "\"{}\"", data);
}

template <typename K, typename V>
inline static void
serialize(Jsn_Serializer &self, const Hash_Table<K, V> &data)
{
	string_append(self.buffer, "[");
	i32 i = 0;
	for (const Hash_Table_Entry<const K, V> &entry : data)
	{
		if (i > 0)
			string_append(self.buffer, ", ");
		string_append(self.buffer, "{{ \"key\": ");
		serialize(self, entry.key);
		string_append(self.buffer, ", \"value\": ");
		serialize(self, entry.value);
		string_append(self.buffer, "}}");
		++i;
	}
	string_append(self.buffer, "]");
}

inline static void
serialize(Jsn_Serializer &self, Serialization_Pair<Jsn_Serializer> pair)
{
	self.is_valid = true;
	if (self.buffer.count > 0)
		string_append(self.buffer, ", ");
	string_append(self.buffer, "\"{}\": ", pair.name);
	pair.to(self, pair.name, pair.data);
	self.is_valid = false;
}

inline static void
serialize(Jsn_Serializer &self, std::initializer_list<Serialization_Pair<Jsn_Serializer>> pairs)
{
	string_append(self.buffer, "{{");
	i32 i = 0;
	for (const Serialization_Pair<Jsn_Serializer> &pair : pairs)
	{
		if (i > 0)
			string_append(self.buffer, ", ");
		string_append(self.buffer, "\"{}\": ", pair.name);
		pair.to(self, pair.name, (void *&)pair.data);
		++i;
	}
	string_append(self.buffer, "}}");
}

/////////////////////////////////////////////////////////////////////
// TODO: Get rid of templates and do overload myself.
template <typename T>
requires (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
inline static void
deserialize(Jsn_Serializer &self, T &data)
{
	ASSERT(array_last(self.values).kind == JSON_VALUE_KIND_NUMBER, "[SERIALIZER][JSON]: JSON value is not a number!");
	*(std::remove_const_t<T> *)&data = (std::remove_const_t<T>)array_last(self.values).as_number;
}

inline static void
deserialize(Jsn_Serializer &self, bool &data)
{
	ASSERT(array_last(self.values).kind == JSON_VALUE_KIND_BOOL, "[SERIALIZER][JSON]: JSON value is not bool!");
	data = array_last(self.values).as_bool;
}

template <typename T>
requires (std::is_pointer_v<T> && !std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
inline static void
deserialize(Jsn_Serializer& self, const T &data)
{
	deserialize(self, *data);
}

template <typename T, u64 N>
inline static void
deserialize(Jsn_Serializer &self, T (&data)[N])
{
	ASSERT(array_last(self.values).kind == JSON_VALUE_KIND_ARRAY, "[SERIALIZER][JSON]: JSON value is not an array!");
	Array<JSON_Value> array_values = array_last(self.values).as_array;
	ASSERT(array_values.count == N, "[SERIALIZER][JSON]: Passed array count does not match the deserialized count.");
	for (u64 i = 0; i < array_values.count; ++i)
	{
		array_push(self.values, array_values[i]);
		deserialize(self, data[i]);
		array_pop(self.values);
	}
}

template <typename T>
inline static void
deserialize(Jsn_Serializer &self, Array<T> &data)
{
	ASSERT(array_last(self.values).kind == JSON_VALUE_KIND_ARRAY, "[SERIALIZER][JSON]: JSON value is not an array!");
	Array<JSON_Value> array_values = array_last(self.values).as_array;
	array_resize(data, array_values.count);
	for (u64 i = 0; i < data.count; ++i)
	{
		array_push(self.values, array_values[i]);
		deserialize(self, data[i]);
		array_pop(self.values);
	}
}

inline static void
deserialize(Jsn_Serializer &self, String &data)
{
	ASSERT(array_last(self.values).kind == JSON_VALUE_KIND_STRING, "[SERIALIZER][JSON]: JSON value is not a string!");
	String str = array_last(self.values).as_string;
	string_clear(data);
	string_append(data, str);
}

template <typename K, typename V>
inline static void
deserialize(Jsn_Serializer &self, Hash_Table<K, V> &data)
{
	ASSERT(array_last(self.values).kind == JSON_VALUE_KIND_ARRAY, "[SERIALIZER][JSON]: JSON value is not an array!");
	Array<JSON_Value> array_values = array_last(self.values).as_array;

	hash_table_clear(data);
	hash_table_resize(data, array_values.count);
	for (u64 i = 0; i < array_values.count; ++i)
	{
		JSON_Value json_value = array_values[i];
		ASSERT(json_value.kind == JSON_VALUE_KIND_OBJECT, "[SERIALIZER][JSON]: JSON value is not an object!");

		auto *key_json_entry = hash_table_find(json_value.as_object, string_literal("key"));
		array_push(self.values, key_json_entry->value);
		K key   = {};
		deserialize(self, key);
		array_pop(self.values);

		auto *value_json_entry = hash_table_find(json_value.as_object, string_literal("value"));
		array_push(self.values, value_json_entry->value);
		V value = {};
		deserialize(self, value);
		array_pop(self.values);

		hash_table_insert(data, key, value);
	}
}

inline static void
deserialize(Jsn_Serializer &self, Serialization_Pair<Jsn_Serializer> pair)
{
	// TODO: Print error.
	// TODO: Clear messages.
	// TODO: Better naming.
	auto [value, error] = json_value_from_string(self.buffer);
	self.value = value;
	ASSERT(!error, "[SERIALIZER][JSON]: Could not parse JSON string.");

	auto *json_entry = hash_table_find(value.as_object, string_literal(pair.name));
	ASSERT(json_entry, "[SERIALIZER][JSON]: Could not find JSON value with provided name.");

	array_push(self.values, json_entry->value);
	pair.from(self, pair.name, pair.data);
	array_pop(self.values);
}

inline static void
deserialize(Jsn_Serializer &self, std::initializer_list<Serialization_Pair<Jsn_Serializer>> pairs)
{
	for (const Serialization_Pair<Jsn_Serializer> &pair : pairs)
	{
		auto *json_entry = hash_table_find(array_last(self.values).as_object, string_literal(pair.name));
		ASSERT(json_entry, "[SERIALIZER][JSON]: Could not find JSON value with provided name.");

		array_push(self.values, json_entry->value);
		pair.from(self, pair.name, pair.data);
		array_pop(self.values);
	}
}

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

	i32 ii = 0;
	Test t2 = {};
	t2.f = array_init<i32>(memory::temp_allocator()); // TODO: Should we init first or let serializer do it for us?
	t2.g = string_init(memory::temp_allocator()); // TODO: Should we init first or let serializer do it for us?
	t2.h = hash_table_init<i32, String>(memory::temp_allocator()); // TODO: Should we init first or let serializer do it for us?
	t2.i = &ii;
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
	t22.b.f = array_init<i32>(memory::temp_allocator()); // TODO: Should we init first or let serializer do it for us?
	t22.b.g = string_init(memory::temp_allocator()); // TODO: Should we init first or let serializer do it for us?
	t22.b.h = hash_table_init<i32, String>(memory::temp_allocator()); // TODO: Should we init first or let serializer do it for us?
	t22.b.i = &ii;

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