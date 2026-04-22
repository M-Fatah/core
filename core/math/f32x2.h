#pragma once

#include <core/defines.h>
#include <core/math/f32.h>

// ============================================================================
// F32x2 — 2D F32 vector. Scalar (SIMD would be overkill for 2 lanes).
// ============================================================================

struct F32x2
{
	F32 x, y;
};

// ---- Operators -------------------------------------------------------------

inline static F32x2
operator+(const F32x2 &a, const F32x2 &b)
{
	return F32x2{a.x + b.x, a.y + b.y};
}

inline static F32x2 &
operator+=(F32x2 &a, const F32x2 &b)
{
	a = a + b;
	return a;
}

inline static F32x2
operator-(const F32x2 &a)
{
	return F32x2{-a.x, -a.y};
}

inline static F32x2
operator-(const F32x2 &a, const F32x2 &b)
{
	return F32x2{a.x - b.x, a.y - b.y};
}

inline static F32x2 &
operator-=(F32x2 &a, const F32x2 &b)
{
	a = a - b;
	return a;
}

inline static F32x2
operator*(const F32x2 &a, F32 s)
{
	return F32x2{a.x * s, a.y * s};
}

inline static F32x2
operator*(F32 s, const F32x2 &a)
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
operator/(const F32x2 &a, F32 s)
{
	return a * (1.0f / s);
}

inline static F32x2 &
operator/=(F32x2 &a, F32 s)
{
	a = a / s;
	return a;
}

inline static bool
operator==(const F32x2 &a, const F32x2 &b)
{
	return a.x == b.x && a.y == b.y;
}

// ---- Free functions --------------------------------------------------------

inline static F32x2
f32x2_from_f32(F32 s)
{
	return F32x2{s, s};
}

inline static F32
f32x2_dot(const F32x2 &a, const F32x2 &b)
{
	return a.x * b.x + a.y * b.y;
}

// 2D "cross product" — scalar z-component of the 3D cross. Useful for orientation tests.
inline static F32
f32x2_cross(const F32x2 &a, const F32x2 &b)
{
	return a.x * b.y - a.y * b.x;
}

inline static F32
f32x2_length_squared(const F32x2 &a)
{
	return a.x * a.x + a.y * a.y;
}

inline static F32
f32x2_length(const F32x2 &a)
{
	return f32_sqrt(f32x2_length_squared(a));
}

inline static F32x2
f32x2_normalize(const F32x2 &a)
{
	return a / f32x2_length(a);
}

inline static F32x2
f32x2_min(const F32x2 &a, const F32x2 &b)
{
	return F32x2{f32_min(a.x, b.x), f32_min(a.y, b.y)};
}

inline static F32x2
f32x2_max(const F32x2 &a, const F32x2 &b)
{
	return F32x2{f32_max(a.x, b.x), f32_max(a.y, b.y)};
}

inline static F32x2
f32x2_lerp(const F32x2 &a, const F32x2 &b, F32 t)
{
	return F32x2{f32_lerp(a.x, b.x, t), f32_lerp(a.y, b.y, t)};
}

inline static F32x2
f32x2_clamp(const F32x2 &v, const F32x2 &lo, const F32x2 &hi)
{
	return f32x2_min(f32x2_max(v, lo), hi);
}

inline static bool
f32x2_approx_equal(const F32x2 &a, const F32x2 &b, F32 epsilon)
{
	return f32_approx_equal(a.x, b.x, epsilon)
	    && f32_approx_equal(a.y, b.y, epsilon);
}

// ---- Constants -------------------------------------------------------------

static constexpr F32x2 F32X2_ZERO = {0.0f, 0.0f};
static constexpr F32x2 F32X2_ONE  = {1.0f, 1.0f};
