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
	Array<JSON_Value> objects;
	Array<const char *> last_names;
	Array<SERIALIZER_BEGIN_STATE> last_states;

	u64 index;
	u64 serialization_index;
	u64 deserialization_index;
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
			json_value_object_insert_at(last, string_literal(name), value);
			break;
		}
		case SERIALIZER_BEGIN_STATE_ARRAY:
		{
			array_push(*last.as_array, value);
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
			json_value_object_insert_at(last, string_literal(name), value);
			break;
		}
		case SERIALIZER_BEGIN_STATE_ARRAY:
		{
			array_push(*last.as_array, value);
			break;
		}
		case SERIALIZER_BEGIN_STATE_STRING:
		{
			// TODO: This works?
			if constexpr (std::is_same_v<T, char>)
				string_append(*last.as_string, data);
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
_json_serializer_deserialize(JSON_Serializer *self, const char *name, T &data)
{
	auto &value = self->ctx->objects[self->ctx->deserialization_index];
	auto entry = hash_table_find(*value.as_object, string_literal(name));
	if (entry)
	{
		auto &vv = entry->value;
		switch (vv.kind)
		{
			case JSON_VALUE_KIND_NULL:
			{
				data = NULL;
				break;
			}
			case JSON_VALUE_KIND_BOOL:
			{
				data = vv.as_bool;
				break;
			}
			case JSON_VALUE_KIND_NUMBER:
			{
				data = (T)vv.as_number;
				break;
			}
			case JSON_VALUE_KIND_ARRAY:
			{
				// TODO: Support different types.
				auto &elem = (*vv.as_array)[self->ctx->index];
				switch (elem.kind)
				{
					case JSON_VALUE_KIND_NUMBER:
						data = (T)elem.as_number;
						break;
					case JSON_VALUE_KIND_STRING:
						if (self->ctx->index == 0)
						{
							data = (T)(*elem.as_string)[self->ctx->index];
							self->ctx->index++;
						}
						else
						{
							data = (T)(*elem.as_string)[self->ctx->index - 1];
							self->ctx->index++;
						}
						break;
					case JSON_VALUE_KIND_OBJECT:
					{
						self->ctx->deserialization_index++;
						break;
					}
					// case JSON_VALUE_KIND_OBJECT:
					// {
					// 	if (self->ctx->index % 2 == 0)
					// 		auto entry = hash_table_find(*elem.as_object, string_literal("key"));
					// 		data = entry->value[]
					// 	break;
					// }
				}
				break;
			}
			// TODO: Add const char * serialization for strings?
			case JSON_VALUE_KIND_STRING:
			{
				if (self->ctx->index == 0)
				{
					data = (T)(vv.as_string->count);
					self->ctx->index++;
				}
				else
				{
					data = (T)(*vv.as_string)[self->ctx->index - 1];
					self->ctx->index++;
				}
				break;
			}
			default:
				break;
		}
	}
}

// API.
JSON_Serializer::JSON_Serializer(memory::Allocator *allocator)
{
	JSON_Serializer *self = this;

	self->deserializing = false;
	self->ctx              = memory::allocate_zeroed<JSON_Serializer_Context>(allocator);
	self->ctx->allocator   = allocator;
	self->ctx->json_values = array_init<JSON_Value>(allocator);
	self->ctx->objects = array_init<JSON_Value>(allocator);
	self->ctx->last_names  = array_init<const char *>(allocator);
	self->ctx->last_states = array_init<SERIALIZER_BEGIN_STATE>(allocator);

	JSON_Value object = json_value_init_as_object(allocator);
	array_push(self->ctx->json_values, object);
	array_push(self->ctx->last_states, SERIALIZER_BEGIN_STATE_OBJECT);
}

JSON_Serializer::~JSON_Serializer()
{
	JSON_Serializer *self = this;
	memory::Allocator *allocator = self->ctx->allocator;
	destroy(self->ctx->json_values);
	destroy(self->ctx->objects);
	array_deinit(self->ctx->last_names);
	array_deinit(self->ctx->last_states);
	memory::deallocate(allocator, self->ctx);
	self->ctx = nullptr;
}

void
JSON_Serializer::serialize(const char *name, i8 data)
{
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, i16 data)
{
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, i32 data)
{
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, i64 data)
{
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, u8 data)
{
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, u16 data)
{
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, u32 data)
{
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, u64 data)
{
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, f32 data)
{
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, f64 data)
{
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, bool data)
{
	JSON_Serializer *self = this;
	_json_serializer_serialize_bool(self, name, data);
}

void
JSON_Serializer::serialize(const char *name, char data)
{
	JSON_Serializer *self = this;
	_json_serializer_serialize_number(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, i8 &data)
{
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, i16 &data)
{
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, i32 &data)
{
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, i64 &data)
{
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, u8 &data)
{
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, u16 &data)
{
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, u32 &data)
{
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, u64 &data)
{
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, f32 &data)
{
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, f64 &data)
{
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, bool &data)
{
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

void
JSON_Serializer::deserialize(const char *name, char &data)
{
	JSON_Serializer *self = this;
	_json_serializer_deserialize(self, name, data);
}

inline static void
_insert_value(JSON_Serializer *self, JSON_Value &value)
{
	auto last_state = self->ctx->last_states[self->ctx->last_states.count - 1];
	switch (last_state)
	{
		case SERIALIZER_BEGIN_STATE_OBJECT:
		{
			json_value_object_insert_at(self->ctx->json_values[self->ctx->json_values.count - 1], string_literal(array_last(self->ctx->last_names)), value);
			break;
		}
		case SERIALIZER_BEGIN_STATE_ARRAY:
		{
			array_push(*self->ctx->json_values[self->ctx->json_values.count - 1].as_array, value);
			break;
		}
		case SERIALIZER_BEGIN_STATE_STRING:
		{
			ASSERT(false, "[JSON_SERIALIZER]: Invalid SERIALIZER_BEGIN_STATE enum.");
			break;
		}
		default:
		{
			ASSERT(false, "[JSON_SERIALIZER]: Invalid SERIALIZER_BEGIN_STATE enum.");
			break;
		}
	}
}

void
JSON_Serializer::begin(SERIALIZER_BEGIN_STATE state, const char *name)
{
	JSON_Serializer *self = this;

	switch (state)
	{
		case SERIALIZER_BEGIN_STATE_OBJECT:
		{
			if (self->deserializing == false)
			{
				array_push(self->ctx->last_names, name);
				JSON_Value object = json_value_init_as_object();
				_insert_value(self, object);
				array_push(self->ctx->objects, object);
				array_push(self->ctx->json_values, object);
			}
			else
			{
				static bool first = true;
				if (first)
				{
					self->ctx->deserialization_index = 0;
					first = false;
				}
				else
				{
					self->ctx->deserialization_index++;
				}
			}
			array_push(self->ctx->last_states, state);
			break;
		}
		case SERIALIZER_BEGIN_STATE_ARRAY:
		{
			if (self->deserializing == false)
			{
				array_push(self->ctx->last_names, name);
				JSON_Value object = json_value_init_as_array();
				_insert_value(self, object);
				array_push(self->ctx->json_values, object);
			}
			self->ctx->index = 0;
			array_push(self->ctx->last_states, state);
			break;
		}
		case SERIALIZER_BEGIN_STATE_STRING:
		{
			if (self->deserializing == false)
			{
				array_push(self->ctx->last_names, name);
				JSON_Value object = json_value_init_as_string("");
				_insert_value(self, object);
				array_push(self->ctx->json_values, object);
			}
			self->ctx->index = 0;
			array_push(self->ctx->last_states, state);
			break;
		}
		default:
		{
			ASSERT(false, "[JSON_SERIALIZER]: Invalid SERIALIZER_BEGIN_STATE enum.");
			break;
		}
	}
}

void
JSON_Serializer::end()
{
	JSON_Serializer *self = this;

	if (self->deserializing == false)
	{
		array_pop(self->ctx->json_values);
		array_pop(self->ctx->last_names);
	}
	else
	{
		switch (array_last(self->ctx->last_states))
		{
			case SERIALIZER_BEGIN_STATE_ARRAY:
			{
				self->ctx->deserialization_index--;
				break;
			}
		}
	}
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
	hash_table_clear(*self->ctx->json_values[0].as_object);
	// TODO:
	array_resize(self->ctx->json_values, 1);
	array_resize(self->ctx->last_states, 1);
	array_clear(self->ctx->last_names);
	array_clear(self->ctx->objects);
	self->ctx->serialization_index   = 0;
	self->ctx->deserialization_index = 0;
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