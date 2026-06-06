#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/math/i32.h"
#include "core/math/f32.h"

union I32x3
{
	struct
	{
		I32 x, y, z;
	};
	I32 components[3];

	inline I32 &
	operator[](U64 index)
	{
		validate(index < 3, "[MATH][I32x3]: Component index out of bounds.");
		return components[index];
	}

	inline const I32 &
	operator[](U64 index) const
	{
		validate(index < 3, "[MATH][I32x3]: Component index out of bounds.");
		return components[index];
	}
};

inline static I32x3
operator+(const I32x3 &a, const I32x3 &b)
{
	return I32x3{.x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z};
}

inline static I32x3 &
operator+=(I32x3 &a, const I32x3 &b)
{
	a = a + b;
	return a;
}

inline static I32x3
operator-(const I32x3 &a)
{
	return I32x3{.x = -a.x, .y = -a.y, .z = -a.z};
}

inline static I32x3
operator-(const I32x3 &a, const I32x3 &b)
{
	return I32x3{.x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z};
}

inline static I32x3 &
operator-=(I32x3 &a, const I32x3 &b)
{
	a = a - b;
	return a;
}

inline static I32x3
operator*(const I32x3 &a, I32 s)
{
	return I32x3{.x = a.x * s, .y = a.y * s, .z = a.z * s};
}

inline static I32x3
operator*(I32 s, const I32x3 &a)
{
	return a * s;
}

inline static I32x3 &
operator*=(I32x3 &a, I32 s)
{
	a = a * s;
	return a;
}

inline static bool
operator==(const I32x3 &a, const I32x3 &b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

inline static I32x3
i32x3_from_i32(I32 s)
{
	return I32x3{.x = s, .y = s, .z = s};
}

inline static F32
i32x3_length(const I32x3 &a)
{
	return f32_sqrt((F32)a.x * (F32)a.x + (F32)a.y * (F32)a.y + (F32)a.z * (F32)a.z);
}

inline static I32
i32x3_length_squared(const I32x3 &a)
{
	return a.x * a.x + a.y * a.y + a.z * a.z;
}

inline static F32
i32x3_distance(const I32x3 &a, const I32x3 &b)
{
	F32 x = (F32)b.x - (F32)a.x;
	F32 y = (F32)b.y - (F32)a.y;
	F32 z = (F32)b.z - (F32)a.z;
	return f32_sqrt(x * x + y * y + z * z);
}

inline static I32
i32x3_distance_squared(const I32x3 &a, const I32x3 &b)
{
	return i32x3_length_squared(b - a);
}

inline static I32x3
i32x3_abs(const I32x3 &a)
{
	return I32x3{.x = i32_abs(a.x), .y = i32_abs(a.y), .z = i32_abs(a.z)};
}

inline static I32x3
i32x3_min(const I32x3 &a, const I32x3 &b)
{
	return I32x3{.x = i32_min(a.x, b.x), .y = i32_min(a.y, b.y), .z = i32_min(a.z, b.z)};
}

inline static I32x3
i32x3_max(const I32x3 &a, const I32x3 &b)
{
	return I32x3{.x = i32_max(a.x, b.x), .y = i32_max(a.y, b.y), .z = i32_max(a.z, b.z)};
}

inline static I32x3
i32x3_clamp(const I32x3 &v, const I32x3 &lo, const I32x3 &hi)
{
	return i32x3_min(i32x3_max(v, lo), hi);
}

inline static I32
i32x3_dot(const I32x3 &a, const I32x3 &b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}