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
	ASSERT(last.kind == JSON_VALUE_KIND_OBJECT, "Shit");
	hash_table_insert(last.as_object, string_from(name), value);
}

template <typename T>
inline static void
_json_serializer_serialize_number(JSON_Serializer *self, const char *name, const T &data)
{
	JSON_Value value = {};
	value.kind      = JSON_VALUE_KIND_NUMBER;
	value.as_number = (f64)data;
	auto &last = array_last(self->ctx->json_values);
	ASSERT(last.kind == JSON_VALUE_KIND_OBJECT, "Shit");
	hash_table_insert(last.as_object, string_from(name), value);
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

	JSON_Value object = {};
	object.kind = JSON_VALUE_KIND_OBJECT;
	object.as_object = hash_table_init<String, JSON_Value>();
	array_push(self->ctx->json_values, object);
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
JSON_Serializer::begin_object(const char *name)
{
	JSON_Serializer *self = this;

	JSON_Value object = {};
	object.kind      = JSON_VALUE_KIND_OBJECT;
	object.as_object = hash_table_init<String, JSON_Value>();

	auto &last = array_last(self->ctx->json_values);
	hash_table_insert(last.as_object, string_from(name), object);
	array_push(self->ctx->json_values, object);
	array_push(self->ctx->last_names, name);
}

void
JSON_Serializer::end_object()
{
	JSON_Serializer *self = this;
	auto &last = array_last(self->ctx->json_values);
	hash_table_insert(self->ctx->json_values[self->ctx->json_values.count - 2].as_object, string_from(array_last(self->ctx->last_names)), last);
	array_pop(self->ctx->json_values);
	array_pop(self->ctx->last_names);
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