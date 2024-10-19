#pragma once

#include <stdint.h>
#include <float.h>

#define CONCATENATE(ARG1, ARG2) _CONCATENATE(ARG1, ARG2)
#define _CONCATENATE(ARG1, ARG2) ARG1##ARG2

#define IF(CONDITION) CONCATENATE(_IF_, CONDITION)
#define _IF_0(TRUE, FALSE) FALSE
#define _IF_1(TRUE, FALSE) TRUE

#define HAS_PARENTHESIS(ARG) _HAS_PARENTHESIS(_HAS_PARENTHESIS_PROBE ARG)
#define _HAS_PARENTHESIS(...) _HAS_PARENTHESIS_CHECK_N(__VA_ARGS__, 0)
#define _HAS_PARENTHESIS_CHECK_N(ARG, N, ...) N
#define _HAS_PARENTHESIS_PROBE(...) 0, 1

#define COUNT_OF(...) _COUNT_OF(__VA_ARGS__, _COUNT_OF_RSEQ())
#define _COUNT_OF(...) _COUNT_OF_CHECK_N(__VA_ARGS__)
#define _COUNT_OF_CHECK_N(                             \
     _01, _02, _03, _04, _05, _06, _07, _08, _09, _10, \
     _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
     _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
     _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
     _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
     _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
     _61, _62, _63, _64, N,...) N
#define _COUNT_OF_RSEQ()                               \
     64, 63, 62, 61, 60,                               \
     59, 58, 57, 56, 55, 54, 53, 52, 51, 50,           \
     49, 48, 47, 46, 45, 44, 43, 42, 41, 40,           \
     39, 38, 37, 36, 35, 34, 33, 32, 31, 30,           \
     29, 28, 27, 26, 25, 24, 23, 22, 21, 20,           \
     19, 18, 17, 16, 15, 14, 13, 12, 11, 10,           \
     09, 08, 07, 06, 05, 04, 03, 02, 01, 00

#define FOR_EACH(ACTION, ...) CONCATENATE(_FOR_EACH, COUNT_OF(__VA_ARGS__))(ACTION, __VA_ARGS__)
#define _FOR_EACH16(ACTION, ARG, ...) ACTION(ARG), _FOR_EACH15(ACTION, __VA_ARGS__)
#define _FOR_EACH15(ACTION, ARG, ...) ACTION(ARG), _FOR_EACH14(ACTION, __VA_ARGS__)
#define _FOR_EACH14(ACTION, ARG, ...) ACTION(ARG), _FOR_EACH13(ACTION, __VA_ARGS__)
#define _FOR_EACH13(ACTION, ARG, ...) ACTION(ARG), _FOR_EACH12(ACTION, __VA_ARGS__)
#define _FOR_EACH12(ACTION, ARG, ...) ACTION(ARG), _FOR_EACH11(ACTION, __VA_ARGS__)
#define _FOR_EACH11(ACTION, ARG, ...) ACTION(ARG), _FOR_EACH10(ACTION, __VA_ARGS__)
#define _FOR_EACH10(ACTION, ARG, ...) ACTION(ARG), _FOR_EACH09(ACTION, __VA_ARGS__)
#define _FOR_EACH09(ACTION, ARG, ...) ACTION(ARG), _FOR_EACH08(ACTION, __VA_ARGS__)
#define _FOR_EACH08(ACTION, ARG, ...) ACTION(ARG), _FOR_EACH07(ACTION, __VA_ARGS__)
#define _FOR_EACH07(ACTION, ARG, ...) ACTION(ARG), _FOR_EACH06(ACTION, __VA_ARGS__)
#define _FOR_EACH06(ACTION, ARG, ...) ACTION(ARG), _FOR_EACH05(ACTION, __VA_ARGS__)
#define _FOR_EACH05(ACTION, ARG, ...) ACTION(ARG), _FOR_EACH04(ACTION, __VA_ARGS__)
#define _FOR_EACH04(ACTION, ARG, ...) ACTION(ARG), _FOR_EACH03(ACTION, __VA_ARGS__)
#define _FOR_EACH03(ACTION, ARG, ...) ACTION(ARG), _FOR_EACH02(ACTION, __VA_ARGS__)
#define _FOR_EACH02(ACTION, ARG, ...) ACTION(ARG), _FOR_EACH01(ACTION, __VA_ARGS__)
#define _FOR_EACH01(ACTION, ARG, ...) ACTION(ARG)

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
inline static constexpr T
clone(const T &, memory::Allocator *)
{
	static_assert(sizeof(T) == 0, "There is no `T clone(const T &, memory::Allocator *)` function overload defined for this type.");
	return {};
}

template <typename T>
inline static constexpr void
destroy(T &)
{
	static_assert(sizeof(T) == 0, "There is no `void destroy(T &)` function overload defined for this type.");
}

template <typename T, u64 N>
inline static u64
count_of(const T (&)[N])
{
	return N;
}

template <typename ...TArgs>
inline static constexpr void
unused(const TArgs &...)
{

}