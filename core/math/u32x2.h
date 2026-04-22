#pragma once

#include <core/defines.h>
#include <core/math/u32.h>

// ============================================================================
// U32x2 — 2D U32 vector (scalar). Commonly texture coords / grid indices.
// ============================================================================

struct U32x2
{
	U32 x, y;
};

inline static U32x2 operator+(const U32x2 &a, const U32x2 &b) { return U32x2{a.x + b.x, a.y + b.y}; }
inline static U32x2 &operator+=(U32x2 &a, const U32x2 &b) { a = a + b; return a; }
inline static U32x2 operator-(const U32x2 &a, const U32x2 &b) { return U32x2{a.x - b.x, a.y - b.y}; }
inline static U32x2 &operator-=(U32x2 &a, const U32x2 &b) { a = a - b; return a; }
inline static U32x2 operator*(const U32x2 &a, U32 s) { return U32x2{a.x * s, a.y * s}; }
inline static U32x2 operator*(U32 s, const U32x2 &a) { return a * s; }
inline static U32x2 &operator*=(U32x2 &a, U32 s) { a = a * s; return a; }
inline static bool   operator==(const U32x2 &a, const U32x2 &b) { return a.x == b.x && a.y == b.y; }

inline static U32    u32x2_dot(const U32x2 &a, const U32x2 &b) { return a.x * b.x + a.y * b.y; }
inline static U32    u32x2_length_squared(const U32x2 &a) { return u32x2_dot(a, a); }
inline static U32x2  u32x2_min(const U32x2 &a, const U32x2 &b) { return U32x2{u32_min(a.x, b.x), u32_min(a.y, b.y)}; }
inline static U32x2  u32x2_max(const U32x2 &a, const U32x2 &b) { return U32x2{u32_max(a.x, b.x), u32_max(a.y, b.y)}; }
inline static U32x2  u32x2_clamp(const U32x2 &v, const U32x2 &lo, const U32x2 &hi) { return u32x2_min(u32x2_max(v, lo), hi); }

static constexpr U32x2 U32X2_ZERO = {0u, 0u};
static constexpr U32x2 U32X2_ONE  = {1u, 1u};
