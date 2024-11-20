#pragma once

#include "core/serialization/serializer.h"
#include "core/defines.h"
#include "core/result.h"
#include "core/containers/array.h"
#include "core/containers/string.h"
#include "core/containers/hash_table.h"

#include <type_traits>

/*
	TODO:
	- [ ] Versioning.
	- [ ] Arena backing memory.
	- [ ] Either we assert that the user should use serialized pairs, or generate names for omitted types.
	- [ ] What happens if the user serializes multiple entries with the same name in json and name dependent serializers.
		- [ ] Should we assert?
		- [ ] Should we print warning messages?
		- [ ] Should we override data?
		- [ ] Should we return Error?
	- [ ] deserializer_init() should take a block or a span or a view.
	- [ ] Cleanup.

	- Binary serializer:
		- [ ] Endianness?
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
inline static Error
serialize(Binary_Serializer &self, const T &data)
{
	array_reserve(self.buffer, sizeof(data));
	::memcpy(self.buffer.data + self.offset, &data, sizeof(T));
	self.buffer.count += sizeof(T);
	self.offset += sizeof(T);
	return {};
}

template <typename T>
requires (std::is_pointer_v<T> && !std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
inline static Error
serialize(Binary_Serializer& self, const T &data)
{
	return serialize(self, *data);
}

template <typename T, u64 N>
requires (!std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
inline static Error
serialize(Binary_Serializer &self, const T (&data)[N])
{
	Error error = serialize(self, N);
	if (error)
		return error;

	for (u64 i = 0; i < N; ++i)
	{
		error = serialize(self, data[i]);
		if (error)
			return error;
	}

	return {};
}

inline static Error
serialize(Binary_Serializer &self, const Block &block)
{
	Error error = serialize(self, block.size);
	if (error)
		return error;

	array_reserve(self.buffer, block.size);
	::memcpy(self.buffer.data + self.offset, block.data, block.size); // TODO: Add array_memcpy().
	self.buffer.count += block.size;
	self.offset += block.size;

	return {};
}

template <typename T>
inline static Error
serialize(Binary_Serializer &self, const Array<T> &data)
{
	Error error = serialize(self, data.count);
	if (error)
		return error;
	for (u64 i = 0; i < data.count; ++i)
	{
		error = serialize(self, data[i]);
		if (error)
			return error;
	}

	return {};
}

inline static Error
serialize(Binary_Serializer &self, const char *data)
{
	return serialize(self, string_literal(data));
}

template <typename K, typename V>
inline static Error
serialize(Binary_Serializer &self, const Hash_Table<K, V> &data)
{
	if (Error error = serialize(self, data.count))
		return error;

	for (const Hash_Table_Entry<const K, V> &entry : data)
	{
		if (Error error = serialize(self, entry.key))
			return error;

		if (Error error = serialize(self, entry.value))
			return error;
	}

	return {};
}

inline static Error
serialize(Binary_Serializer &self, Serialize_Pair<Binary_Serializer> pair)
{
	return pair.archive(self, pair.name, pair.data);
}

inline static Error
serialize(Binary_Serializer &self, std::initializer_list<Serialize_Pair<Binary_Serializer>> pairs)
{
	for (const Serialize_Pair<Binary_Serializer> &pair : pairs)
		if (Error error = pair.archive(self, pair.name, pair.data))
			return error;
	return Error{};
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
inline static Error
serialize(Binary_Deserializer &self, T &data)
{
	u8 *d = (u8 *)&data;
	u64 data_size = sizeof(data);

	if (self.offset + data_size > self.buffer.count)
		return Error{"[DESERIALIZER][BINARY]: Trying to deserialize beyond buffer capacity."};

	// TODO: Memcpy?
	for (u64 i = 0; i < data_size; ++i)
		d[i] = self.buffer[i + self.offset];
	self.offset += data_size;

	return Error{};
}

template <typename T>
requires (std::is_pointer_v<T> && !std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
inline static Error
serialize(Binary_Deserializer& self, T &data)
{
	if (data == nullptr)
		data = memory::allocate<std::remove_pointer_t<T>>(self.allocator);

	if (data == nullptr)
		return Error{"[DESERIALIZER][BINARY]: Could not allocate memory for passed pointer type."};

	return serialize(self, *data);
}

template <typename T, u64 N>
requires (!std::is_same_v<T, char *> && !std::is_same_v<T, const char *>) // TODO: Test removal.
inline static Error
serialize(Binary_Deserializer &self, T (&data)[N])
{
	u64 count = 0;
	if (Error error = serialize(self, count))
		return error;

	if (count != N)
		return Error{"[DESERIALIZER][BINARY]: Passed array count does not match the deserialized count."};

	for (u64 i = 0; i < count; ++i)
		if (Error error = serialize(self, data[i]))
			return error;

	return Error{};
}

inline static Error
serialize(Binary_Deserializer &self, Block &block)
{
	if (Error error = serialize(self, block.size))
		return error;

	if (block.data == nullptr)
		block.data = (u8 *)memory::allocate(block.size);

	if (block.data == nullptr)
		return Error{"[DESERIALIZER][BINARY]: Could not allocate memory for passed pointer type."};

	for (u64 i = 0; i < block.size; ++i)
		if (Error error = serialize(self, ((u8 *)block.data)[i]))
			return error;

	return Error{};
}

template <typename T>
inline static Error
serialize(Binary_Deserializer &self, Array<T> &data)
{
	if (self.allocator != data.allocator || data.allocator == nullptr)
	{
		destroy(data);
		data = array_init<T>(self.allocator);
	}

	u64 count = 0;
	if (Error error = serialize(self, count))
		return error;

	array_resize(data, count);
	for (u64 i = 0; i < data.count; ++i)
		if (Error error = serialize(self, data[i]))
			return error;

	return Error{};
}

inline static Error
serialize(Binary_Deserializer &self, String &data)
{
	if (self.allocator != data.allocator || data.allocator == nullptr)
	{
		string_deinit(data);
		data = string_init(self.allocator);
	}

	u64 count = 0;
	if (Error error = serialize(self, count))
		return error;

	string_resize(data, count);
	for (u64 i = 0; i < data.count; ++i)
		if (Error error = serialize(self, data[i]))
			return error;

	return Error{};
}

inline static Error
serialize(Binary_Deserializer &self, const char *&data)
{
	String out = {};
	if (Error error = serialize(self, out))
		return error;

	data = out.data;
	out = {};

	return Error{};
}

template <typename K, typename V>
inline static Error
serialize(Binary_Deserializer &self, Hash_Table<K, V> &data)
{
	// TODO: Should we add allocator in hash table?
	if (self.allocator != data.entries.allocator || data.entries.allocator == nullptr)
	{
		destroy(data);
		data = hash_table_init<K, V>(self.allocator);
	}

	u64 count = 0;
	if (Error error = serialize(self, count))
		return error;

	hash_table_clear(data);
	for (u64 i = 0; i < count; ++i)
	{
		K key   = {};
		V value = {};

		if (Error error = serialize(self, key))
			return error;

		if (Error error = serialize(self, value))
			return error;

		hash_table_insert(data, key, value);
	}

	return Error{};
}

inline static Error
serialize(Binary_Deserializer &self, Serialize_Pair<Binary_Deserializer> pair)
{
	return pair.archive(self, pair.name, pair.data);
}

inline static Error
serialize(Binary_Deserializer &self, std::initializer_list<Serialize_Pair<Binary_Deserializer>> pairs)
{
	for (const Serialize_Pair<Binary_Deserializer> &pair : pairs)
		if (Error error = pair.archive(self, pair.name, pair.data))
			return error;
	return Error{};
}

template <typename T>
inline static Result<Array<u8>>
to_binary(const T &data)
{
	Binary_Serializer self = binary_serializer_init();
	DEFER(binary_serializer_deinit(self));
	if (Error error = serialize(self, data))
		return error;
	return array_copy(self.buffer);
}

template <typename T>
inline static Error
from_binary(const Array<u8> &buffer, T &data)
{
	Binary_Deserializer self = binary_deserializer_init(buffer);
	DEFER(binary_deserializer_deinit(self));
	return serialize(self, data);
}