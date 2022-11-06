#include "core/serialization/serializer.h"

#include "core/defines.h"
#include "core/result.h"
#include "core/containers/string.h"

#define SERIALIZE(T)                             \
void                                             \
serialize(const char *name, T data) override;

#define DESERIALIZE(T)                           \
void                                             \
deserialize(const char *name, T &data) override;

struct Binary_Serializer : Serializer
{
	struct Binary_Serializer_Context *ctx;

	Binary_Serializer(memory::Allocator *allocator = memory::heap_allocator());

	~Binary_Serializer() override;

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
	clear() override;
};

#undef SERIALIZE
#undef DESERIALIZE

CORE_API Binary_Serializer *
binary_serializer_init(memory::Allocator *allocator = memory::heap_allocator());

CORE_API Result<Binary_Serializer *>
binary_serializer_from_file(const char *filepath, memory::Allocator *allocator = memory::heap_allocator());

inline static Result<Binary_Serializer *>
binary_serializer_from_file(const String &filepath, memory::Allocator *allocator = memory::heap_allocator())
{
	return binary_serializer_from_file(filepath.data, allocator);
}

CORE_API void
binary_serializer_deinit(Binary_Serializer *self);

// TODO:
template <typename T>
inline static void
binary_serializer_serialize(Binary_Serializer *self, const T &data)
{
	serialize(self, "", data);
}

// TODO:
template <typename T>
inline static void
binary_serializer_deserialize(Binary_Serializer *self, T &data)
{
	deserialize(self, "", data);
}

CORE_API void
binary_serializer_clear(Binary_Serializer *self);

CORE_API Error
binary_serializer_from_file(Binary_Serializer *self, const char *filepath);

inline static Error
binary_serializer_from_file(Binary_Serializer *self, const String &filepath)
{
	return binary_serializer_from_file(self, filepath.data);
}

CORE_API Error
binary_serializer_to_file(Binary_Serializer *self, const char *filepath);

inline static Error
binary_serializer_to_file(Binary_Serializer *self, const String &filepath)
{
	return binary_serializer_to_file(self, filepath.data);
}