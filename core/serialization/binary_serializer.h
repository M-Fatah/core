#pragma once

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
	- [ ] What happens if the user serializes multiple entries with the same name in jsn and name dependent serializers.
		- [ ] Should we assert?
		- [ ] Should we print warning messages?
		- [ ] Should we override data?
	- [ ] deserializer_init() should take a block or a span or a view.
	- [ ] Return Error on failure.
	- [ ] Endianness?
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
	Error (*to)(Binary_Serializer &serializer, const char *name, void *data);
	Error (*from)(Binary_Deserializer &deserializer, const char *name, void *data);

	template <typename T>
	Binary_Serialization_Pair(const char *name, T &data)
	{
		Binary_Serialization_Pair &self = *this;
		self.name = name;
		self.data = (void *)&data;
		self.to = +[](Binary_Serializer &serializer, const char *, void *data) -> Error {
			T &d = *(T *)data;
			return serialize(serializer, d);
		};
		self.from = +[](Binary_Deserializer &deserializer, const char *, void *data) -> Error {
			T &d = *(T *)data;
			return serialize(deserializer, d);
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
serialize(Binary_Serializer &self, const char *&data)
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
serialize(Binary_Serializer &self, Binary_Serialization_Pair pair)
{
	return pair.to(self, pair.name, pair.data);
}

inline static Error
serialize(Binary_Serializer &self, std::initializer_list<Binary_Serialization_Pair> pairs)
{
	for (const Binary_Serialization_Pair &pair : pairs)
		if (Error error = pair.to(self, pair.name, pair.data))
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
serialize(Binary_Deserializer &self, const T &data)
{
	u8 *d = (u8 *)&data;
	u64 data_size = sizeof(data);

	if (self.offset + data_size > self.buffer.count)
		return Error{"[DESERIALIZER][BINARY]: Trying to deserialize beyond buffer capacity."};

	for (u64 i = 0; i < data_size; ++i)
		d[i] = self.buffer[i + self.offset];
	self.offset += data_size;

	return Error{};
}

template <typename T>
requires (std::is_pointer_v<T> && !std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
inline static Error
serialize(Binary_Deserializer& self, const T &data)
{
	T &d = (T &)data;

	if (d == nullptr)
		d = memory::allocate<std::remove_pointer_t<T>>(self.allocator);

	if (d == nullptr)
		return Error{"[DESERIALIZER][BINARY]: Could not allocate memory for passed pointer type."};

	return serialize(self, *d);
}

template <typename T, u64 N>
requires (!std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
inline static Error
serialize(Binary_Deserializer &self, const T (&data)[N])
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
serialize(Binary_Deserializer &self, const Block &block)
{
	Block &d = (Block &)block;

	if (Error error = serialize(self, d.size))
		return error;

	if (d.data == nullptr)
		d.data = (u8 *)memory::allocate(d.size);

	if (d.data == nullptr)
		return Error{"[DESERIALIZER][BINARY]: Could not allocate memory for passed pointer type."};

	for (u64 i = 0; i < d.size; ++i)
		if (Error error = serialize(self, ((u8 *)d.data)[i]))
			return error;

	return Error{};
}

template <typename T>
inline static Error
serialize(Binary_Deserializer &self, const Array<T> &data)
{
	Array<T> &d = (Array<T> &)data;

	if (self.allocator != d.allocator || d.allocator == nullptr)
	{
		destroy(d);
		d = array_init<T>(self.allocator);
	}

	u64 count = 0;
	if (Error error = serialize(self, count))
		return error;

	array_resize(d, count);
	for (u64 i = 0; i < d.count; ++i)
		if (Error error = serialize(self, d[i]))
			return error;

	return Error{};
}

inline static Error
serialize(Binary_Deserializer &self, const String &data)
{
	String &d = (String &)data;

	if (self.allocator != d.allocator || d.allocator == nullptr)
	{
		string_deinit(d);
		d = string_init(self.allocator);
	}

	u64 count = 0;
	if (Error error = serialize(self, count))
		return error;

	string_resize(d, count);
	for (u64 i = 0; i < d.count; ++i)
		if (Error error = serialize(self, d[i]))
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
	if (Error error = serialize(self, count))
		return error;

	hash_table_clear(d);
	for (u64 i = 0; i < count; ++i)
	{
		K key   = {};
		V value = {};

		if (Error error = serialize(self, key))
			return error;

		if (Error error = serialize(self, value))
			return error;

		hash_table_insert(d, key, value);
	}

	return Error{};
}

inline static Error
serialize(Binary_Deserializer &self, Binary_Serialization_Pair pair)
{
	return pair.from(self, pair.name, pair.data);
}

inline static Error
serialize(Binary_Deserializer &self, std::initializer_list<Binary_Serialization_Pair> pairs)
{
	for (const Binary_Serialization_Pair &pair : pairs)
		if (Error error = pair.from(self, pair.name, pair.data))
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