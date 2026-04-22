#pragma once

#include <core/defines.h>
#include <core/math/i32.h>

// ============================================================================
// I32x2 — 2D I32 vector (scalar).
// ============================================================================

struct I32x2
{
	I32 x, y;
};

inline static I32x2 operator+(const I32x2 &a, const I32x2 &b) { return I32x2{a.x + b.x, a.y + b.y}; }
inline static I32x2 &operator+=(I32x2 &a, const I32x2 &b) { a = a + b; return a; }
inline static I32x2 operator-(const I32x2 &a) { return I32x2{-a.x, -a.y}; }
inline static I32x2 operator-(const I32x2 &a, const I32x2 &b) { return I32x2{a.x - b.x, a.y - b.y}; }
inline static I32x2 &operator-=(I32x2 &a, const I32x2 &b) { a = a - b; return a; }
inline static I32x2 operator*(const I32x2 &a, I32 s) { return I32x2{a.x * s, a.y * s}; }
inline static I32x2 operator*(I32 s, const I32x2 &a) { return a * s; }
inline static I32x2 &operator*=(I32x2 &a, I32 s) { a = a * s; return a; }
inline static bool   operator==(const I32x2 &a, const I32x2 &b) { return a.x == b.x && a.y == b.y; }

inline static I32    i32x2_dot(const I32x2 &a, const I32x2 &b) { return a.x * b.x + a.y * b.y; }
inline static I32    i32x2_length_squared(const I32x2 &a) { return i32x2_dot(a, a); }
inline static I32x2  i32x2_abs(const I32x2 &a) { return I32x2{i32_abs(a.x), i32_abs(a.y)}; }
inline static I32x2  i32x2_min(const I32x2 &a, const I32x2 &b) { return I32x2{i32_min(a.x, b.x), i32_min(a.y, b.y)}; }
inline static I32x2  i32x2_max(const I32x2 &a, const I32x2 &b) { return I32x2{i32_max(a.x, b.x), i32_max(a.y, b.y)}; }
inline static I32x2  i32x2_clamp(const I32x2 &v, const I32x2 &lo, const I32x2 &hi) { return i32x2_min(i32x2_max(v, lo), hi); }

static constexpr I32x2 I32X2_ZERO = {0, 0};
static constexpr I32x2 I32X2_ONE  = {1, 1};
