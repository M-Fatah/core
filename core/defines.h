#pragma once

#include <stdint.h>
#include <float.h>

#define UNUSED(expr) ((void)(expr))

#define I8_MIN  INT8_MIN
#define I8_MAX  INT8_MAX
#define I16_MIN INT16_MIN
#define I16_MAX INT16_MAX
#define I32_MIN INT32_MIN
#define I32_MAX INT32_MAX
#define I64_MIN INT64_MIN
#define I64_MAX INT64_MAX

#define U8_MAX  UINT8_MAX
#define U16_MAX UINT16_MAX
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX

#define F32_MIN FLT_MIN
#define F32_MAX FLT_MAX
#define F64_MIN DBL_MIN
#define F64_MAX DBL_MAX

typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

typedef float     f32;
typedef double    f64;

typedef intptr_t  iptr;
typedef uintptr_t uptr;

namespace memory { struct Allocator; }

template <typename T>
inline static T
clone(const T &, memory::Allocator *)
{
	static_assert(sizeof(T) == 0, "There is no `T clone(const T &, memory::Allocator *)` function overload defined for this type.");
	return {};
}

template <typename T>
inline static void
destroy(T &)
{
	static_assert(sizeof(T) == 0, "There is no `void destroy(T &)` function overload defined for this type.");
}