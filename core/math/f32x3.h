#pragma once

#include <core/defines.h>
#include <core/math/f32.h>

// ============================================================================
// F32x3 — 3D F32 vector. Packed 12 bytes (no alignas) for GPU-attribute interop.
// ============================================================================

struct F32x3
{
	F32 x, y, z;
};

// ---- Operators -------------------------------------------------------------

inline static F32x3
operator+(const F32x3 &a, const F32x3 &b)
{
	return F32x3{a.x + b.x, a.y + b.y, a.z + b.z};
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
	return F32x3{-a.x, -a.y, -a.z};
}

inline static F32x3
operator-(const F32x3 &a, const F32x3 &b)
{
	return F32x3{a.x - b.x, a.y - b.y, a.z - b.z};
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
	return F32x3{a.x * s, a.y * s, a.z * s};
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

// ---- Free functions --------------------------------------------------------

inline static F32x3
f32x3_from_f32(F32 s)
{
	return F32x3{s, s, s};
}

inline static F32
f32x3_dot(const F32x3 &a, const F32x3 &b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline static F32x3
f32x3_cross(const F32x3 &a, const F32x3 &b)
{
	return F32x3{
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	};
}

inline static F32
f32x3_length_squared(const F32x3 &a)
{
	return f32x3_dot(a, a);
}

inline static F32
f32x3_length(const F32x3 &a)
{
	return f32_sqrt(f32x3_length_squared(a));
}

inline static F32x3
f32x3_normalize(const F32x3 &a)
{
	return a / f32x3_length(a);
}

inline static F32x3
f32x3_min(const F32x3 &a, const F32x3 &b)
{
	return F32x3{f32_min(a.x, b.x), f32_min(a.y, b.y), f32_min(a.z, b.z)};
}

inline static F32x3
f32x3_max(const F32x3 &a, const F32x3 &b)
{
	return F32x3{f32_max(a.x, b.x), f32_max(a.y, b.y), f32_max(a.z, b.z)};
}

inline static F32x3
f32x3_lerp(const F32x3 &a, const F32x3 &b, F32 t)
{
	return F32x3{f32_lerp(a.x, b.x, t), f32_lerp(a.y, b.y, t), f32_lerp(a.z, b.z, t)};
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

// ---- Canonical-convention constants ----------------------------------------
// Right-handed, Y-up, +Z toward the viewer. See docs/math.md for the full
// coordinate convention.

static constexpr F32x3 F32X3_ZERO     = { 0.0f,  0.0f,  0.0f};
static constexpr F32x3 F32X3_ONE      = { 1.0f,  1.0f,  1.0f};
static constexpr F32x3 F32X3_RIGHT    = { 1.0f,  0.0f,  0.0f};
static constexpr F32x3 F32X3_LEFT     = {-1.0f,  0.0f,  0.0f};
static constexpr F32x3 F32X3_UP       = { 0.0f,  1.0f,  0.0f};
static constexpr F32x3 F32X3_DOWN     = { 0.0f, -1.0f,  0.0f};
static constexpr F32x3 F32X3_FORWARD  = { 0.0f,  0.0f, -1.0f};
static constexpr F32x3 F32X3_BACKWARD = { 0.0f,  0.0f,  1.0f};
