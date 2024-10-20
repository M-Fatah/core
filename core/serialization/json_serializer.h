#pragma once

#include <core/defines.h>
#include <core/json.h>
#include <core/logger.h>
#include <core/containers/array.h>
#include <core/containers/string.h>
#include <core/containers/hash_table.h>

// TODO: Remove.
#include <string>

// TODO: Remove.
/*
   base64.cpp and base64.h

   Copyright (C) 2004-2008 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch

*/

static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";


static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';

  }

  return ret;

}
std::string base64_decode(std::string const& encoded_string) {
  size_t in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::string ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = (char)base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret += char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = (char)base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
  }

  return ret;
}

struct Jsn_Serializer
{
	memory::Allocator *allocator;
	String buffer;
	JSON_Value value;
	Array<JSON_Value> values;
	bool is_valid;
};

struct Jsn_Serialization_Pair
{
	const char *name;
	void *data;
	void (*to)(Jsn_Serializer &self, const char *name, void *data);
	void (*from)(Jsn_Serializer &self, const char *name, void *data);

	template <typename T>
	Jsn_Serialization_Pair(const char *name, T &data)
	{
		Jsn_Serialization_Pair &self = *this;
		self.name = name;
		self.data = (void *)&data;
		self.to = +[](Jsn_Serializer &self, const char *, void *data) {
			T &d = *(T *)data;
			serialize(self, d);
		};
		self.from = +[](Jsn_Serializer &self, const char *, void *data) {
			T &d = *(T *)data;
			deserialize(self, d);
		};
	}
};

inline static Jsn_Serializer
jsn_serializer_init(memory::Allocator *allocator = memory::heap_allocator())
{
	return Jsn_Serializer {
		.allocator = allocator,
		.buffer = string_init(allocator),
		.value = {},
		.values = array_init<JSON_Value>(allocator),
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
	string_append(self.buffer, std::to_string(data).c_str()); // TODO: remove std::to_string().
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
requires (!std::is_same_v<T, char *> && !std::is_same_v<T, const char *>) // TODO: Test char arrays. For some reason, this gets invoked by C string calls.
inline static void
serialize(Jsn_Serializer &self, const T (&data)[N])
{
	string_append(self.buffer, '[');
	for (u64 i = 0; i < N; ++i)
	{
		if (i > 0)
			string_append(self.buffer, ',');
		serialize(self, data[i]);
	}
	string_append(self.buffer, ']');
}

template <typename T>
inline static void
serialize(Jsn_Serializer &self, const Array<T> &data)
{
	string_append(self.buffer, '[');
	for (u64 i = 0; i < data.count; ++i)
	{
		if (i > 0)
			string_append(self.buffer, ',');
		serialize(self, data[i]);
	}
	string_append(self.buffer, ']');
}

inline static void
serialize(Jsn_Serializer &self, const String &data)
{
	string_append(self.buffer, "\"{}\"", data);
}

inline static void
serialize(Jsn_Serializer &self, const char *&data)
{
	serialize(self, string_literal(data));
}

template <typename K, typename V>
inline static void
serialize(Jsn_Serializer &self, const Hash_Table<K, V> &data)
{
	string_append(self.buffer, '[');
	i32 i = 0;
	for (const Hash_Table_Entry<const K, V> &entry : data)
	{
		if (i > 0)
			string_append(self.buffer, ',');
		string_append(self.buffer, "{{\"key\":");
		serialize(self, entry.key);
		string_append(self.buffer, ",\"value\":");
		serialize(self, entry.value);
		string_append(self.buffer, '}');
		++i;
	}
	string_append(self.buffer, ']');
}

inline static void
serialize(Jsn_Serializer &self, const Block &block)
{
	std::string o = base64_encode((const unsigned char *)block.data, (u32)block.size);
	string_append(self.buffer, "\"{}\"", o.c_str());
}

inline static void
serialize(Jsn_Serializer &self, Jsn_Serialization_Pair pair)
{
	self.is_valid = true;

	if (!string_is_empty(self.buffer))
	{
		self.buffer.count--;
		string_append(self.buffer, ',');
	}
	else
	{
		string_append(self.buffer, '{');
	}

	string_append(self.buffer, "\"{}\":", pair.name);
	pair.to(self, pair.name, pair.data);
	string_append(self.buffer, '}');

	self.is_valid = false;
}

inline static void
serialize(Jsn_Serializer &self, std::initializer_list <Jsn_Serialization_Pair> pairs)
{
	string_append(self.buffer, '{');
	i32 i = 0;
	for (const Jsn_Serialization_Pair &pair : pairs)
	{
		if (i > 0)
			string_append(self.buffer, ',');
		string_append(self.buffer, "\"{}\":", pair.name);
		pair.to(self, pair.name, pair.data);
		++i;
	}
	string_append(self.buffer, '}');
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
deserialize(Jsn_Serializer& self, T &data)
{
	if (data == nullptr)
		data = memory::allocate<std::remove_pointer_t<T>>(self.allocator);
	deserialize(self, *data);
}

template <typename T, u64 N>
requires (!std::is_same_v<T, char *> && !std::is_same_v<T, const char *>) // TODO: Test char arrays. For some reason, this gets invoked by C string calls.
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
	if (self.allocator != data.allocator || data.allocator == nullptr)
	{
		destroy(data);
		data = array_init<T>(self.allocator);
	}

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
	if (self.allocator != data.allocator || data.allocator == nullptr)
	{
		string_deinit(data);
		data = string_init(self.allocator);
	}

	ASSERT(array_last(self.values).kind == JSON_VALUE_KIND_STRING, "[SERIALIZER][JSON]: JSON value is not a string!");
	String str = array_last(self.values).as_string;
	string_clear(data);
	string_append(data, str);
}

inline static void
deserialize(Jsn_Serializer &self, const char *&data)
{
	String out = {};
	deserialize(self, out);
	data = out.data;
	out = {};
}

template <typename K, typename V>
inline static void
deserialize(Jsn_Serializer &self, Hash_Table<K, V> &data)
{
	// TODO: Should we add allocator in hash table?
	if (self.allocator != data.entries.allocator || data.entries.allocator == nullptr)
	{
		destroy(data);
		data = hash_table_init<K, V>(self.allocator);
	}

	ASSERT(array_last(self.values).kind == JSON_VALUE_KIND_ARRAY, "[SERIALIZER][JSON]: JSON value is not an array!");
	Array<JSON_Value> array_values = array_last(self.values).as_array;

	hash_table_clear(data);
	hash_table_resize(data, array_values.count); // TODO: Should we remove this?
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
deserialize(Jsn_Serializer &self, Block &block)
{
	ASSERT(array_last(self.values).kind == JSON_VALUE_KIND_STRING, "[SERIALIZER][JSON]: JSON value is not a string!");
	String str = array_last(self.values).as_string;

	std::string o = base64_decode(std::string(str.data));

	if (block.data == nullptr)
		block.data = (u8 *)memory::allocate(o.size());

	::memcpy(block.data, o.data(), o.size());
	block.size = o.size();
}

inline static void
deserialize(Jsn_Serializer &self, Jsn_Serialization_Pair pair)
{
	// TODO: Print error.
	// TODO: Clear messages.
	// TODO: Better naming.
	json_value_deinit(self.value);

	auto [value, error] = json_value_from_string(self.buffer, self.allocator);
	self.value = value;
	ASSERT(!error, "[SERIALIZER][JSON]: Could not parse JSON string.");

	auto *json_entry = hash_table_find(value.as_object, string_literal(pair.name));
	ASSERT(json_entry, "[SERIALIZER][JSON]: Could not find JSON value with provided name.");

	array_push(self.values, json_entry->value);
	pair.from(self, pair.name, pair.data);
	array_pop(self.values);
}

inline static void
deserialize(Jsn_Serializer &self, std::initializer_list<Jsn_Serialization_Pair> pairs)
{
	for (const Jsn_Serialization_Pair &pair : pairs)
	{
		auto *json_entry = hash_table_find(array_last(self.values).as_object, string_literal(pair.name));
		ASSERT(json_entry, "[SERIALIZER][JSON]: Could not find JSON value with provided name.");

		array_push(self.values, json_entry->value);
		pair.from(self, pair.name, pair.data);
		array_pop(self.values);
	}
}