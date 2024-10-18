#pragma once

#include <core/defines.h>
#include <core/containers/array.h>
#include <core/containers/string.h>
#include <core/containers/hash_table.h>

#include <type_traits>

struct Bin_Serializer
{
	memory::Allocator *allocator;
	Array<u8> buffer;
	u64 s_offset;
	u64 d_offset;
	bool is_valid;
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
	// TODO: memcpy.
	const u8 *d = (const u8 *)&data;
	for (u64 i = 0; i < sizeof(data); ++i)
		array_push(self.buffer, d[i]);
	self.s_offset += sizeof(data);
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

struct Serialization_Pair
{
	const char *name;
	void *data;
	void (*to)(Bin_Serializer &serializer, const char *name, void *data);
	void (*from)(Bin_Serializer &serializer, const char *name, void *data);

	template <typename T>
	Serialization_Pair(const char *name, T &data)
	{
		Serialization_Pair &self = *this;
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

inline static void
serialize(Bin_Serializer &self, Serialization_Pair pair)
{
	self.is_valid = true;
	pair.to(self, pair.name, pair.data);
	self.is_valid = false;
}

inline static void
serialize(Bin_Serializer &self, std::initializer_list<Serialization_Pair> pairs)
{
	self.is_valid = true;
	for (const Serialization_Pair &pair : pairs)
		pair.to(self, pair.name, (void *&)pair.data);
	self.is_valid = false;
}

inline static void
deserialize(Bin_Serializer &self, Serialization_Pair pair)
{
	pair.from(self, pair.name, pair.data);
}

inline static void
deserialize(Bin_Serializer &self, std::initializer_list<Serialization_Pair> pairs)
{
	for (const Serialization_Pair &pair : pairs)
		pair.from(self, pair.name, (void *&)pair.data);
}