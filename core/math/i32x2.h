#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/math/f32.h"
#include "core/math/i32.h"

union I32x2
{
	struct
	{
		I32 x, y;
	};
	I32 components[2];

	inline I32 &
	operator[](U64 index)
	{
		validate(index < 2, "[MATH][I32x2]: Component index out of bounds.");
		return components[index];
	}

	inline const I32 &
	operator[](U64 index) const
	{
		validate(index < 2, "[MATH][I32x2]: Component index out of bounds.");
		return components[index];
	}
};

inline static I32x2
operator+(const I32x2 &a, const I32x2 &b)
{
	return I32x2{.x = a.x + b.x, .y = a.y + b.y};
}

inline static I32x2 &
operator+=(I32x2 &a, const I32x2 &b)
{
	a = a + b;
	return a;
}

inline static I32x2
operator-(const I32x2 &a)
{
	return I32x2{.x = -a.x, .y = -a.y};
}

inline static I32x2
operator-(const I32x2 &a, const I32x2 &b)
{
	return I32x2{.x = a.x - b.x, .y = a.y - b.y};
}

inline static I32x2 &
operator-=(I32x2 &a, const I32x2 &b)
{
	a = a - b;
	return a;
}

inline static I32x2
operator*(const I32x2 &a, I32 s)
{
	return I32x2{.x = a.x * s, .y = a.y * s};
}

inline static I32x2
operator*(I32 s, const I32x2 &a)
{
	return a * s;
}

inline static I32x2 &
operator*=(I32x2 &a, I32 s)
{
	a = a * s;
	return a;
}

inline static bool
operator==(const I32x2 &a, const I32x2 &b)
{
	return a.x == b.x && a.y == b.y;
}

inline static I32x2
i32x2_from_i32(I32 s)
{
	return I32x2{.x = s, .y = s};
}

inline static F32
i32x2_length(const I32x2 &a)
{
	return f32_sqrt((F32)a.x * (F32)a.x + (F32)a.y * (F32)a.y);
}

inline static I32
i32x2_length_squared(const I32x2 &a)
{
	return a.x * a.x + a.y * a.y;
}

inline static F32
i32x2_distance(const I32x2 &a, const I32x2 &b)
{
	F32 x = (F32)b.x - (F32)a.x;
	F32 y = (F32)b.y - (F32)a.y;
	return f32_sqrt(x * x + y * y);
}

inline static I32
i32x2_distance_squared(const I32x2 &a, const I32x2 &b)
{
	return i32x2_length_squared(b - a);
}

inline static I32x2
i32x2_abs(const I32x2 &a)
{
	return I32x2{.x = i32_abs(a.x), .y = i32_abs(a.y)};
}

inline static I32x2
i32x2_min(const I32x2 &a, const I32x2 &b)
{
	return I32x2{.x = i32_min(a.x, b.x), .y = i32_min(a.y, b.y)};
}

inline static I32x2
i32x2_max(const I32x2 &a, const I32x2 &b)
{
	return I32x2{.x = i32_max(a.x, b.x), .y = i32_max(a.y, b.y)};
}

inline static I32x2
i32x2_clamp(const I32x2 &v, const I32x2 &lo, const I32x2 &hi)
{
	return i32x2_min(i32x2_max(v, lo), hi);
}

inline static I32
i32x2_dot(const I32x2 &a, const I32x2 &b)
{
	return a.x * b.x + a.y * b.y;
}