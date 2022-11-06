#include "core/serialization/serializer.h"

#include "core/defines.h"
#include "core/result.h"
#include "core/containers/string.h"

/*
	TODO:
	- [ ] Rename to Json_Serializer?
*/

#define SERIALIZE(T)                             \
void                                             \
serialize(const char *name, T data) override;

#define DESERIALIZE(T)                           \
void                                             \
deserialize(const char *name, T &data) override;

struct JSON_Serializer : Serializer
{
	struct JSON_Serializer_Context *ctx;

	JSON_Serializer(memory::Allocator *allocator = memory::heap_allocator());

	~JSON_Serializer() override;

	SERIALIZE(i8)
	SERIALIZE(i16)
	SERIALIZE(i32)
	SERIALIZE(i64)
	SERIALIZE(u8)
	SERIALIZE(u16)
	SERIALIZE(u32)
	SERIALIZE(u64)
	SERIALIZE(f32)
	SERIALIZE(f64)
	SERIALIZE(bool)
	SERIALIZE(char)

	DESERIALIZE(i8)
	DESERIALIZE(i16)
	DESERIALIZE(i32)
	DESERIALIZE(i64)
	DESERIALIZE(u8)
	DESERIALIZE(u16)
	DESERIALIZE(u32)
	DESERIALIZE(u64)
	DESERIALIZE(f32)
	DESERIALIZE(f64)
	DESERIALIZE(bool)
	DESERIALIZE(char)

	void
	begin_object(const char *name) override;

	void
	end_object() override;

	void
	clear() override;
};

#undef SERIALIZE
#undef DESERIALIZE

CORE_API JSON_Serializer *
json_serializer_init(memory::Allocator *allocator = memory::heap_allocator());

CORE_API Result<JSON_Serializer *>
json_serializer_from_file(const char *filepath, memory::Allocator *allocator = memory::heap_allocator());

inline static Result<JSON_Serializer *>
json_serializer_from_file(const String &filepath, memory::Allocator *allocator = memory::heap_allocator())
{
	return json_serializer_from_file(filepath.data, allocator);
}

CORE_API void
json_serializer_deinit(JSON_Serializer *self);

template <typename T>
inline static void
json_serializer_serialize(JSON_Serializer *self, const char *name, const T &data)
{
	serialize(self, name, data);
}

template <typename T>
inline static void
json_serializer_deserialize(JSON_Serializer *self, const char *name, T &data)
{
	deserialize(self, name, data);
}

CORE_API void
json_serializer_clear(JSON_Serializer *self);

CORE_API Error
json_serializer_from_file(JSON_Serializer *self, const char *filepath);

inline static Error
json_serializer_from_file(JSON_Serializer *self, const String &filepath)
{
	return json_serializer_from_file(self, filepath.data);
}

CORE_API Error
json_serializer_to_file(JSON_Serializer *self, const char *filepath);

inline static Error
json_serializer_to_file(JSON_Serializer *self, const String &filepath)
{
	return json_serializer_to_file(self, filepath.data);
}