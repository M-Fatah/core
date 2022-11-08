#include "core/serialization/json_serializer.h"

#include "core/logger.h"
#include "core/memory/memory.h"
#include "core/memory/pool_allocator.h"
#include "core/containers/array.h"
#include "core/platform/platform.h"
#include "core/json.h"

struct JSON_Serializer_Context
{
	memory::Allocator *allocator;

	Array<JSON_Value> json_values;
	Array<const char *> last_names;
	Array<SERIALIZER_BEGIN_STATE> last_states;

	bool first_element_in_string_or_array;

	u64 serialization_offset;
	u64 deserialization_offset;
};

inline static void
_json_serializer_serialize_bool(JSON_Serializer *self, const char *name, bool data)
{
	JSON_Value value = {};
	value.kind    = JSON_VALUE_KIND_BOOL;
	value.as_bool = data;
	auto &last = array_last(self->ctx->json_values);
	auto last_state = array_last(self->ctx->last_states);
	switch (last_state)
	{
		case SERIALIZER_BEGIN_STATE_OBJECT:
		{
			hash_table_insert(last.as_object, string_from(name), value);
			break;
		}
		case SERIALIZER_BEGIN_STATE_ARRAY:
		{
			if (self->ctx->first_element_in_string_or_array == false)
				array_push(last.as_array, value);
			self->ctx->first_element_in_string_or_array = false;
			break;
		}
		case SERIALIZER_BEGIN_STATE_STRING:
		{
			ASSERT(false, "[JSON_SERIALIZER]: Cannot push bool to string.");
			break;
		}
		default:
		{
			ASSERT(false, "[JSON_SERIALIZER]: Invalid SERIALIZER_BEGIN_STATE enum.");
			break;
		}
	}
}

template <typename T>
inline static void
_json_serializer_serialize_number(JSON_Serializer *self, const char *name, const T &data)
{
	JSON_Value value = {};
	value.kind      = JSON_VALUE_KIND_NUMBER;
	value.as_number = (f64)data;
	auto &last = array_last(self->ctx->json_values);
	auto last_state = array_last(self->ctx->last_states);
	switch (last_state)
	{
		case SERIALIZER_BEGIN_STATE_OBJECT:
		{
			hash_table_insert(last.as_object, string_from(name), value);
			break;
		}
		case SERIALIZER_BEGIN_STATE_ARRAY:
		{
			if (self->ctx->first_element_in_string_or_array == false)
				array_push(last.as_array, value);
			self->ctx->first_element_in_string_or_array = false;
			break;
		}
		case SERIALIZER_BEGIN_STATE_STRING:
		{
			// TODO: This works?
			if constexpr (std::is_same_v<T, char>)
				if (self->ctx->first_element_in_string_or_array == false)
					string_append(last.as_string, data);
			self->ctx->first_element_in_string_or_array = false;
			break;
		}
		default:
		{
			ASSERT(false, "[JSON_SERIALIZER]: Invalid SERIALIZER_BEGIN_STATE enum.");
			break;
		}
	}
}

inline static void
_insert_value(JSON_Serializer *self, SERIALIZER_BEGIN_STATE state, const char *name, JSON_Value &value)
{
	switch (state)
	{
		case SERIALIZER_BEGIN_STATE_OBJECT:
		{
			hash_table_insert(self->ctx->json_values[self->ctx->json_values.count - 1].as_object, string_from(name), value);
			break;
		}
		case SERIALIZER_BEGIN_STATE_ARRAY:
		{
			array_push(self->ctx->json_values[self->ctx->json_values.count - 1].as_array, value);
			break;
		}
		case SERIALIZER_BEGIN_STATE_STRING:
		{
			// hash_table_insert(self->ctx->json_values[self->ctx->json_values.count - 2].as_string, string_from(array_last(self->ctx->last_names)), last);
			// ASSERT(false, "[JSON_SERIALIZER]: Invalid SERIALIZER_BEGIN_STATE enum.");
			break;
		}
		default:
		{
			ASSERT(false, "[JSON_SERIALIZER]: Invalid SERIALIZER_BEGIN_STATE enum.");
			break;
		}
	}
}

template <typename T>
inline static void
_json_serializer_deserialize(JSON_Serializer *, const char *, T &)
{
}

// API.
JSON_Serializer::JSON_Serializer(memory::Allocator *allocator)
{
	JSON_Serializer *self = this;

	self->ctx                         = memory::allocate<JSON_Serializer_Context>(allocator);
	self->ctx->allocator              = allocator;
	self->ctx->serialization_offset   = 0;
	self->ctx->deserialization_offset = 0;
	self->ctx->json_values            = array_init<JSON_Value>(allocator);
	self->ctx->last_names             = array_init<const char *>(allocator);
	self->ctx->last_states            = array_init<SERIALIZER_BEGIN_STATE>(allocator);

	JSON_Value object = {};
	object.kind = JSON_VALUE_KIND_OBJECT;
	object.as_object = hash_table_init<String, JSON_Value>();
	array_push(self->ctx->json_values, object);
	array_push(self->ctx->last_states, SERIALIZER_BEGIN_STATE_OBJECT);
}

JSON_Serializer::~JSON_Serializer()
{
	JSON_Serializer *self = this;
	memory::Allocator *allocator = self->ctx->allocator;
	memory::deallocate(allocator, self->ctx);
	self->ctx = nullptr;
}

void
JSON_Serializer::serialize(const char *name, i8 data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, i16 data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, i32 data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, i64 data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, u8 data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, u16 data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, u32 data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, u64 data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, f32 data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, f64 data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, bool data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_serialize_bool(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, char data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, i8 &data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, i16 &data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, i32 &data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, i64 &data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, u8 &data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, u16 &data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, u32 &data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, u64 &data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, f32 &data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, f64 &data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, bool &data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, char &data)
{
	unused(name);
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::begin(SERIALIZER_BEGIN_STATE state, const char *name)
{
	JSON_Serializer *self = this;

	switch (state)
	{
		case SERIALIZER_BEGIN_STATE_OBJECT:
		{
			JSON_Value object = {};
			object.kind      = JSON_VALUE_KIND_OBJECT;
			object.as_object = hash_table_init<String, JSON_Value>();
			// auto &last = array_last(self->ctx->json_values);
			_insert_value(self, state, name, object);
			array_push(self->ctx->json_values, object);
			array_push(self->ctx->last_names, name);
			break;
		}
		case SERIALIZER_BEGIN_STATE_ARRAY:
		{
			JSON_Value object = {};
			object.kind     = JSON_VALUE_KIND_ARRAY;
			object.as_array = array_init<JSON_Value>();
			// auto &last = array_last(self->ctx->json_values);
			_insert_value(self, state, name, object);
			array_push(self->ctx->json_values, object);
			array_push(self->ctx->last_names, name);
			self->ctx->first_element_in_string_or_array = true;
			break;
		}
		case SERIALIZER_BEGIN_STATE_STRING:
		{
			JSON_Value object = {};
			object.kind      = JSON_VALUE_KIND_STRING;
			object.as_string = string_init();
			// auto &last = array_last(self->ctx->json_values);
			_insert_value(self, state, name, object);
			array_push(self->ctx->json_values, object);
			array_push(self->ctx->last_names, name);
			self->ctx->first_element_in_string_or_array = true;
			break;
		}
		default:
		{
			ASSERT(false, "[JSON_SERIALIZER]: Invalid SERIALIZER_BEGIN_STATE enum.");
			break;
		}
	}

	array_push(self->ctx->last_states, state);
}

void
JSON_Serializer::end()
{
	JSON_Serializer *self = this;
	auto &last = array_last(self->ctx->json_values);
	auto last_state = self->ctx->last_states[self->ctx->last_states.count - 2];
	switch (last_state)
	{
		case SERIALIZER_BEGIN_STATE_OBJECT:
		{
			hash_table_insert(self->ctx->json_values[self->ctx->json_values.count - 2].as_object, string_from(array_last(self->ctx->last_names)), last);
			break;
		}
		case SERIALIZER_BEGIN_STATE_ARRAY:
		{
			array_push(self->ctx->json_values[self->ctx->json_values.count - 2].as_array, last);
			break;
		}
		case SERIALIZER_BEGIN_STATE_STRING:
		{
			// hash_table_insert(self->ctx->json_values[self->ctx->json_values.count - 2].as_string, string_from(array_last(self->ctx->last_names)), last);
			ASSERT(false, "[JSON_SERIALIZER]: Invalid SERIALIZER_BEGIN_STATE enum.");
			break;
		}
		default:
		{
			ASSERT(false, "[JSON_SERIALIZER]: Invalid SERIALIZER_BEGIN_STATE enum.");
			break;
		}
	}
	array_pop(self->ctx->json_values);
	array_pop(self->ctx->last_names);
	array_pop(self->ctx->last_states);
}

void
JSON_Serializer::clear()
{
	JSON_Serializer *self = this;
	//
	// TODO:
	// This will get invalidated if we use an arena allocator and clear that arena,
	//    while json serializer is in use.
	// Since the arena might get resized, if we then clear it, it frees the underlying blocks,
	//    and then allocates a new one that of the total size of the previous blocks combined and with a different pointer,
	//    which will make our array's data pointer invalid.
	//
	hash_table_clear(self->ctx->json_values[0].as_object);
	// TODO:
	// array_clear(self->ctx->json_values);
	self->ctx->serialization_offset   = 0;
	self->ctx->deserialization_offset = 0;
}

JSON_Serializer *
json_serializer_init(memory::Allocator *allocator)
{
	return memory::allocate_and_call_constructor<JSON_Serializer>(allocator, allocator);
}

Result<JSON_Serializer *>
json_serializer_from_file(const char *filepath, memory::Allocator *allocator)
{
	JSON_Serializer *self = json_serializer_init(allocator);
	auto error = json_serializer_from_file(self, filepath);
	if (error)
	{
		json_serializer_deinit(self);
		return error;
	}
	return self;
}

void
json_serializer_deinit(JSON_Serializer *self)
{
	if (self)
	{
		memory::Allocator *allocator = self->ctx->allocator;
		memory::deallocate_and_call_destructor(allocator, self);
	}
}

void
json_serializer_clear(JSON_Serializer *self)
{
	self->clear();
}

Error
json_serializer_from_file(JSON_Serializer *self, const char *filepath)
{
	ASSERT(self->ctx, "[JSON_SERIALIZER]: Cannot read file data to uninitialized serializer.");
	auto [value, error] = json_value_from_file(filepath);
	// TODO: push.
	self->ctx->json_values[0] = value;
	return error;
}

Error
json_serializer_to_file(JSON_Serializer *self, const char *filepath)
{
	return json_value_to_file(self->ctx->json_values[0], filepath);
}