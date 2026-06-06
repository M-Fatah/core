#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/math/f32.h"

union F32x3
{
	struct
	{
		F32 x, y, z;
	};
	F32 components[3];

	inline F32 &
	operator[](U64 index)
	{
		validate(index < 3, "[MATH][F32x3]: Component index out of bounds.");
		return components[index];
	}

	inline const F32 &
	operator[](U64 index) const
	{
		validate(index < 3, "[MATH][F32x3]: Component index out of bounds.");
		return components[index];
	}
};

inline static F32x3
operator+(const F32x3 &a, const F32x3 &b)
{
	return F32x3{.x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z};
}

inline static F32x3 &
operator+=(F32x3 &a, const F32x3 &b)
{
	a = a + b;
	return a;
}

inline static F32x3
operator-(const F32x3 &a)
{
	return F32x3{.x = -a.x, .y = -a.y, .z = -a.z};
}

inline static F32x3
operator-(const F32x3 &a, const F32x3 &b)
{
	return F32x3{.x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z};
}

inline static F32x3 &
operator-=(F32x3 &a, const F32x3 &b)
{
	a = a - b;
	return a;
}

inline static F32x3
operator*(const F32x3 &a, F32 s)
{
	return F32x3{.x = a.x * s, .y = a.y * s, .z = a.z * s};
}

inline static F32x3
operator*(F32 s, const F32x3 &a)
{
	return a * s;
}

inline static F32x3 &
operator*=(F32x3 &a, F32 s)
{
	a = a * s;
	return a;
}

inline static F32x3
operator/(const F32x3 &a, F32 s)
{
	validate(s != 0.0f, "[MATH][F32x3]: Scalar divisor must be non-zero.");
	return a * (1.0f / s);
}

inline static F32x3 &
operator/=(F32x3 &a, F32 s)
{
	a = a / s;
	return a;
}

inline static bool
operator==(const F32x3 &a, const F32x3 &b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

inline static F32x3
f32x3_from_f32(F32 s)
{
	return F32x3{.x = s, .y = s, .z = s};
}

inline static F32x3
f32x3_normalize(const F32x3 &a)
{
	F32 length_squared = a.x * a.x + a.y * a.y + a.z * a.z;
	validate(length_squared != 0.0f, "[MATH][F32x3]: Cannot normalize zero-length vector.");
	return a / f32_sqrt(length_squared);
}

inline static F32
f32x3_length(const F32x3 &a)
{
	return f32_sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

inline static F32
f32x3_length_squared(const F32x3 &a)
{
	return a.x * a.x + a.y * a.y + a.z * a.z;
}

inline static F32
f32x3_distance(const F32x3 &a, const F32x3 &b)
{
	return f32_sqrt(f32x3_length_squared(b - a));
}

inline static F32
f32x3_distance_squared(const F32x3 &a, const F32x3 &b)
{
	return f32x3_length_squared(b - a);
}

inline static F32x3
f32x3_min(const F32x3 &a, const F32x3 &b)
{
	return F32x3{.x = f32_min(a.x, b.x), .y = f32_min(a.y, b.y), .z = f32_min(a.z, b.z)};
}

inline static F32x3
f32x3_max(const F32x3 &a, const F32x3 &b)
{
	return F32x3{.x = f32_max(a.x, b.x), .y = f32_max(a.y, b.y), .z = f32_max(a.z, b.z)};
}

inline static F32x3
f32x3_clamp(const F32x3 &v, const F32x3 &lo, const F32x3 &hi)
{
	return f32x3_min(f32x3_max(v, lo), hi);
}

inline static bool
f32x3_approx_equal(const F32x3 &a, const F32x3 &b, F32 epsilon)
{
	return f32_approx_equal(a.x, b.x, epsilon)
		&& f32_approx_equal(a.y, b.y, epsilon)
		&& f32_approx_equal(a.z, b.z, epsilon);
}

inline static F32
f32x3_dot(const F32x3 &a, const F32x3 &b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline static F32x3
f32x3_cross(const F32x3 &a, const F32x3 &b)
{
	return F32x3 {
		.x = a.y * b.z - a.z * b.y,
		.y = a.z * b.x - a.x * b.z,
		.z = a.x * b.y - a.y * b.x
	};
}

inline static F32x3
f32x3_reflect(const F32x3 &v, const F32x3 &normal)
{
	return v - 2.0f * f32x3_dot(v, normal) * normal;
}

inline static F32x3
f32x3_project(const F32x3 &v, const F32x3 &onto)
{
	F32 length_squared = f32x3_length_squared(onto);
	validate(length_squared != 0.0f, "[MATH][F32x3]: Cannot project onto zero-length vector.");
	return onto * (f32x3_dot(v, onto) / length_squared);
}

inline static F32x3
f32x3_reject(const F32x3 &v, const F32x3 &onto)
{
	return v - f32x3_project(v, onto);
}

inline static F32x3
f32x3_lerp(const F32x3 &a, const F32x3 &b, F32 t)
{
	return F32x3{.x = f32_lerp(a.x, b.x, t), .y = f32_lerp(a.y, b.y, t), .z = f32_lerp(a.z, b.z, t)};
}