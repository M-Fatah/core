#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/math/f64.h"

union F64x3
{
	struct
	{
		F64 x, y, z;
	};
	F64 components[3];

	inline F64 &
	operator[](U64 index)
	{
		validate(index < 3, "[MATH][F64x3]: Component index out of bounds.");
		return components[index];
	}

	inline const F64 &
	operator[](U64 index) const
	{
		validate(index < 3, "[MATH][F64x3]: Component index out of bounds.");
		return components[index];
	}
};

inline static F64x3
operator+(const F64x3 &a, const F64x3 &b)
{
	return F64x3{.x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z};
}

inline static F64x3 &
operator+=(F64x3 &a, const F64x3 &b)
{
	a = a + b;
	return a;
}

inline static F64x3
operator-(const F64x3 &a)
{
	return F64x3{.x = -a.x, .y = -a.y, .z = -a.z};
}

inline static F64x3
operator-(const F64x3 &a, const F64x3 &b)
{
	return F64x3{.x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z};
}

inline static F64x3 &
operator-=(F64x3 &a, const F64x3 &b)
{
	a = a - b;
	return a;
}

inline static F64x3
operator*(const F64x3 &a, F64 s)
{
	return F64x3{.x = a.x * s, .y = a.y * s, .z = a.z * s};
}

inline static F64x3
operator*(F64 s, const F64x3 &a)
{
	return a * s;
}

inline static F64x3 &
operator*=(F64x3 &a, F64 s)
{
	a = a * s;
	return a;
}

inline static F64x3
operator/(const F64x3 &a, F64 s)
{
	validate(s != 0.0, "[MATH][F64x3]: scalar divisor must be non-zero.");
	return a * (1.0 / s);
}

inline static F64x3 &
operator/=(F64x3 &a, F64 s)
{
	a = a / s;
	return a;
}

inline static bool
operator==(const F64x3 &a, const F64x3 &b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

inline static F64x3
f64x3_from_f64(F64 s)
{
	return F64x3{.x = s, .y = s, .z = s};
}

inline static F64x3
f64x3_normalize(const F64x3 &a)
{
	F64 length_squared = a.x * a.x + a.y * a.y + a.z * a.z;
	validate(length_squared != 0.0, "[MATH][F64x3]: Cannot normalize zero-length vector.");
	return a / f64_sqrt(length_squared);
}

inline static F64
f64x3_length(const F64x3 &a)
{
	return f64_sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

inline static F64
f64x3_length_squared(const F64x3 &a)
{
	return a.x * a.x + a.y * a.y + a.z * a.z;
}

inline static F64
f64x3_distance(const F64x3 &a, const F64x3 &b)
{
	return f64_sqrt(f64x3_length_squared(b - a));
}

inline static F64
f64x3_distance_squared(const F64x3 &a, const F64x3 &b)
{
	return f64x3_length_squared(b - a);
}

inline static F64x3
f64x3_min(const F64x3 &a, const F64x3 &b)
{
	return F64x3{.x = f64_min(a.x, b.x), .y = f64_min(a.y, b.y), .z = f64_min(a.z, b.z)};
}

inline static F64x3
f64x3_max(const F64x3 &a, const F64x3 &b)
{
	return F64x3{.x = f64_max(a.x, b.x), .y = f64_max(a.y, b.y), .z = f64_max(a.z, b.z)};
}

inline static F64x3
f64x3_clamp(const F64x3 &v, const F64x3 &lo, const F64x3 &hi)
{
	return f64x3_min(f64x3_max(v, lo), hi);
}

inline static bool
f64x3_approx_equal(const F64x3 &a, const F64x3 &b, F64 epsilon)
{
	return f64_approx_equal(a.x, b.x, epsilon)
		&& f64_approx_equal(a.y, b.y, epsilon)
		&& f64_approx_equal(a.z, b.z, epsilon);
}

inline static F64
f64x3_dot(const F64x3 &a, const F64x3 &b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline static F64x3
f64x3_cross(const F64x3 &a, const F64x3 &b)
{
	return F64x3 {
		.x = a.y * b.z - a.z * b.y,
		.y = a.z * b.x - a.x * b.z,
		.z = a.x * b.y - a.y * b.x
	};
}

inline static F64x3
f64x3_reflect(const F64x3 &v, const F64x3 &normal)
{
	return v - 2.0 * f64x3_dot(v, normal) * normal;
}

inline static F64x3
f64x3_project(const F64x3 &v, const F64x3 &onto)
{
	F64 length_squared = f64x3_length_squared(onto);
	validate(length_squared != 0.0, "[MATH][F64x3]: Cannot project onto zero-length vector.");
	return onto * (f64x3_dot(v, onto) / length_squared);
}

inline static F64x3
f64x3_reject(const F64x3 &v, const F64x3 &onto)
{
	return v - f64x3_project(v, onto);
}

inline static F64x3
f64x3_lerp(const F64x3 &a, const F64x3 &b, F64 t)
{
	return F64x3{.x = f64_lerp(a.x, b.x, t), .y = f64_lerp(a.y, b.y, t), .z = f64_lerp(a.z, b.z, t)};
}