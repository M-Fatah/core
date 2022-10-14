#pragma once

#include "core/defines.h"

/*
	TODO:
	- [ ] Support versioning.
	- [ ] Support json serialization.
	- [ ] Add unittests for json serialization.
	- [ ] Define fixed size for char and bool, since they are compiler implementation specific and differ between compilers.
*/

#define SERIALIZE(T)          \
virtual void                  \
serialize(const T &data) = 0;

#define DESERIALIZE(T)        \
virtual void                  \
deserialize(T &data) = 0;

struct Serializer
{
	virtual
	~Serializer() = default;

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
	SERIALIZE(char)
	SERIALIZE(bool)

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
	DESERIALIZE(char)
	DESERIALIZE(bool)

	virtual void
	clear() = 0;
};

#undef SERIALIZE
#undef DESERIALIZE

template <typename T>
inline static void
serialize(Serializer *, const T &)
{
	static_assert(sizeof(T) == 0, "There is no `void serialize(Serializer *, const T &)` function overload defined for this type.");
}

template <typename T>
inline static void
deserialize(Serializer *, T &)
{
	static_assert(sizeof(T) == 0, "There is no `void deserialize(Serializer *, T &)` function overload defined for this type.");
}

#define SERIALIZE(T)                       \
inline static void                         \
serialize(Serializer *self, const T &data) \
{                                          \
    self->serialize(data);                 \
}

#define DESERIALIZE(T)                     \
inline static void                         \
deserialize(Serializer *self, T &data)     \
{                                          \
    self->deserialize(data);               \
}

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
SERIALIZE(char)
SERIALIZE(bool)

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
DESERIALIZE(char)
DESERIALIZE(bool)

#undef SERIALIZE
#undef DESERIALIZE