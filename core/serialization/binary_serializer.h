#pragma once

#include <core/defines.h>
#include <core/containers/array.h>
#include <core/containers/string.h>
#include <core/containers/hash_table.h>

#include <type_traits>

/*
	TODO:
	- [x] Fundamental types.
	- [x] Pointers.
	- [x] Arrays.
	- [x] Strings.
		- [x] C strings.
	- [x] Hash tables.
	- [x] Structs.
		- [x] Nested structed.
	- [x] Blobs.
		- [x] Need to be serialized as base64 string in json serializer.
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
	- [x] Unit tests.
	- [ ] Return Error on failure.

	- JSON serializer:
		- [ ] Write our own Base64 encoder/decoder.
		- [ ] Should we use JSON_Value instead of string buffer?
		- [x] Current API is error prone by mis-usage from the user.
				serialize(serializer, {
					{"a", a},
					{"b", b}
				});
				is not equal to:
				serialize(serializer, {"a", a});
				serialize(serializer, {"b", b});
*/

struct Bin_Serializer
{
	memory::Allocator *allocator;
	Array<u8> buffer;
	u64 s_offset;
	u64 d_offset;
	bool is_valid;
};

struct Bin_Serialization_Pair
{
	const char *name;
	void *data;
	void (*to)(Bin_Serializer &serializer, const char *name, void *data);
	void (*from)(Bin_Serializer &serializer, const char *name, void *data);

	template <typename T>
	Bin_Serialization_Pair(const char *name, T &data)
	{
		Bin_Serialization_Pair &self = *this;
		self.name = name;
		self.data = (void *)&data;
		self.to = +[](Bin_Serializer &serializer, const char *, void *data) {
			T &d = *(T *)data;
			serialize(serializer, d);
		};
		self.from = +[](Bin_Serializer &serializer, const char *, void *data) {
			T &d = *(T *)data;
			deserialize(serializer, d);
		};
	}
};

inline static Bin_Serializer
bin_serializer_init(memory::Allocator *allocator = memory::heap_allocator())
{
	return Bin_Serializer {
		.allocator = allocator,
		.buffer = array_init<u8>(allocator),
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
template <typename T>
requires (std::is_arithmetic_v<T>)
inline static void
serialize(Bin_Serializer &self, const T &data)
{
	// ASSERT(self.is_valid, "[SERIALIZER][BINARY]: Missing serialization name."); // TODO:
	array_reserve(self.buffer, sizeof(data));
	::memcpy(self.buffer.data + self.s_offset, &data, sizeof(T));
	self.buffer.count += sizeof(T);
	self.s_offset += sizeof(T);
}

template <typename T>
requires (std::is_pointer_v<T> && !std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
inline static void
serialize(Bin_Serializer& self, const T &data)
{
	serialize(self, *data);
}

template <typename T, u64 N>
requires (!std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
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

inline static void
serialize(Bin_Serializer &self, const char *data)
{
	serialize(self, string_literal(data));
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
serialize(Bin_Serializer &self, const Block &block)
{
	serialize(self, block.size);
	array_reserve(self.buffer, block.size);
	::memcpy(self.buffer.data + self.s_offset, block.data, block.size); // TODO: Add array_memcpy().
	self.buffer.count += block.size;
	self.s_offset += block.size;
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
deserialize(Bin_Serializer& self, T &data)
{
	if (data == nullptr)
		data = memory::allocate<std::remove_pointer_t<T>>(self.allocator);
	deserialize(self, *data);
}

template <typename T, u64 N>
requires (!std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
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
	if (self.allocator != data.allocator || data.allocator == nullptr)
	{
		destroy(data);
		data = array_init<T>(self.allocator);
	}

	u64 count = 0;
	deserialize(self, count);
	array_resize(data, count);
	for (u64 i = 0; i < data.count; ++i)
		deserialize(self, data[i]);
}

inline static void
deserialize(Bin_Serializer &self, String &data)
{
	if (self.allocator != data.allocator || data.allocator == nullptr)
	{
		string_deinit(data);
		data = string_init(self.allocator);
	}

	u64 count = 0;
	deserialize(self, count);
	string_resize(data, count);
	for (u64 i = 0; i < data.count; ++i)
		deserialize(self, data[i]);
}

inline static void
deserialize(Bin_Serializer &self, const char *&data)
{
	String out = {};
	deserialize(self, out);
	data = out.data;
	out = {};
}

template <typename K, typename V>
inline static void
deserialize(Bin_Serializer &self, Hash_Table<K, V> &data)
{
	// TODO: Should we add allocator in hash table?
	if (self.allocator != data.entries.allocator || data.entries.allocator == nullptr)
	{
		destroy(data);
		data = hash_table_init<K, V>(self.allocator);
	}

	u64 count = 0;
	deserialize(self, count);
	hash_table_clear(data);
	hash_table_resize(data, count); // TODO: Should we remove this?
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
deserialize(Bin_Serializer &self, Block &block)
{
	deserialize(self, block.size);

	if (block.data == nullptr)
		block.data = (u8 *)memory::allocate(block.size);

	for (u64 i = 0; i < block.size; ++i)
		deserialize(self, ((u8 *)block.data)[i]);
}

inline static void
serialize(Bin_Serializer &self, Bin_Serialization_Pair pair)
{
	self.is_valid = true;
	pair.to(self, pair.name, pair.data);
	self.is_valid = false;
}

inline static void
serialize(Bin_Serializer &self, std::initializer_list<Bin_Serialization_Pair> pairs)
{
	for (const Bin_Serialization_Pair &pair : pairs)
		pair.to(self, pair.name, pair.data);
}

inline static void
deserialize(Bin_Serializer &self, Bin_Serialization_Pair pair)
{
	self.is_valid = true;
	pair.from(self, pair.name, pair.data);
	self.is_valid = false;
}

inline static void
deserialize(Bin_Serializer &self, std::initializer_list<Bin_Serialization_Pair> pairs)
{
	for (const Bin_Serialization_Pair &pair : pairs)
		pair.from(self, pair.name, pair.data);
}