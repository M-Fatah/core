#include "core/serialization/binary_serializer.h"

#include "core/logger.h"
#include "core/memory/memory.h"
#include "core/containers/array.h"
#include "core/platform/platform.h"

struct Binary_Serializer_Context
{
	memory::Allocator *allocator;
	Array<u8> buffer;
	u64 serialization_offset;
	u64 deserialization_offset;
};

inline static u8 *
_binary_serializer_allocate(Binary_Serializer *self, u64 size)
{
	if (self->ctx->serialization_offset + size >= self->ctx->buffer.count)
	{
		auto added_capacity = size;
		if (self->ctx->buffer.count + size >= self->ctx->buffer.capacity)
			added_capacity = self->ctx->buffer.capacity;
		array_reserve(self->ctx->buffer, added_capacity);
	}
	return self->ctx->buffer.data + self->ctx->serialization_offset;
}

template <typename T>
inline static void
_binary_serializer_serialize(Binary_Serializer *self, const T &data)
{
	::memcpy(_binary_serializer_allocate(self, sizeof(T)), &data, sizeof(T));
	self->ctx->serialization_offset += sizeof(T);
	self->ctx->buffer.count += sizeof(T);
}

template <typename T>
inline static void
_binary_serializer_deserialize(Binary_Serializer *self, T &data)
{
	ASSERT(self->ctx->deserialization_offset + sizeof(T) <= self->ctx->buffer.count, "[BINARY_SERIALIZER]: Out of bounds deserialization.");
	::memcpy(&data, &self->ctx->buffer[self->ctx->deserialization_offset], sizeof(T));
	self->ctx->deserialization_offset += sizeof(T);
}

// API.
Binary_Serializer::Binary_Serializer(memory::Allocator *allocator)
{
	Binary_Serializer *self           = this;
	self->ctx                         = memory::allocate<Binary_Serializer_Context>(allocator);
	self->ctx->allocator              = allocator;
	self->ctx->buffer                 = array_with_capacity<u8>(4 * 1024 * 1024ULL, allocator);
	self->ctx->serialization_offset   = 0;
	self->ctx->deserialization_offset = 0;
}

Binary_Serializer::~Binary_Serializer()
{
	Binary_Serializer *self = this;
	array_deinit(self->ctx->buffer);
	memory::Allocator *allocator = self->ctx->allocator;
	memory::deallocate(allocator, self->ctx);
	self->ctx = nullptr;
}

void
Binary_Serializer::serialize(i8 data)
{
	Binary_Serializer *self = this;
	_binary_serializer_serialize(self, data);
}

void
Binary_Serializer::serialize(i16 data)
{
	Binary_Serializer *self = this;
	_binary_serializer_serialize(self, data);
}

void
Binary_Serializer::serialize(i32 data)
{
	Binary_Serializer *self = this;
	_binary_serializer_serialize(self, data);
}

void
Binary_Serializer::serialize(i64 data)
{
	Binary_Serializer *self = this;
	_binary_serializer_serialize(self, data);
}

void
Binary_Serializer::serialize(u8 data)
{
	Binary_Serializer *self = this;
	_binary_serializer_serialize(self, data);
}

void
Binary_Serializer::serialize(u16 data)
{
	Binary_Serializer *self = this;
	_binary_serializer_serialize(self, data);
}

void
Binary_Serializer::serialize(u32 data)
{
	Binary_Serializer *self = this;
	_binary_serializer_serialize(self, data);
}

void
Binary_Serializer::serialize(u64 data)
{
	Binary_Serializer *self = this;
	_binary_serializer_serialize(self, data);
}

void
Binary_Serializer::serialize(f32 data)
{
	Binary_Serializer *self = this;
	_binary_serializer_serialize(self, data);
}

void
Binary_Serializer::serialize(f64 data)
{
	Binary_Serializer *self = this;
	_binary_serializer_serialize(self, data);
}

void
Binary_Serializer::serialize(bool data)
{
	Binary_Serializer *self = this;
	_binary_serializer_serialize(self, data);
}

void
Binary_Serializer::serialize(char data)
{
	Binary_Serializer *self = this;
	_binary_serializer_serialize(self, data);
}

void
Binary_Serializer::deserialize(i8 &data)
{
	Binary_Serializer *self = this;
	_binary_serializer_deserialize(self, data);
}

void
Binary_Serializer::deserialize(i16 &data)
{
	Binary_Serializer *self = this;
	_binary_serializer_deserialize(self, data);
}

void
Binary_Serializer::deserialize(i32 &data)
{
	Binary_Serializer *self = this;
	_binary_serializer_deserialize(self, data);
}

void
Binary_Serializer::deserialize(i64 &data)
{
	Binary_Serializer *self = this;
	_binary_serializer_deserialize(self, data);
}

void
Binary_Serializer::deserialize(u8 &data)
{
	Binary_Serializer *self = this;
	_binary_serializer_deserialize(self, data);
}

void
Binary_Serializer::deserialize(u16 &data)
{
	Binary_Serializer *self = this;
	_binary_serializer_deserialize(self, data);
}

void
Binary_Serializer::deserialize(u32 &data)
{
	Binary_Serializer *self = this;
	_binary_serializer_deserialize(self, data);
}

void
Binary_Serializer::deserialize(u64 &data)
{
	Binary_Serializer *self = this;
	_binary_serializer_deserialize(self, data);
}

void
Binary_Serializer::deserialize(f32 &data)
{
	Binary_Serializer *self = this;
	_binary_serializer_deserialize(self, data);
}

void
Binary_Serializer::deserialize(f64 &data)
{
	Binary_Serializer *self = this;
	_binary_serializer_deserialize(self, data);
}

void
Binary_Serializer::deserialize(bool &data)
{
	Binary_Serializer *self = this;
	_binary_serializer_deserialize(self, data);
}

void
Binary_Serializer::deserialize(char &data)
{
	Binary_Serializer *self = this;
	_binary_serializer_deserialize(self, data);
}

void
Binary_Serializer::clear()
{
	Binary_Serializer *self = this;
	//
	// TODO:
	// This will get invalidated if we use an arena allocator and clear that arena,
	//    while binary serializer is in use.
	// Since the arena might get resized, if we then clear it, it frees the underlying blocks,
	//    and then allocates a new one that of the total size of the previous blocks combined and with a different pointer,
	//    which will make our array's data pointer invalid.
	//
	array_clear(self->ctx->buffer);
	self->ctx->serialization_offset   = 0;
	self->ctx->deserialization_offset = 0;
}

Binary_Serializer *
binary_serializer_init(memory::Allocator *allocator)
{
	return memory::allocate_and_call_constructor<Binary_Serializer>(allocator, allocator);
}

Result<Binary_Serializer *>
binary_serializer_from_file(const char *filepath, memory::Allocator *allocator)
{
	Binary_Serializer *self = binary_serializer_init(allocator);
	auto error = binary_serializer_from_file(self, filepath);
	if (error)
	{
		binary_serializer_deinit(self);
		return error;
	}
	return self;
}

void
binary_serializer_deinit(Binary_Serializer *self)
{
	if (self)
	{
		memory::Allocator *allocator = self->ctx->allocator;
		memory::deallocate_and_call_destructor(allocator, self);
	}
}

void
binary_serializer_clear(Binary_Serializer *self)
{
	self->clear();
}

Error
binary_serializer_from_file(Binary_Serializer *self, const char *filepath)
{
	ASSERT(self->ctx, "[BINARY_SERIALIZER]: Cannot read file data to uninitialized serializer.");
	u64 file_size = platform_file_size(filepath);
	u64 read_size = platform_file_read(filepath, Platform_Memory{_binary_serializer_allocate(self, file_size), file_size});
	if (read_size != file_size)
		return Error{"[BINARY_SERIALIZER]: Could not read file '{}', read size is '{}', but file size is '{}'.", filepath, read_size, file_size};
	self->ctx->serialization_offset = file_size;
	self->ctx->buffer.count         = file_size;
	return Error{};
}

Error
binary_serializer_to_file(Binary_Serializer *self, const char *filepath)
{
	u64 written_size = platform_file_write(filepath, Platform_Memory{self->ctx->buffer.data, self->ctx->buffer.count});
	if (written_size != self->ctx->buffer.count)
		return Error{"[BINARY_SERIALIZER]: Could not write file '{}', written size is '{}', but total size is '{}'.", filepath, written_size, self->ctx->buffer.count};
	return Error{};
}