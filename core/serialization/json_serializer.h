#pragma once

#include "core/serialization/serializer.h"
#include "core/defines.h"
#include "core/json.h"
#include "core/logger.h"
#include "core/base64.h"
#include "core/containers/array.h"
#include "core/containers/string.h"
#include "core/containers/hash_table.h"

struct Json_Serializer
{
	memory::Allocator *allocator;
	Array<JSON_Value> values;
	bool is_valid;
};

struct Json_Deserializer
{
	memory::Allocator *allocator;
	Array<JSON_Value> values;
	bool is_valid;
};

inline static Json_Serializer
json_serializer_init(memory::Allocator *allocator = memory::heap_allocator())
{
	return Json_Serializer {
		.allocator = allocator,
		.values = array_from({json_value_init_as_object(allocator)}, allocator),
		.is_valid = false
	};
}

inline static void
json_serializer_deinit(Json_Serializer &self)
{
	destroy(self.values);
	self = Json_Serializer{};
}

template <typename T>
requires (std::is_arithmetic_v<T>)
inline static Error
serialize(Json_Serializer &self, const T &data)
{
	if (!self.is_valid)
		return Error{"[SERIALIZER][JSON]: Please use Serialize_Pair, for e.x 'serialize(serializer, {{\"a\", a}})'."};

	JSON_Value &value = array_last(self.values);
	value = json_value_init_as_number((f64)data);
	return Error{};
}

inline static Error
serialize(Json_Serializer &self, const bool &data)
{
	if (!self.is_valid)
		return Error{"[SERIALIZER][JSON]: Please use Serialize_Pair, for e.x 'serialize(serializer, {{\"a\", a}})'."};

	JSON_Value &value = array_last(self.values);
	value = json_value_init_as_bool(data);
	return Error{};
}

template <typename T>
requires (std::is_pointer_v<T> && !std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
inline static Error
serialize(Json_Serializer &self, const T &data)
{
	return serialize(self, *data);
}

template <typename T>
requires (std::is_array_v<T>)
inline static Error
serialize(Json_Serializer &self, const T &data)
{
	if (!self.is_valid)
		return Error{"[SERIALIZER][JSON]: Please use Serialize_Pair, for e.x 'serialize(serializer, {{\"a\", a}})'."};

	JSON_Value &value = array_last(self.values);
	value = json_value_init_as_array(self.allocator);

	for (u64 i = 0; i < count_of(data); ++i)
	{
		array_push(self.values, JSON_Value{});
		if (Error error = serialize(self, data[i]))
			return error;
		array_push(value.as_array, array_pop(self.values));
	}

	return Error{};
}

inline static Error
serialize(Json_Serializer &self, const Block &block)
{
	if (!self.is_valid)
		return Error{"[SERIALIZER][JSON]: Please use Serialize_Pair, for e.x 'serialize(serializer, {{\"a\", a}})'."};

	JSON_Value &value = array_last(self.values);
	value.kind = JSON_VALUE_KIND_STRING;
	value.as_string = base64_encode((const u8 *)block.data, (u32)block.size, self.allocator);
	return Error{};
}

template <typename T>
inline static Error
serialize(Json_Serializer &self, const Array<T> &data)
{
	if (!self.is_valid)
		return Error{"[SERIALIZER][JSON]: Please use Serialize_Pair, for e.x 'serialize(serializer, {{\"a\", a}})'."};

	JSON_Value &value = array_last(self.values);
	value = json_value_init_as_array(self.allocator);

	for (u64 i = 0; i < data.count; ++i)
	{
		array_push(self.values, JSON_Value{});
		if (Error error = serialize(self, data[i]))
			return error;
		JSON_Value element = array_pop(self.values);
		array_push(value.as_array, element);
	}

	return Error{};
}

inline static Error
serialize(Json_Serializer &self, const String &data)
{
	if (!self.is_valid)
		return Error{"[SERIALIZER][JSON]: Please use Serialize_Pair, for e.x 'serialize(serializer, {{\"a\", a}})'."};

	JSON_Value &value = array_last(self.values);
	value.kind = JSON_VALUE_KIND_STRING;
	value.as_string = string_copy(data, self.allocator);
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
	if (!self.is_valid)
		return Error{"[SERIALIZER][JSON]: Please use Serialize_Pair, for e.x 'serialize(serializer, {{\"a\", a}})'."};

	JSON_Value &array = array_last(self.values);
	array = json_value_init_as_array(self.allocator);

	for (const Hash_Table_Entry<const K, V> &entry : data)
	{
		array_push(self.values, JSON_Value{});
		if (Error error = serialize(self, entry.key))
			return error;
		JSON_Value key = array_pop(self.values);

		array_push(self.values, JSON_Value{});
		if (Error error = serialize(self, entry.value))
			return error;
		JSON_Value value = array_pop(self.values);

		JSON_Value key_value_json_object = json_value_init_as_object(self.allocator);
		json_value_object_insert(key_value_json_object, "key", key);
		json_value_object_insert(key_value_json_object, "value", value);
		array_push(array.as_array, key_value_json_object);
	}

	return Error{};
}

template <typename T>
inline static Error
serialize(Json_Serializer &self, const char *name, const T &data)
{
	self.is_valid = true;
	DEFER(self.is_valid = false);

	array_push(self.values, json_value_init_as_object(self.allocator));

	if (Error error = serialize(self, data))
		return error;

	JSON_Value object = array_pop(self.values);

	if (json_value_object_find(array_last(self.values), name))
		log_warning("[SERIALIZER][JSON]: Overwrite of duplicate json object with name '{}'.", name);

	json_value_object_insert(array_last(self.values), name, object);

	return Error{};
}

inline static Json_Deserializer
json_deserializer_init(const String &buffer, memory::Allocator *allocator = memory::heap_allocator())
{
	// TODO: Need to figure out where to put this on first deserialization.
	auto [value, error] = json_value_from_string(buffer, allocator);
	if (error)
	{
		log_error(error.message.data);
		return {};
	}

	Array<JSON_Value> values = array_init<JSON_Value>(allocator);
	array_push(values, value);

	return Json_Deserializer {
		.allocator = allocator,
		.values = values,
		.is_valid = false
	};
}

inline static Json_Deserializer
json_deserializer_init(const JSON_Value &value, memory::Allocator *allocator = memory::heap_allocator())
{
	Array<JSON_Value> values = array_init<JSON_Value>(allocator);
	array_push(values, json_value_copy(value));

	return Json_Deserializer {
		.allocator = allocator,
		.values = values,
		.is_valid = false
	};
}

inline static void
json_deserializer_deinit(Json_Deserializer &self)
{
	destroy(self.values);
	self = Json_Deserializer{};
}

template <typename T>
requires (std::is_arithmetic_v<T>)
inline static Error
serialize(Json_Deserializer &self, T &data)
{
	if (!self.is_valid)
		return Error{"[DESERIALIZER][JSON]: Please use Serialize_Pair, for e.x 'serialize(deserializer, {{\"a\", a}})'."};

	data = (T)json_value_get_as_number(array_last(self.values));
	return Error{};
}

inline static Error
serialize(Json_Deserializer &self, bool &data)
{
	if (!self.is_valid)
		return Error{"[DESERIALIZER][JSON]: Please use Serialize_Pair, for e.x 'serialize(deserializer, {{\"a\", a}})'."};

	data = json_value_get_as_bool(array_last(self.values));
	return Error{};
}

template <typename T>
requires (std::is_pointer_v<T> && !std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
inline static Error
serialize(Json_Deserializer &self, T &data)
{
	if (!self.is_valid)
		return Error{"[DESERIALIZER][JSON]: Please use Serialize_Pair, for e.x 'serialize(deserializer, {{\"a\", a}})'."};

	if (data == nullptr)
		data = memory::allocate<std::remove_pointer_t<T>>(self.allocator);

	if (data == nullptr)
		return Error{"[DESERIALIZER][JSON]: Could not allocate memory for passed pointer type."};

	return serialize(self, *data);
}

template <typename T>
requires (std::is_array_v<T>)
inline static Error
serialize(Json_Deserializer &self, T &data)
{
	if (!self.is_valid)
		return Error{"[DESERIALIZER][JSON]: Please use Serialize_Pair, for e.x 'serialize(deserializer, {{\"a\", a}})'."};

	Array<JSON_Value> array_values = json_value_get_as_array(array_last(self.values));
	if (array_values.count != count_of(data))
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
	if (!self.is_valid)
		return Error{"[DESERIALIZER][JSON]: Please use Serialize_Pair, for e.x 'serialize(deserializer, {{\"a\", a}})'."};

	String str = json_value_get_as_string(array_last(self.values));

	String o = base64_decode(str, memory::temp_allocator());
	DEFER(string_deinit(o));

	if (block.data == nullptr)
		block.data = (u8 *)memory::allocate(self.allocator, o.count);

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
	if (!self.is_valid)
		return Error{"[DESERIALIZER][JSON]: Please use Serialize_Pair, for e.x 'serialize(deserializer, {{\"a\", a}})'."};

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
	if (!self.is_valid)
		return Error{"[DESERIALIZER][JSON]: Please use Serialize_Pair, for e.x 'serialize(deserializer, {{\"a\", a}})'."};

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
	if (!self.is_valid)
		return Error{"[DESERIALIZER][JSON]: Please use Serialize_Pair, for e.x 'serialize(deserializer, {{\"a\", a}})'."};

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
	if (!self.is_valid)
		return Error{"[DESERIALIZER][JSON]: Please use Serialize_Pair, for e.x 'serialize(deserializer, {{\"a\", a}})'."};

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

template <typename T>
inline static Error
serialize(Json_Deserializer &self, const char *name, T &data)
{
	self.is_valid = true;
	DEFER(self.is_valid = false);

	JSON_Value json_value = json_value_object_find(array_last(self.values), name);
	if (!json_value)
		return Error{"[DESERIALIZER][JSON]: Could not find JSON value with the provided name."};

	array_push(self.values, json_value);
	if (Error error = serialize(self, data))
		return error;
	array_pop(self.values);

	return Error{};
}

template <typename T>
inline static Result<String>
to_json(const T &data, memory::Allocator *allocator = memory::heap_allocator())
{
	Json_Serializer self = json_serializer_init(allocator);
	DEFER(json_serializer_deinit(self));
	if constexpr (std::is_class_v<T>                 &&
				 !is_specialization_v<T, Array>      && // TODO: Add is_array<>
				 !is_specialization_v<T, Hash_Table> && // TODO: Add is_hash_table<>
				 !std::is_same_v<T, Block>)
	{
		if (Error error = serialize(self, data))
			return error;
	}
	else
	{
		if (Error error = serialize(self, {"data", data}))
			return error;
	}
	return json_value_to_string(array_first(self.values), allocator);
}

template <typename T>
inline static Error
from_json(const String &buffer, T &data, memory::Allocator *allocator = memory::heap_allocator())
{
	Json_Deserializer self = json_deserializer_init(buffer, allocator);
	DEFER(json_deserializer_deinit(self));
	if constexpr (std::is_class_v<T>                 &&
				 !is_specialization_v<T, Array>      && // TODO: Add is_array<>
				 !is_specialization_v<T, Hash_Table> && // TODO: Add is_hash_table<>
				 !std::is_same_v<T, Block>)
	{
		if (Error error = serialize(self, data))
			return error;
	}
	else
	{
		if (Error error = serialize(self, {"data", data}))
			return error;
	}
	return Error{};
}