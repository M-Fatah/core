#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/math/f32.h"
#include "core/math/u32.h"

union U32x3
{
	struct
	{
		U32 x, y, z;
	};
	U32 components[3];

	inline U32 &
	operator[](U64 index)
	{
		validate(index < 3, "[MATH][U32x3]: Component index out of bounds.");
		return components[index];
	}

	inline const U32 &
	operator[](U64 index) const
	{
		validate(index < 3, "[MATH][U32x3]: Component index out of bounds.");
		return components[index];
	}
};

inline static U32x3
operator+(const U32x3 &a, const U32x3 &b)
{
	return U32x3{.x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z};
}

inline static U32x3 &
operator+=(U32x3 &a, const U32x3 &b)
{
	a = a + b;
	return a;
}

inline static U32x3
operator-(const U32x3 &a, const U32x3 &b)
{
	return U32x3{.x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z};
}

inline static U32x3 &
operator-=(U32x3 &a, const U32x3 &b)
{
	a = a - b;
	return a;
}

inline static U32x3
operator*(const U32x3 &a, U32 s)
{
	return U32x3{.x = a.x * s, .y = a.y * s, .z = a.z * s};
}

inline static U32x3
operator*(U32 s, const U32x3 &a)
{
	return a * s;
}

inline static U32x3 &
operator*=(U32x3 &a, U32 s)
{
	a = a * s;
	return a;
}

inline static bool
operator==(const U32x3 &a, const U32x3 &b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

inline static U32x3
u32x3_from_u32(U32 s)
{
	return U32x3{.x = s, .y = s, .z = s};
}

inline static F32
u32x3_length(const U32x3 &a)
{
	return f32_sqrt((F32)a.x * (F32)a.x + (F32)a.y * (F32)a.y + (F32)a.z * (F32)a.z);
}

inline static U32
u32x3_length_squared(const U32x3 &a)
{
	return a.x * a.x + a.y * a.y + a.z * a.z;
}

inline static U32x3
u32x3_min(const U32x3 &a, const U32x3 &b)
{
	return U32x3{.x = u32_min(a.x, b.x), .y = u32_min(a.y, b.y), .z = u32_min(a.z, b.z)};
}

inline static U32x3
u32x3_max(const U32x3 &a, const U32x3 &b)
{
	return U32x3{.x = u32_max(a.x, b.x), .y = u32_max(a.y, b.y), .z = u32_max(a.z, b.z)};
}

inline static U32x3
u32x3_clamp(const U32x3 &v, const U32x3 &lo, const U32x3 &hi)
{
	return u32x3_min(u32x3_max(v, lo), hi);
}

inline static U32
u32x3_dot(const U32x3 &a, const U32x3 &b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}