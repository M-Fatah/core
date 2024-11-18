#pragma once

#include <core/defines.h>
#include <core/json.h>
#include <core/logger.h>
#include <core/base64.h>
#include <core/containers/array.h>
#include <core/containers/string.h>
#include <core/containers/hash_table.h>

struct Json_Serializer
{
	memory::Allocator *allocator;
	String buffer;
};

struct Json_Deserializer
{
	memory::Allocator *allocator;
	Array<JSON_Value> values;
};

struct Json_Serialization_Pair
{
	const char *name;
	void *data;
	Error (*to)(Json_Serializer &self, const char *name, void *data);
	Error (*from)(Json_Deserializer &self, const char *name, void *data);

	template <typename T>
	Json_Serialization_Pair(const char *name, T &data)
	{
		Json_Serialization_Pair &self = *this;
		self.name = name;
		self.data = (void *)&data;
		self.to = +[](Json_Serializer &self, const char *, void *data) -> Error {
			return serialize(self, *(const T *)data);
		};
		self.from = +[](Json_Deserializer &self, const char *, void *data) -> Error {
			return serialize(self, *(std::remove_const_t<T> *)data);
		};
	}
};

inline static Json_Serializer
json_serializer_init(memory::Allocator *allocator = memory::heap_allocator())
{
	return Json_Serializer {
		.allocator = allocator,
		.buffer = string_init(allocator)
	};
}

inline static void
json_serializer_deinit(Json_Serializer &self)
{
	string_deinit(self.buffer);
	self = {};
}

template <typename T>
requires (std::is_arithmetic_v<T> && !std::is_same_v<T, bool> && !std::is_same_v<T, char>)
inline static Error
serialize(Json_Serializer &self, const T &data)
{
	string_append(self.buffer, "{}", data);
	return Error{};
}

inline static Error
serialize(Json_Serializer &self, const char &data)
{
	string_append(self.buffer, "{}", (i32)data);
	return Error{};
}

inline static Error
serialize(Json_Serializer &self, const bool &data)
{
	string_append(self.buffer, data ? "true" : "false");
	return Error{};
}

template <typename T>
requires (std::is_pointer_v<T> && !std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
inline static Error
serialize(Json_Serializer &self, const T &data)
{
	return serialize(self, *data);
}

template <typename T, u64 N>
requires (!std::is_same_v<T, char *> && !std::is_same_v<T, const char *>) // TODO: Test char arrays. For some reason, this gets invoked by C string calls.
inline static Error
serialize(Json_Serializer &self, const T (&data)[N])
{
	string_append(self.buffer, '[');
	for (u64 i = 0; i < N; ++i)
	{
		if (i > 0)
			string_append(self.buffer, ',');
		if (Error error = serialize(self, data[i]))
			return error;
	}
	string_append(self.buffer, ']');
	return Error{};
}

inline static Error
serialize(Json_Serializer &self, const Block &block)
{
	String o = base64_encode((const unsigned char *)block.data, (u32)block.size, self.allocator);
	DEFER(string_deinit(o));
	string_append(self.buffer, "\"{}\"", o);
	return Error{};
}

template <typename T>
inline static Error
serialize(Json_Serializer &self, const Array<T> &data)
{
	string_append(self.buffer, '[');
	for (u64 i = 0; i < data.count; ++i)
	{
		if (i > 0)
			string_append(self.buffer, ',');
		if (Error error = serialize(self, data[i]))
			return error;
	}
	string_append(self.buffer, ']');
	return Error{};
}

inline static Error
serialize(Json_Serializer &self, const String &data)
{
	string_append(self.buffer, "\"{}\"", data);
	return Error{};
}

inline static Error
serialize(Json_Serializer &self, const char *data)
{
	return serialize(self, string_literal(data));
}

template <typename K, typename V>
inline static Error
serialize(Json_Serializer &self, const Hash_Table<K, V> &data)
{
	string_append(self.buffer, '[');
	i32 i = 0;
	for (const Hash_Table_Entry<const K, V> &entry : data)
	{
		if (i > 0)
			string_append(self.buffer, ',');
		string_append(self.buffer, "{{\"key\":");

		if (Error error = serialize(self, entry.key))
			return error;

		string_append(self.buffer, ",\"value\":");

		if (Error error = serialize(self, entry.value))
			return error;

		string_append(self.buffer, '}');
		++i;
	}
	string_append(self.buffer, ']');

	return Error{};
}

inline static Error
serialize(Json_Serializer &self, Json_Serialization_Pair pair)
{
	if (string_is_empty(self.buffer))
	{
		string_append(self.buffer, '{');
	}
	else if (array_last(self.buffer) == ':')
	{
		string_append(self.buffer, '{');
	}
	else
	{
		self.buffer.count--;
		string_append(self.buffer, ',');
	}

	string_append(self.buffer, "\"{}\":", pair.name);
	if (Error error = pair.to(self, pair.name, pair.data))
		return error;
	string_append(self.buffer, '}');

	return Error{};
}

inline static Error
serialize(Json_Serializer &self, std::initializer_list<Json_Serialization_Pair> pairs)
{
	string_append(self.buffer, '{');
	i32 i = 0;
	for (const Json_Serialization_Pair &pair : pairs)
	{
		if (i > 0)
			string_append(self.buffer, ',');
		string_append(self.buffer, "\"{}\":", pair.name);
		if (Error error = pair.to(self, pair.name, pair.data))
			return error;
		++i;
	}
	string_append(self.buffer, '}');

	return Error{};
}

inline static Json_Deserializer
json_deserializer_init(String buffer, memory::Allocator *allocator = memory::heap_allocator())
{
	// TODO: Need to figure out where to put this on first deserialization.
	auto [value, error] = json_value_from_string(buffer, allocator);
	if (error)
	{
		LOG_ERROR(error.message);
		return {};
	}

	Array<JSON_Value> values = array_init<JSON_Value>(allocator);
	array_push(values, value);

	return Json_Deserializer {
		.allocator = allocator,
		.values = values
	};
}

inline static void
json_deserializer_deinit(Json_Deserializer &self)
{
	destroy(self.values);
	self = {};
}

template <typename T>
requires (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
inline static Error
serialize(Json_Deserializer &self, T &data)
{
	data = (T)json_value_get_as_number(array_last(self.values));
	return Error{};
}

inline static Error
serialize(Json_Deserializer &self, bool &data)
{
	data = json_value_get_as_bool(array_last(self.values));
	return Error{};
}

template <typename T>
requires (std::is_pointer_v<T> && !std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
inline static Error
serialize(Json_Deserializer &self, T &data)
{
	if (data == nullptr)
		data = memory::allocate<std::remove_pointer_t<T>>(self.allocator);

	if (data == nullptr)
		return Error{"[DESERIALIZER][JSON]: Could not allocate memory for passed pointer type."};

	return serialize(self, *data);
}

template <typename T, u64 N>
requires (!std::is_same_v<T, char *> && !std::is_same_v<T, const char *>) // TODO: Test char arrays. For some reason, this gets invoked by C string calls.
inline static Error
serialize(Json_Deserializer &self, T (&data)[N])
{
	Array<JSON_Value> array_values = json_value_get_as_array(array_last(self.values));
	if (array_values.count != N)
		return Error{"[DESERIALIZER][JSON]: Passed array count does not match the deserialized count."};

	for (u64 i = 0; i < array_values.count; ++i)
	{
		array_push(self.values, array_values[i]);

		if (Error error = serialize(self, data[i]))
			return error;

		array_pop(self.values);
	}

	return Error{};
}

inline static Error
serialize(Json_Deserializer &self, Block &block)
{
	String str = json_value_get_as_string(array_last(self.values));

	String o = base64_decode(str, self.allocator);
	DEFER(string_deinit(o));

	if (block.data == nullptr)
		block.data = (u8 *)memory::allocate(o.count);

	if (block.data == nullptr)
		return Error{"[DESERIALIZER][JSON]: Could not allocate memory for passed pointer type."};

	::memcpy(block.data, o.data, o.count);
	block.size = o.count;

	return Error{};
}

template <typename T>
inline static Error
serialize(Json_Deserializer &self, Array<T> &data)
{
	if (self.allocator != data.allocator || data.allocator == nullptr)
	{
		destroy(data);
		data = array_init<T>(self.allocator);
	}

	Array<JSON_Value> array_values = json_value_get_as_array(array_last(self.values));
	array_resize(data, array_values.count);
	for (u64 i = 0; i < data.count; ++i)
	{
		array_push(self.values, array_values[i]);
		if (Error error = serialize(self, data[i]))
			return error;
		array_pop(self.values);
	}

	return Error{};
}

inline static Error
serialize(Json_Deserializer &self, String &data)
{
	if (self.allocator != data.allocator || data.allocator == nullptr)
	{
		string_deinit(data);
		data = string_init(self.allocator);
	}

	String str = json_value_get_as_string(array_last(self.values));
	string_clear(data);
	string_append(data, str);

	return Error{};
}

inline static Error
serialize(Json_Deserializer &self, const char *&data)
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
serialize(Json_Deserializer &self, Hash_Table<K, V> &data)
{
	// TODO: Should we add allocator in hash table?
	if (self.allocator != data.entries.allocator || data.entries.allocator == nullptr)
	{
		destroy(data);
		data = hash_table_init<K, V>(self.allocator);
	}

	Array<JSON_Value> array_values = array_last(self.values).as_array;

	hash_table_clear(data);
	for (u64 i = 0; i < array_values.count; ++i)
	{
		K key   = {};
		V value = {};

		JSON_Value json_value = array_values[i];

		array_push(self.values, json_value_object_find(json_value, "key"));
		if (Error error = serialize(self, key))
			return error;
		array_pop(self.values);

		array_push(self.values, json_value_object_find(json_value, "value"));
		if (Error error = serialize(self, value))
			return error;
		array_pop(self.values);

		hash_table_insert(data, key, value);
	}

	return Error{};
}

inline static Error
serialize(Json_Deserializer &self, Json_Serialization_Pair pair)
{
	JSON_Value json_value = json_value_object_find(array_last(self.values), pair.name);
	if (json_value.kind == JSON_VALUE_KIND_INVALID)
		return Error{"[DESERIALIZER][JSON]: Could not find JSON value with the provided name."};

	array_push(self.values, json_value);
	if (Error error = pair.from(self, pair.name, pair.data))
		return error;
	array_pop(self.values);

	return Error{};
}

inline static Error
serialize(Json_Deserializer &self, std::initializer_list <Json_Serialization_Pair> pairs)
{
	for (const Json_Serialization_Pair &pair : pairs)
		if (Error error = serialize(self, pair))
			return error;
	return Error{};
}

template <typename T>
inline static Result<String>
to_json(const T &data)
{
	Json_Serializer self = json_serializer_init();
	DEFER(json_serializer_deinit(self));
	if (Error error = serialize(self, data))
		return error;
	return string_copy(self.buffer);
}

template <typename T>
inline static Error
from_json(const String &buffer, T &data)
{
	Json_Deserializer self = json_deserializer_init(buffer);
	DEFER(json_deserializer_deinit(self));
	return serialize(self, data);
}