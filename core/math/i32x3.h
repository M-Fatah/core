#pragma once

#include <core/defines.h>
#include <core/math/i32.h>

// ============================================================================
// I32x3 — 3D I32 vector (scalar, packed).
// ============================================================================

struct I32x3
{
	I32 x, y, z;
};

inline static I32x3 operator+(const I32x3 &a, const I32x3 &b) { return I32x3{a.x + b.x, a.y + b.y, a.z + b.z}; }
inline static I32x3 &operator+=(I32x3 &a, const I32x3 &b) { a = a + b; return a; }
inline static I32x3 operator-(const I32x3 &a) { return I32x3{-a.x, -a.y, -a.z}; }
inline static I32x3 operator-(const I32x3 &a, const I32x3 &b) { return I32x3{a.x - b.x, a.y - b.y, a.z - b.z}; }
inline static I32x3 &operator-=(I32x3 &a, const I32x3 &b) { a = a - b; return a; }
inline static I32x3 operator*(const I32x3 &a, I32 s) { return I32x3{a.x * s, a.y * s, a.z * s}; }
inline static I32x3 operator*(I32 s, const I32x3 &a) { return a * s; }
inline static I32x3 &operator*=(I32x3 &a, I32 s) { a = a * s; return a; }
inline static bool   operator==(const I32x3 &a, const I32x3 &b) { return a.x == b.x && a.y == b.y && a.z == b.z; }

inline static I32    i32x3_dot(const I32x3 &a, const I32x3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline static I32    i32x3_length_squared(const I32x3 &a) { return i32x3_dot(a, a); }
inline static I32x3  i32x3_abs(const I32x3 &a) { return I32x3{i32_abs(a.x), i32_abs(a.y), i32_abs(a.z)}; }
inline static I32x3  i32x3_min(const I32x3 &a, const I32x3 &b) { return I32x3{i32_min(a.x, b.x), i32_min(a.y, b.y), i32_min(a.z, b.z)}; }
inline static I32x3  i32x3_max(const I32x3 &a, const I32x3 &b) { return I32x3{i32_max(a.x, b.x), i32_max(a.y, b.y), i32_max(a.z, b.z)}; }
inline static I32x3  i32x3_clamp(const I32x3 &v, const I32x3 &lo, const I32x3 &hi) { return i32x3_min(i32x3_max(v, lo), hi); }

static constexpr I32x3 I32X3_ZERO = {0, 0, 0};
static constexpr I32x3 I32X3_ONE  = {1, 1, 1};
