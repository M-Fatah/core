#pragma once

#include <core/defines.h>
#include <core/math/f64.h>

// ============================================================================
// F64x3 — 3D F64 vector. Packed 24 bytes (no alignas) for GPU-attribute interop.
// ============================================================================

struct F64x3
{
	F64 x, y, z;
};

// ---- Operators -------------------------------------------------------------

inline static F64x3
operator+(const F64x3 &a, const F64x3 &b) { return F64x3{a.x + b.x, a.y + b.y, a.z + b.z}; }

inline static F64x3 &
operator+=(F64x3 &a, const F64x3 &b) { a = a + b; return a; }

inline static F64x3
operator-(const F64x3 &a) { return F64x3{-a.x, -a.y, -a.z}; }

inline static F64x3
operator-(const F64x3 &a, const F64x3 &b) { return F64x3{a.x - b.x, a.y - b.y, a.z - b.z}; }

inline static F64x3 &
operator-=(F64x3 &a, const F64x3 &b) { a = a - b; return a; }

inline static F64x3
operator*(const F64x3 &a, F64 s) { return F64x3{a.x * s, a.y * s, a.z * s}; }

inline static F64x3
operator*(F64 s, const F64x3 &a) { return a * s; }

inline static F64x3 &
operator*=(F64x3 &a, F64 s) { a = a * s; return a; }

inline static F64x3
operator/(const F64x3 &a, F64 s) { return a * (1.0 / s); }

inline static F64x3 &
operator/=(F64x3 &a, F64 s) { a = a / s; return a; }

inline static bool
operator==(const F64x3 &a, const F64x3 &b) { return a.x == b.x && a.y == b.y && a.z == b.z; }

// ---- Free functions --------------------------------------------------------

inline static F64x3
f64x3_from_f64(F64 s) { return F64x3{s, s, s}; }

inline static F64
f64x3_dot(const F64x3 &a, const F64x3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

inline static F64x3
f64x3_cross(const F64x3 &a, const F64x3 &b)
{
	return F64x3{
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	};
}

inline static F64
f64x3_length_squared(const F64x3 &a) { return f64x3_dot(a, a); }

inline static F64
f64x3_length(const F64x3 &a) { return f64_sqrt(f64x3_length_squared(a)); }

inline static F64x3
f64x3_normalize(const F64x3 &a) { return a / f64x3_length(a); }

inline static F64x3
f64x3_min(const F64x3 &a, const F64x3 &b) { return F64x3{f64_min(a.x, b.x), f64_min(a.y, b.y), f64_min(a.z, b.z)}; }

inline static F64x3
f64x3_max(const F64x3 &a, const F64x3 &b) { return F64x3{f64_max(a.x, b.x), f64_max(a.y, b.y), f64_max(a.z, b.z)}; }

inline static F64x3
f64x3_lerp(const F64x3 &a, const F64x3 &b, F64 t) { return F64x3{f64_lerp(a.x, b.x, t), f64_lerp(a.y, b.y, t), f64_lerp(a.z, b.z, t)}; }

inline static F64x3
f64x3_clamp(const F64x3 &v, const F64x3 &lo, const F64x3 &hi) { return f64x3_min(f64x3_max(v, lo), hi); }

inline static bool
f64x3_approx_equal(const F64x3 &a, const F64x3 &b, F64 epsilon)
{
	return f64_approx_equal(a.x, b.x, epsilon)
	    && f64_approx_equal(a.y, b.y, epsilon)
	    && f64_approx_equal(a.z, b.z, epsilon);
}

// ---- Constants -------------------------------------------------------------

static constexpr F64x3 F64X3_ZERO     = { 0.0,  0.0,  0.0};
static constexpr F64x3 F64X3_ONE      = { 1.0,  1.0,  1.0};
static constexpr F64x3 F64X3_RIGHT    = { 1.0,  0.0,  0.0};
static constexpr F64x3 F64X3_LEFT     = {-1.0,  0.0,  0.0};
static constexpr F64x3 F64X3_UP       = { 0.0,  1.0,  0.0};
static constexpr F64x3 F64X3_DOWN     = { 0.0, -1.0,  0.0};
static constexpr F64x3 F64X3_FORWARD  = { 0.0,  0.0, -1.0};
static constexpr F64x3 F64X3_BACKWARD = { 0.0,  0.0,  1.0};
