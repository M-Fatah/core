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
	JSON_VALUE_KIND_NULL,
	JSON_VALUE_KIND_BOOL,
	JSON_VALUE_KIND_NUMBER,
	JSON_VALUE_KIND_STRING,
	JSON_VALUE_KIND_ARRAY,
	JSON_VALUE_KIND_OBJECT
};

struct JSON_Value
{
	// TODO: Is this a good idea?
	memory::Allocator *allocator;
	JSON_VALUE_KIND kind;
	union
	{
		bool as_bool;
		f64 as_number;
		String *as_string;
		Array<JSON_Value> *as_array;
		Hash_Table<String, JSON_Value> *as_object;
	};
};

// TODO: Revise these functions.
//       Move to cpp?
//       Add allocator?
//       Re-order the functions.
inline static JSON_Value
json_value_init_as_bool(bool value)
{
	JSON_Value self = {};
	self.allocator = memory::heap_allocator();
	self.kind = JSON_VALUE_KIND_BOOL;
	self.as_bool = value;
	return self;
}

inline static JSON_Value
json_value_init_as_number(f64 value)
{
	JSON_Value self = {};
	self.allocator = memory::heap_allocator();
	self.kind = JSON_VALUE_KIND_NUMBER;
	self.as_number = value;
	return self;
}

// TODO: Should this take a value?
inline static JSON_Value
json_value_init_as_string(const String &value, memory::Allocator *allocator = memory::heap_allocator())
{
	JSON_Value self = {};
	self.allocator = allocator;
	self.kind = JSON_VALUE_KIND_STRING;
	self.as_string = memory::allocate<String>(allocator);
	*self.as_string = clone(value, allocator);
	return self;
}

// TODO: Should this take a value?
inline static JSON_Value
json_value_init_as_string(const char *value, memory::Allocator *allocator = memory::heap_allocator())
{
	JSON_Value self = {};
	self.allocator = allocator;
	self.kind = JSON_VALUE_KIND_STRING;
	self.as_string = memory::allocate<String>(allocator);
	*self.as_string = string_from(value, allocator);
	return self;
}

inline static JSON_Value
json_value_init_as_array(memory::Allocator *allocator = memory::heap_allocator())
{
	JSON_Value self = {};
	self.allocator = allocator;
	self.kind = JSON_VALUE_KIND_ARRAY;
	self.as_array = memory::allocate_zeroed<Array<JSON_Value>>(allocator);
	*self.as_array = array_init<JSON_Value>(allocator);
	return self;
}

inline static JSON_Value
json_value_init_as_object(memory::Allocator *allocator = memory::heap_allocator())
{
	JSON_Value self = {};
	self.allocator = allocator;
	self.kind = JSON_VALUE_KIND_OBJECT;
	self.as_object = memory::allocate_zeroed<Hash_Table<String, JSON_Value>>(allocator);
	*self.as_object = hash_table_init<String, JSON_Value>(allocator);
	return self;
}

// TODO:
void
json_value_deinit(JSON_Value &self);

inline static void
json_value_object_insert_at(JSON_Value &self, const String &key, JSON_Value value)
{
	if (auto entry = hash_table_find(*self.as_object, key))
	{
		json_value_deinit(entry->value);
		entry->value = value;
	}
	else
	{
		hash_table_insert(*self.as_object, clone(key, self.allocator), value);
	}
}

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

CORE_API Result<String>
json_value_to_string(const JSON_Value &self, memory::Allocator *allocator = memory::heap_allocator());

CORE_API Error
json_value_to_file(const JSON_Value &self, const char *filepath);

inline static Error
json_value_to_file(const JSON_Value &self, const String &filepath)
{
	return json_value_to_file(self, filepath.data);
}

// TODO: Cloning json values with a different allocator other than the heap allocator, and then freeing that json values,
//           using json_value_deinit(); function will crash the application.
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
			copy.as_string = memory::allocate<String>(allocator);
			*copy.as_string = clone(*self.as_string, allocator);
			return copy;
		}
		case JSON_VALUE_KIND_ARRAY:
		{
			JSON_Value copy = self;
			copy.as_array = memory::allocate<Array<JSON_Value>>(allocator);
			*copy.as_array = clone(*self.as_array, allocator);
			return copy;
		}
		case JSON_VALUE_KIND_OBJECT:
		{
			JSON_Value copy = self;
			copy.as_object = memory::allocate<Hash_Table<String, JSON_Value>>(allocator);
			*copy.as_object = clone(*self.as_object, allocator);
			return copy;
		}
		default:
		{
			ASSERT(false, "[JSON]: Invalid JSON_VALUE_KIND.");
			return {};
		}
	}
}

inline static void
destroy(JSON_Value &self)
{
	json_value_deinit(self);
}