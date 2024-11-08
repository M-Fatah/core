#pragma once

#include "core/export.h"
#include "core/defines.h"
#include "core/result.h"
#include "core/memory/memory.h"
#include "core/containers/array.h"
#include "core/containers/string.h"
#include "core/containers/hash_table.h"

enum JSON_VALUE_KIND : u8
{
	JSON_VALUE_KIND_INVALID,
	JSON_VALUE_KIND_NULL,
	JSON_VALUE_KIND_BOOL,
	JSON_VALUE_KIND_NUMBER,
	JSON_VALUE_KIND_STRING,
	JSON_VALUE_KIND_ARRAY,
	JSON_VALUE_KIND_OBJECT
};

struct JSON_Value
{
	JSON_VALUE_KIND kind;
	union
	{
		bool as_bool;
		f64 as_number;
		String as_string;
		Array<JSON_Value> as_array;
		Hash_Table<String, JSON_Value> as_object;
	};
};

CORE_API Result<JSON_Value>
json_value_from_string(const char *json_string, memory::Allocator *allocator = memory::heap_allocator());

CORE_API Result<JSON_Value>
json_value_from_file(const char *filepath, memory::Allocator *allocator = memory::heap_allocator());

inline static Result<JSON_Value>
json_value_from_string(const String &json_string, memory::Allocator *allocator = memory::heap_allocator())
{
	return json_value_from_string(json_string.data, allocator);
}

inline static Result<JSON_Value>
json_value_from_file(const String &filepath, memory::Allocator *allocator = memory::heap_allocator())
{
	return json_value_from_file(filepath.data, allocator);
}

CORE_API void
json_value_deinit(JSON_Value &self);

CORE_API JSON_Value
json_value_object_find(const JSON_Value &self, const String &name);

inline static JSON_Value
json_value_object_find(const JSON_Value &self, const char *name)
{
	return json_value_object_find(self, string_literal(name));
}

CORE_API bool
json_value_get_as_bool(const JSON_Value &self);

CORE_API f64
json_value_get_as_number(const JSON_Value &self);

CORE_API String
json_value_get_as_string(const JSON_Value &self);

CORE_API Array<JSON_Value>
json_value_get_as_array(const JSON_Value &self);

CORE_API Hash_Table<String, JSON_Value>
json_value_get_as_object(const JSON_Value &self);

CORE_API Result<String>
json_value_to_string(const JSON_Value &self, memory::Allocator *allocator = memory::heap_allocator());

CORE_API Error
json_value_to_file(const JSON_Value &self, const char *filepath);

inline static Error
json_value_to_file(const JSON_Value &self, const String &filepath)
{
	return json_value_to_file(self, filepath.data);
}

inline static JSON_Value
clone(const JSON_Value &self, memory::Allocator *allocator = memory::heap_allocator())
{
	switch (self.kind)
	{
		case JSON_VALUE_KIND_NULL:
		case JSON_VALUE_KIND_BOOL:
		case JSON_VALUE_KIND_NUMBER:
		{
			return self;
		}
		case JSON_VALUE_KIND_STRING:
		{
			JSON_Value copy = self;
			copy.as_string = string_copy(self.as_string, allocator);
			return copy;
		}
		case JSON_VALUE_KIND_ARRAY:
		{
			JSON_Value copy = self;
			copy.as_array = clone(self.as_array, allocator);
			return copy;
		}
		case JSON_VALUE_KIND_OBJECT:
		{
			JSON_Value copy = self;
			copy.as_object = clone(self.as_object, allocator);
			return copy;
		}
		default:
		{
			ASSERT(false, "[JSON]: Invalid JSON_VALUE_KIND.");
			return JSON_Value{};
		}
	}
}

inline static void
destroy(JSON_Value &self)
{
	json_value_deinit(self);
}