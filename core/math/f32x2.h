#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/math/f32.h"

union F32x2
{
	struct
	{
		F32 x, y;
	};
	F32 components[2];

	inline F32 &
	operator[](U64 index)
	{
		validate(index < 2, "[MATH][F32x2]: Component index out of bounds.");
		return components[index];
	}

	inline const F32 &
	operator[](U64 index) const
	{
		validate(index < 2, "[MATH][F32x2]: Component index out of bounds.");
		return components[index];
	}
};

inline static F32x2
operator+(F32x2 a, F32x2 b)
{
	return F32x2{.x = a.x + b.x, .y = a.y + b.y};
}

inline static F32x2 &
operator+=(F32x2 &a, F32x2 b)
{
	a = a + b;
	return a;
}

inline static F32x2
operator-(F32x2 a)
{
	return F32x2{.x = -a.x, .y = -a.y};
}

inline static F32x2
operator-(F32x2 a, F32x2 b)
{
	return F32x2{.x = a.x - b.x, .y = a.y - b.y};
}

inline static F32x2 &
operator-=(F32x2 &a, F32x2 b)
{
	a = a - b;
	return a;
}

inline static F32x2
operator*(F32x2 a, F32 s)
{
	return F32x2{.x = a.x * s, .y = a.y * s};
}

inline static F32x2
operator*(F32 s, F32x2 a)
{
	return a * s;
}

inline static F32x2 &
operator*=(F32x2 &a, F32 s)
{
	a = a * s;
	return a;
}

inline static F32x2
operator/(F32x2 a, F32 s)
{
	validate(s != 0.0f, "[MATH][F32x2]: Scalar divisor must be non-zero.");
	return a * (1.0f / s);
}

inline static F32x2 &
operator/=(F32x2 &a, F32 s)
{
	a = a / s;
	return a;
}

inline static bool
operator==(F32x2 a, F32x2 b)
{
	return a.x == b.x && a.y == b.y;
}

inline static F32x2
f32x2_from_f32(F32 s)
{
	return F32x2{.x = s, .y = s};
}

inline static F32x2
f32x2_normalize(F32x2 a)
{
	F32 length_squared = a.x * a.x + a.y * a.y;
	validate(length_squared != 0.0f, "[MATH][F32x2]: Cannot normalize zero-length vector.");
	return a / f32_sqrt(length_squared);
}

inline static F32
f32x2_length(F32x2 a)
{
	return f32_sqrt(a.x * a.x + a.y * a.y);
}

inline static F32
f32x2_length_squared(F32x2 a)
{
	return a.x * a.x + a.y * a.y;
}

inline static F32
f32x2_distance(F32x2 a, F32x2 b)
{
	return f32_sqrt(f32x2_length_squared(b - a));
}

inline static F32
f32x2_distance_squared(F32x2 a, F32x2 b)
{
	return f32x2_length_squared(b - a);
}

inline static F32x2
f32x2_min(F32x2 a, F32x2 b)
{
	return F32x2{.x = f32_min(a.x, b.x), .y = f32_min(a.y, b.y)};
}

inline static F32x2
f32x2_max(F32x2 a, F32x2 b)
{
	return F32x2{.x = f32_max(a.x, b.x), .y = f32_max(a.y, b.y)};
}

inline static F32x2
f32x2_clamp(F32x2 v, F32x2 a, F32x2 b)
{
	return f32x2_min(f32x2_max(v, a), b);
}

inline static bool
f32x2_approx_equal(F32x2 a, F32x2 b, F32 epsilon)
{
	return f32_approx_equal(a.x, b.x, epsilon) && f32_approx_equal(a.y, b.y, epsilon);
}

inline static F32
f32x2_dot(F32x2 a, F32x2 b)
{
	return a.x * b.x + a.y * b.y;
}

inline static F32
f32x2_cross(F32x2 a, F32x2 b)
{
	return a.x * b.y - a.y * b.x;
}

inline static F32x2
f32x2_reflect(F32x2 v, F32x2 normal)
{
	return v - 2.0f * f32x2_dot(v, normal) * normal;
}

inline static F32x2
f32x2_project(F32x2 v, F32x2 onto)
{
	F32 length_squared = f32x2_length_squared(onto);
	validate(length_squared != 0.0f, "[MATH][F32x2]: Cannot project onto zero-length vector.");
	return onto * (f32x2_dot(v, onto) / length_squared);
}

inline static F32x2
f32x2_reject(F32x2 v, F32x2 onto)
{
	return v - f32x2_project(v, onto);
}

inline static F32x2
f32x2_lerp(F32x2 a, F32x2 b, F32 t)
{
	return F32x2{.x = f32_lerp(a.x, b.x, t), .y = f32_lerp(a.y, b.y, t)};
}