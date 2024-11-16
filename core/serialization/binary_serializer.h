#pragma once

#include <core/defines.h>
#include <core/containers/array.h>
#include <core/containers/string.h>
#include <core/containers/hash_table.h>

#include <type_traits>

/*
	TODO:
	- [ ] Versioning.
	- [ ] Arena backing memory.
	- [ ] Either we assert that the user should use serialized pairs, or generate names for omitted types.
	- [ ] What happens if the user serializes multiple entries with the same name in jsn and name dependent serializers.
		- [ ] Should we assert?
		- [ ] Should we print warning messages?
		- [ ] Should we override data?
	- [ ] deserializer_init() should take a block or a span or a view.
	- [ ] Return Error on failure.
	- [ ] Cleanup.

	- JSON serializer:
		- [ ] Should we use JSON_Value instead of string buffer?
*/

struct Binary_Serializer
{
	memory::Allocator *allocator;
	Array<u8> buffer;
	u64 offset;
};

struct Binary_Deserializer
{
	memory::Allocator *allocator;
	Array<u8> buffer;
	u64 offset;
};

struct Binary_Serialization_Pair
{
	const char *name;
	void *data;
	void (*to)(Binary_Serializer &serializer, const char *name, void *data);
	void (*from)(Binary_Deserializer &deserializer, const char *name, void *data);

	template <typename T>
	Binary_Serialization_Pair(const char *name, T &data)
	{
		Binary_Serialization_Pair &self = *this;
		self.name = name;
		self.data = (void *)&data;
		self.to = +[](Binary_Serializer &serializer, const char *, void *data) {
			T &d = *(T *)data;
			serialize(serializer, d);
		};
		self.from = +[](Binary_Deserializer &deserializer, const char *, void *data) {
			T &d = *(T *)data;
			serialize(deserializer, d);
		};
	}
};

inline static Binary_Serializer
binary_serializer_init(memory::Allocator *allocator = memory::heap_allocator())
{
	return Binary_Serializer {
		.allocator = allocator,
		.buffer = array_init<u8>(allocator),
		.offset = 0
	};
}

inline static void
binary_serializer_deinit(Binary_Serializer &self)
{
	array_deinit(self.buffer);
	self = {};
}

template <typename T>
requires (std::is_arithmetic_v<T>)
inline static void
serialize(Binary_Serializer &self, const T &data)
{
	array_reserve(self.buffer, sizeof(data));
	::memcpy(self.buffer.data + self.offset, &data, sizeof(T));
	self.buffer.count += sizeof(T);
	self.offset += sizeof(T);
}

template <typename T>
requires (std::is_pointer_v<T> && !std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
inline static void
serialize(Binary_Serializer& self, const T &data)
{
	serialize(self, *data);
}

template <typename T, u64 N>
requires (!std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
inline static void
serialize(Binary_Serializer &self, const T (&data)[N])
{
	serialize(self, N);
	for (u64 i = 0; i < N; ++i)
		serialize(self, data[i]);
}

template <typename T>
inline static void
serialize(Binary_Serializer &self, const Array<T> &data)
{
	serialize(self, data.count);
	for (u64 i = 0; i < data.count; ++i)
		serialize(self, data[i]);
}

inline static void
serialize(Binary_Serializer &self, const String &data)
{
	serialize(self, data.count);
	for (u64 i = 0; i < data.count; ++i)
		serialize(self, data[i]);
}

inline static void
serialize(Binary_Serializer &self, const char *&data)
{
	serialize(self, string_literal(data));
}

template <typename K, typename V>
inline static void
serialize(Binary_Serializer &self, const Hash_Table<K, V> &data)
{
	serialize(self, data.count);
	for (const Hash_Table_Entry<const K, V> &entry : data)
	{
		serialize(self, entry.key);
		serialize(self, entry.value);
	}
}

inline static void
serialize(Binary_Serializer &self, const Block &block)
{
	serialize(self, block.size);
	array_reserve(self.buffer, block.size);
	::memcpy(self.buffer.data + self.offset, block.data, block.size); // TODO: Add array_memcpy().
	self.buffer.count += block.size;
	self.offset += block.size;
}

inline static void
serialize(Binary_Serializer &self, Binary_Serialization_Pair pair)
{
	pair.to(self, pair.name, pair.data);
}

inline static void
serialize(Binary_Serializer &self, std::initializer_list<Binary_Serialization_Pair> pairs)
{
	for (const Binary_Serialization_Pair &pair : pairs)
		pair.to(self, pair.name, pair.data);
}

inline static Binary_Deserializer
binary_deserializer_init(const Array<u8> &buffer, memory::Allocator *allocator = memory::heap_allocator())
{
	return Binary_Deserializer {
		.allocator = allocator,
		.buffer = buffer,
		.offset = 0
	};
}

inline static void
binary_deserializer_deinit(Binary_Deserializer &self)
{
	self = {};
}

template <typename T>
requires (std::is_arithmetic_v<T>)
inline static void
serialize(Binary_Deserializer &self, const T &data)
{
	u8 *d = (u8 *)&data;
	u64 data_size = sizeof(data);

	ASSERT(self.offset + data_size <= self.buffer.count, "[SERIALIZER][BINARY]: Trying to deserialize beyond buffer capacity.");
	for (u64 i = 0; i < data_size; ++i)
		d[i] = self.buffer[i + self.offset];
	self.offset += data_size;
}

template <typename T>
requires (std::is_pointer_v<T> && !std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
inline static void
serialize(Binary_Deserializer& self, const T &data)
{
	T &d = (T &)data;

	if (d == nullptr)
		d = memory::allocate<std::remove_pointer_t<T>>(self.allocator);

	serialize(self, *d);
}

template <typename T, u64 N>
requires (!std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
inline static void
serialize(Binary_Deserializer &self, const T (&data)[N])
{
	u64 count = 0;
	serialize(self, count);
	ASSERT(count == N, "[SERIALIZER][BINARY]: Passed array count does not match the deserialized count.");
	for (u64 i = 0; i < count; ++i)
		serialize(self, data[i]);
}

inline static void
serialize(Binary_Deserializer &self, const Block &block)
{
	Block &d = (Block &)block;

	serialize(self, d.size);
	if (d.data == nullptr)
		d.data = (u8 *)memory::allocate(d.size);

	for (u64 i = 0; i < d.size; ++i)
		serialize(self, ((u8 *)d.data)[i]);
}

template <typename T>
inline static void
serialize(Binary_Deserializer &self, const Array<T> &data)
{
	Array<T> &d = (Array<T> &)data;

	if (self.allocator != d.allocator || d.allocator == nullptr)
	{
		destroy(d);
		d = array_init<T>(self.allocator);
	}

	u64 count = 0;
	serialize(self, count);
	array_resize(d, count);
	for (u64 i = 0; i < d.count; ++i)
		serialize(self, d[i]);
}

inline static void
serialize(Binary_Deserializer &self, const String &data)
{
	String &d = (String &)data;

	if (self.allocator != d.allocator || d.allocator == nullptr)
	{
		string_deinit(d);
		d = string_init(self.allocator);
	}

	u64 count = 0;
	serialize(self, count);
	string_resize(d, count);
	for (u64 i = 0; i < d.count; ++i)
		serialize(self, d[i]);
}

inline static void
serialize(Binary_Deserializer &self, const char *&data)
{
	String out = {};
	serialize(self, out);
	data = out.data;
	out = {};
}

template <typename K, typename V>
inline static void
serialize(Binary_Deserializer &self, const Hash_Table<K, V> &data)
{
	Hash_Table<K, V> &d = (Hash_Table<K, V> &)data;

	// TODO: Should we add allocator in hash table?
	if (self.allocator != d.entries.allocator || d.entries.allocator == nullptr)
	{
		destroy(d);
		d = hash_table_init<K, V>(self.allocator);
	}

	u64 count = 0;
	serialize(self, count);
	hash_table_clear(d);
	for (u64 i = 0; i < count; ++i)
	{
		K key   = {};
		V value = {};
		serialize(self, key);
		serialize(self, value);
		hash_table_insert(d, key, value);
	}
}

inline static void
serialize(Binary_Deserializer &self, Binary_Serialization_Pair pair)
{
	pair.from(self, pair.name, pair.data);
}

inline static void
serialize(Binary_Deserializer &self, std::initializer_list<Binary_Serialization_Pair> pairs)
{
	for (const Binary_Serialization_Pair &pair : pairs)
		pair.from(self, pair.name, pair.data);
}

template <typename T>
inline static Array<u8>
to_binary(const T &data)
{
	Binary_Serializer self = binary_serializer_init();
	DEFER(binary_serializer_deinit(self));
	serialize(self, data);
	return array_copy(self.buffer);
}

template <typename T>
inline static void
from_binary(const Array<u8> &buffer, T &data)
{
	Binary_Deserializer self = binary_deserializer_init(buffer);
	DEFER(binary_deserializer_deinit(self));
	serialize(self, data);
}