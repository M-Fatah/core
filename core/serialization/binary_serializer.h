#pragma once

#include "core/serialization/serializer.h"
#include "core/defines.h"
#include "core/result.h"
#include "core/containers/array.h"
#include "core/containers/string.h"
#include "core/containers/hash_table.h"

struct Binary_Serializer
{
	Array<U8> buffer;
	U64 offset;
	bool is_valid;
};

struct Binary_Deserializer
{
	memory::Allocator *allocator;
	Array<U8> buffer;
	U64 offset;
	bool is_valid;
};

inline static Binary_Serializer
binary_serializer_init(memory::Allocator *allocator = memory::heap_allocator())
{
	return Binary_Serializer {
		.buffer = array_init<U8>(allocator),
		.offset = 0,
		.is_valid = false
	};
}

inline static void
binary_serializer_deinit(Binary_Serializer &self)
{
	array_deinit(self.buffer);
	self = Binary_Serializer{};
}

template <typename T>
requires (std::is_arithmetic_v<T>)
inline static Error
serialize(Binary_Serializer &self, const T &data)
{
	if (!self.is_valid)
		return Error{"[SERIALIZER][BINARY]: Please use Serialize_Pair, for e.x 'serialize(serializer, {{\"a\", a}})'."};

	array_resize(self.buffer, self.buffer.count + sizeof(T));
	::memcpy(self.buffer.data + self.offset, &data, sizeof(T)); // TODO: Add array_memcpy().
	self.offset += sizeof(T);
	return Error{};
}

template <typename T>
requires (std::is_pointer_v<T> && !is_c_string_v<T>)
inline static Error
serialize(Binary_Serializer& self, const T &data)
{
	return serialize(self, *data);
}

template <typename T>
requires (std::is_array_v<T>)
inline static Error
serialize(Binary_Serializer &self, const T &data)
{
	if (!self.is_valid)
		return Error{"[SERIALIZER][BINARY]: Please use Serialize_Pair, for e.x 'serialize(serializer, {{\"a\", a}})'."};

	if (Error error = serialize(self, count_of(data)))
		return error;

	for (U64 i = 0; i < count_of(data); ++i)
		if (Error error = serialize(self, data[i]))
			return error;

	return Error{};
}

inline static Error
serialize(Binary_Serializer &self, const Memory_Block &block)
{
	if (!self.is_valid)
		return Error{"[SERIALIZER][BINARY]: Please use Serialize_Pair, for e.x 'serialize(serializer, {{\"a\", a}})'."};

	if (Error error = serialize(self, block.size))
		return error;

	array_resize(self.buffer, self.buffer.count + block.size);
	::memcpy(self.buffer.data + self.offset, block.data, block.size); // TODO: Add array_memcpy().
	self.offset += block.size;

	return Error{};
}

template <typename T>
inline static Error
serialize(Binary_Serializer &self, const Array<T> &data)
{
	if (!self.is_valid)
		return Error{"[SERIALIZER][BINARY]: Please use Serialize_Pair, for e.x 'serialize(serializer, {{\"a\", a}})'."};

	if (Error error = serialize(self, data.count))
		return error;

	for (U64 i = 0; i < data.count; ++i)
		if (Error error = serialize(self, data[i]))
			return error;

	return Error{};
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
	if (!self.is_valid)
		return Error{"[SERIALIZER][BINARY]: Please use Serialize_Pair, for e.x 'serialize(serializer, {{\"a\", a}})'."};

	if (Error error = serialize(self, data.count))
		return error;

	for (const Hash_Table_Entry<const K, V> &entry : data)
	{
		if (Error error = serialize(self, entry.key))
			return error;

		if (Error error = serialize(self, entry.value))
			return error;
	}

	return Error{};
}

template <typename T>
inline static Error
serialize(Binary_Serializer &self, const char *name, const T &data)
{
	unused(name);
	self.is_valid = true;
	DEFER(self.is_valid = false);
	return serialize(self, data);
}

inline static Binary_Deserializer
binary_deserializer_init(const Array<U8> &buffer, memory::Allocator *allocator = memory::heap_allocator())
{
	return Binary_Deserializer {
		.allocator = allocator,
		.buffer = buffer,
		.offset = 0,
		.is_valid = false
	};
}

inline static void
binary_deserializer_deinit(Binary_Deserializer &self)
{
	self = Binary_Deserializer{};
}

template <typename T>
requires (std::is_arithmetic_v<T>)
inline static Error
serialize(Binary_Deserializer &self, T &data)
{
	if (!self.is_valid)
		return Error{"[DESERIALIZER][BINARY]: Please use Serialize_Pair, for e.x 'serialize(deserializer, {{\"a\", a}})'."};

	U8 *d = (U8 *)&data;
	U64 data_size = sizeof(data);

	if (self.offset + data_size > self.buffer.count)
		return Error{"[DESERIALIZER][BINARY]: Trying to deserialize beyond buffer capacity."};

	// TODO: Memcpy?
	for (U64 i = 0; i < data_size; ++i)
		d[i] = self.buffer[i + self.offset];
	self.offset += data_size;

	return Error{};
}

template <typename T>
requires (std::is_pointer_v<T> && !is_c_string_v<T>)
inline static Error
serialize(Binary_Deserializer& self, T &data)
{
	if (!self.is_valid)
		return Error{"[DESERIALIZER][BINARY]: Please use Serialize_Pair, for e.x 'serialize(deserializer, {{\"a\", a}})'."};

	if (data == nullptr)
		data = memory::allocate<std::remove_pointer_t<T>>(self.allocator);

	if (data == nullptr)
		return Error{"[DESERIALIZER][BINARY]: Could not allocate memory for passed pointer type."};

	return serialize(self, *data);
}

template <typename T>
requires (std::is_array_v<T>)
inline static Error
serialize(Binary_Deserializer &self, T &data)
{
	if (!self.is_valid)
		return Error{"[DESERIALIZER][BINARY]: Please use Serialize_Pair, for e.x 'serialize(deserializer, {{\"a\", a}})'."};

	U64 count = 0;
	if (Error error = serialize(self, count))
		return error;

	if (count != count_of(data))
		return Error{"[DESERIALIZER][BINARY]: Passed array count does not match the deserialized count."};

	for (U64 i = 0; i < count; ++i)
		if (Error error = serialize(self, data[i]))
			return error;

	return Error{};
}

inline static Error
serialize(Binary_Deserializer &self, Memory_Block &block)
{
	if (!self.is_valid)
		return Error{"[DESERIALIZER][BINARY]: Please use Serialize_Pair, for e.x 'serialize(deserializer, {{\"a\", a}})'."};

	U64 size = 0;
	if (Error error = serialize(self, size))
		return error;

	if (self.offset + size > self.buffer.count)
		return Error{"[DESERIALIZER][BINARY]: Trying to deserialize beyond buffer capacity."};

	if (block.data == nullptr)
		block = memory::allocate(self.allocator, size, alignof(U8));
	else if (block.size < size)
		return Error{"[DESERIALIZER][BINARY]: Passed block capacity is smaller than the deserialized block."};

	if (block.data == nullptr)
		return Error{"[DESERIALIZER][BINARY]: Could not allocate memory for passed pointer type."};

	::memcpy(block.data, self.buffer.data + self.offset, size);
	self.offset += size;
	block.size   = size;

	return Error{};
}

template <typename T>
inline static Error
serialize(Binary_Deserializer &self, Array<T> &data)
{
	if (!self.is_valid)
		return Error{"[DESERIALIZER][BINARY]: Please use Serialize_Pair, for e.x 'serialize(deserializer, {{\"a\", a}})'."};

	if (self.allocator != data.allocator || data.allocator == nullptr)
	{
		destroy(data);
		data = array_init<T>(self.allocator);
	}

	U64 count = 0;
	if (Error error = serialize(self, count))
		return error;

	array_resize(data, count);
	for (U64 i = 0; i < data.count; ++i)
		if (Error error = serialize(self, data[i]))
			return error;

	return Error{};
}

inline static Error
serialize(Binary_Deserializer &self, String &data)
{
	if (!self.is_valid)
		return Error{"[DESERIALIZER][BINARY]: Please use Serialize_Pair, for e.x 'serialize(deserializer, {{\"a\", a}})'."};

	if (self.allocator != data.allocator || data.allocator == nullptr)
	{
		string_deinit(data);
		data = string_init(self.allocator);
	}

	U64 count = 0;
	if (Error error = serialize(self, count))
		return error;

	string_resize(data, count);
	for (U64 i = 0; i < data.count; ++i)
		if (Error error = serialize(self, data[i]))
			return error;

	return Error{};
}

inline static Error
serialize(Binary_Deserializer &self, const char *&data)
{
	if (!self.is_valid)
		return Error{"[DESERIALIZER][BINARY]: Please use Serialize_Pair, for e.x 'serialize(deserializer, {{\"a\", a}})'."};

	U64 count = 0;
	if (Error error = serialize(self, count))
		return error;

	if (self.offset + count > self.buffer.count)
		return Error{"[DESERIALIZER][BINARY]: Trying to deserialize beyond buffer capacity."};

	Memory_Block block = memory::allocate(self.allocator, count + 1, alignof(char));
	if (block.data == nullptr)
		return Error{"[DESERIALIZER][BINARY]: Could not allocate memory for passed c-string type."};

	char *output = (char *)block.data;
	::memcpy(output, self.buffer.data + self.offset, count);
	self.offset += count;
	output[count] = '\0';
	data = output;

	return Error{};
}

template <typename K, typename V>
inline static Error
serialize(Binary_Deserializer &self, Hash_Table<K, V> &data)
{
	if (!self.is_valid)
		return Error{"[DESERIALIZER][BINARY]: Please use Serialize_Pair, for e.x 'serialize(deserializer, {{\"a\", a}})'."};

	// TODO: Should we add allocator in hash table?
	if (self.allocator != data.entries.allocator || data.entries.allocator == nullptr)
	{
		destroy(data);
		data = hash_table_init<K, V>(self.allocator);
	}

	U64 count = 0;
	if (Error error = serialize(self, count))
		return error;

	hash_table_clear(data);
	for (U64 i = 0; i < count; ++i)
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

template <typename T>
inline static Error
serialize(Binary_Deserializer &self, const char *name, T &data)
{
	unused(name);
	self.is_valid = true;
	DEFER(self.is_valid = false);
	return serialize(self, data);
}

template <typename T>
inline static Result<Array<U8>>
to_binary(const T &data, memory::Allocator *allocator = memory::heap_allocator())
{
	Binary_Serializer self = binary_serializer_init(allocator);
	DEFER(self = Binary_Serializer{});
	if (Error error = serialize(self, data))
		return error;
	return self.buffer;
}

template <typename T>
inline static Error
from_binary(const Array<U8> &buffer, T &data, memory::Allocator *allocator = memory::heap_allocator())
{
	Binary_Deserializer self = binary_deserializer_init(buffer, allocator);
	DEFER(binary_deserializer_deinit(self));
	return serialize(self, data);
}