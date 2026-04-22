#pragma once

#include <core/defines.h>
#include <core/math/u32.h>

// ============================================================================
// U32x3 — 3D U32 vector (scalar). Commonly 3D grid / cluster indices.
// ============================================================================

struct U32x3
{
	U32 x, y, z;
};

inline static U32x3 operator+(const U32x3 &a, const U32x3 &b) { return U32x3{a.x + b.x, a.y + b.y, a.z + b.z}; }
inline static U32x3 &operator+=(U32x3 &a, const U32x3 &b) { a = a + b; return a; }
inline static U32x3 operator-(const U32x3 &a, const U32x3 &b) { return U32x3{a.x - b.x, a.y - b.y, a.z - b.z}; }
inline static U32x3 &operator-=(U32x3 &a, const U32x3 &b) { a = a - b; return a; }
inline static U32x3 operator*(const U32x3 &a, U32 s) { return U32x3{a.x * s, a.y * s, a.z * s}; }
inline static U32x3 operator*(U32 s, const U32x3 &a) { return a * s; }
inline static U32x3 &operator*=(U32x3 &a, U32 s) { a = a * s; return a; }
inline static bool   operator==(const U32x3 &a, const U32x3 &b) { return a.x == b.x && a.y == b.y && a.z == b.z; }

inline static U32    u32x3_dot(const U32x3 &a, const U32x3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline static U32    u32x3_length_squared(const U32x3 &a) { return u32x3_dot(a, a); }
inline static U32x3  u32x3_min(const U32x3 &a, const U32x3 &b) { return U32x3{u32_min(a.x, b.x), u32_min(a.y, b.y), u32_min(a.z, b.z)}; }
inline static U32x3  u32x3_max(const U32x3 &a, const U32x3 &b) { return U32x3{u32_max(a.x, b.x), u32_max(a.y, b.y), u32_max(a.z, b.z)}; }
inline static U32x3  u32x3_clamp(const U32x3 &v, const U32x3 &lo, const U32x3 &hi) { return u32x3_min(u32x3_max(v, lo), hi); }

static constexpr U32x3 U32X3_ZERO = {0u, 0u, 0u};
static constexpr U32x3 U32X3_ONE  = {1u, 1u, 1u};
