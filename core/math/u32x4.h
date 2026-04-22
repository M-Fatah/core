#pragma once

#include <core/defines.h>
#include <core/math/u32.h>

// ============================================================================
// U32x4 — 4D U32 vector, SIMD-backed. SIMD masks, packed flags.
// ============================================================================

#if defined(SIMD_FORCE_SCALAR)
	struct Simd_U32x4 { U32 v[4]; };
#elif defined(SIMD_NEON)
	#include <arm_neon.h>
	typedef uint32x4_t Simd_U32x4;
#elif defined(SIMD_AVX)
	#include <immintrin.h>
	typedef __m128i Simd_U32x4;
#else
	struct Simd_U32x4 { U32 v[4]; };
#endif

struct alignas(16) U32x4
{
	union
	{
		struct { U32 x, y, z, w; };
		Simd_U32x4 simd;
	};
};

// ---- Operators -------------------------------------------------------------

inline static U32x4
operator+(const U32x4 &a, const U32x4 &b)
{
	U32x4 r;
#if defined(SIMD_NEON)
	r.simd = vaddq_u32(a.simd, b.simd);
#elif defined(SIMD_AVX)
	r.simd = _mm_add_epi32(a.simd, b.simd);
#else
	r.x = a.x + b.x; r.y = a.y + b.y; r.z = a.z + b.z; r.w = a.w + b.w;
#endif
	return r;
}

inline static U32x4 &
operator+=(U32x4 &a, const U32x4 &b) { a = a + b; return a; }

inline static U32x4
operator-(const U32x4 &a, const U32x4 &b)
{
	U32x4 r;
#if defined(SIMD_NEON)
	r.simd = vsubq_u32(a.simd, b.simd);
#elif defined(SIMD_AVX)
	r.simd = _mm_sub_epi32(a.simd, b.simd);
#else
	r.x = a.x - b.x; r.y = a.y - b.y; r.z = a.z - b.z; r.w = a.w - b.w;
#endif
	return r;
}

inline static U32x4 &
operator-=(U32x4 &a, const U32x4 &b) { a = a - b; return a; }

inline static U32x4
operator*(const U32x4 &a, U32 s)
{
	U32x4 r;
#if defined(SIMD_NEON)
	r.simd = vmulq_n_u32(a.simd, s);
#elif defined(SIMD_AVX)
	r.simd = _mm_mullo_epi32(a.simd, _mm_set1_epi32((I32)s));
#else
	r.x = a.x * s; r.y = a.y * s; r.z = a.z * s; r.w = a.w * s;
#endif
	return r;
}

inline static U32x4
operator*(U32 s, const U32x4 &a) { return a * s; }

inline static U32x4 &
operator*=(U32x4 &a, U32 s) { a = a * s; return a; }

inline static bool
operator==(const U32x4 &a, const U32x4 &b) { return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; }

// ---- Free functions --------------------------------------------------------

inline static U32x4
u32x4_from_u32(U32 s)
{
	U32x4 r;
#if defined(SIMD_NEON)
	r.simd = vdupq_n_u32(s);
#elif defined(SIMD_AVX)
	r.simd = _mm_set1_epi32((I32)s);
#else
	r.x = s; r.y = s; r.z = s; r.w = s;
#endif
	return r;
}

inline static U32
u32x4_dot(const U32x4 &a, const U32x4 &b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

inline static U32
u32x4_length_squared(const U32x4 &a) { return u32x4_dot(a, a); }

inline static U32x4
u32x4_min(const U32x4 &a, const U32x4 &b)
{
	U32x4 r;
#if defined(SIMD_NEON)
	r.simd = vminq_u32(a.simd, b.simd);
#elif defined(SIMD_AVX)
	r.simd = _mm_min_epu32(a.simd, b.simd);  // SSE4.1+
#else
	r.x = u32_min(a.x, b.x); r.y = u32_min(a.y, b.y);
	r.z = u32_min(a.z, b.z); r.w = u32_min(a.w, b.w);
#endif
	return r;
}

inline static U32x4
u32x4_max(const U32x4 &a, const U32x4 &b)
{
	U32x4 r;
#if defined(SIMD_NEON)
	r.simd = vmaxq_u32(a.simd, b.simd);
#elif defined(SIMD_AVX)
	r.simd = _mm_max_epu32(a.simd, b.simd);  // SSE4.1+
#else
	r.x = u32_max(a.x, b.x); r.y = u32_max(a.y, b.y);
	r.z = u32_max(a.z, b.z); r.w = u32_max(a.w, b.w);
#endif
	return r;
}

inline static U32x4
u32x4_clamp(const U32x4 &v, const U32x4 &lo, const U32x4 &hi)
{
	return u32x4_min(u32x4_max(v, lo), hi);
}

// ---- Constants -------------------------------------------------------------

static constexpr U32x4 U32X4_ZERO = {0u, 0u, 0u, 0u};
static constexpr U32x4 U32X4_ONE  = {1u, 1u, 1u, 1u};
