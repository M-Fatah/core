#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/math/f32.h"
#include "core/math/u32.h"

union U32x2
{
	struct
	{
		U32 x, y;
	};
	U32 components[2];

	inline U32 &
	operator[](U64 index)
	{
		validate(index < 2, "[MATH][U32x2]: Component index out of bounds.");
		return components[index];
	}

	inline const U32 &
	operator[](U64 index) const
	{
		validate(index < 2, "[MATH][U32x2]: Component index out of bounds.");
		return components[index];
	}
};

inline static U32x2
operator+(const U32x2 &a, const U32x2 &b)
{
	return U32x2{.x = a.x + b.x, .y = a.y + b.y};
}

inline static U32x2 &
operator+=(U32x2 &a, const U32x2 &b)
{
	a = a + b;
	return a;
}

inline static U32x2
operator-(const U32x2 &a, const U32x2 &b)
{
	return U32x2{.x = a.x - b.x, .y = a.y - b.y};
}

inline static U32x2 &
operator-=(U32x2 &a, const U32x2 &b)
{
	a = a - b;
	return a;
}

inline static U32x2
operator*(const U32x2 &a, U32 s)
{
	return U32x2{.x = a.x * s, .y = a.y * s};
}

inline static U32x2
operator*(U32 s, const U32x2 &a)
{
	return a * s;
}

inline static U32x2 &
operator*=(U32x2 &a, U32 s)
{
	a = a * s;
	return a;
}

inline static bool
operator==(const U32x2 &a, const U32x2 &b)
{
	return a.x == b.x && a.y == b.y;
}

inline static U32x2
u32x2_from_u32(U32 s)
{
	return U32x2{.x = s, .y = s};
}

inline static F32
u32x2_length(const U32x2 &a)
{
	return f32_sqrt((F32)a.x * (F32)a.x + (F32)a.y * (F32)a.y);
}

inline static U32
u32x2_length_squared(const U32x2 &a)
{
	return a.x * a.x + a.y * a.y;
}

inline static U32x2
u32x2_min(const U32x2 &a, const U32x2 &b)
{
	return U32x2{.x = u32_min(a.x, b.x), .y = u32_min(a.y, b.y)};
}

inline static U32x2
u32x2_max(const U32x2 &a, const U32x2 &b)
{
	return U32x2{.x = u32_max(a.x, b.x), .y = u32_max(a.y, b.y)};
}

inline static U32x2
u32x2_clamp(const U32x2 &v, const U32x2 &lo, const U32x2 &hi)
{
	return u32x2_min(u32x2_max(v, lo), hi);
}

inline static U32
u32x2_dot(const U32x2 &a, const U32x2 &b)
{
	return a.x * b.x + a.y * b.y;
}