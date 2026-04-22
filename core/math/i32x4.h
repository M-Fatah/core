#pragma once

#include <core/defines.h>
#include <core/math/i32.h>

// ============================================================================
// I32x4 — 4D I32 vector, SIMD-backed. Useful for SIMD masks, packed flags,
// cluster indexing, batched integer arithmetic.
// ============================================================================

#if defined(SIMD_FORCE_SCALAR)
	struct Simd_I32x4 { I32 v[4]; };
#elif defined(SIMD_NEON)
	#include <arm_neon.h>
	typedef int32x4_t Simd_I32x4;
#elif defined(SIMD_AVX)
	#include <immintrin.h>
	typedef __m128i Simd_I32x4;
#else
	struct Simd_I32x4 { I32 v[4]; };
#endif

struct alignas(16) I32x4
{
	union
	{
		struct { I32 x, y, z, w; };
		Simd_I32x4 simd;
	};
};

// ---- Operators -------------------------------------------------------------

inline static I32x4
operator+(const I32x4 &a, const I32x4 &b)
{
	I32x4 r;
#if defined(SIMD_NEON)
	r.simd = vaddq_s32(a.simd, b.simd);
#elif defined(SIMD_AVX)
	r.simd = _mm_add_epi32(a.simd, b.simd);
#else
	r.x = a.x + b.x; r.y = a.y + b.y; r.z = a.z + b.z; r.w = a.w + b.w;
#endif
	return r;
}

inline static I32x4 &
operator+=(I32x4 &a, const I32x4 &b) { a = a + b; return a; }

inline static I32x4
operator-(const I32x4 &a)
{
	I32x4 r;
#if defined(SIMD_NEON)
	r.simd = vnegq_s32(a.simd);
#elif defined(SIMD_AVX)
	r.simd = _mm_sub_epi32(_mm_setzero_si128(), a.simd);
#else
	r.x = -a.x; r.y = -a.y; r.z = -a.z; r.w = -a.w;
#endif
	return r;
}

inline static I32x4
operator-(const I32x4 &a, const I32x4 &b)
{
	I32x4 r;
#if defined(SIMD_NEON)
	r.simd = vsubq_s32(a.simd, b.simd);
#elif defined(SIMD_AVX)
	r.simd = _mm_sub_epi32(a.simd, b.simd);
#else
	r.x = a.x - b.x; r.y = a.y - b.y; r.z = a.z - b.z; r.w = a.w - b.w;
#endif
	return r;
}

inline static I32x4 &
operator-=(I32x4 &a, const I32x4 &b) { a = a - b; return a; }

inline static I32x4
operator*(const I32x4 &a, I32 s)
{
	I32x4 r;
#if defined(SIMD_NEON)
	r.simd = vmulq_n_s32(a.simd, s);
#elif defined(SIMD_AVX)
	// _mm_mullo_epi32 is SSE4.1; guaranteed under AVX baseline.
	r.simd = _mm_mullo_epi32(a.simd, _mm_set1_epi32(s));
#else
	r.x = a.x * s; r.y = a.y * s; r.z = a.z * s; r.w = a.w * s;
#endif
	return r;
}

inline static I32x4
operator*(I32 s, const I32x4 &a) { return a * s; }

inline static I32x4 &
operator*=(I32x4 &a, I32 s) { a = a * s; return a; }

inline static bool
operator==(const I32x4 &a, const I32x4 &b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

// ---- Free functions --------------------------------------------------------

inline static I32x4
i32x4_from_i32(I32 s)
{
	I32x4 r;
#if defined(SIMD_NEON)
	r.simd = vdupq_n_s32(s);
#elif defined(SIMD_AVX)
	r.simd = _mm_set1_epi32(s);
#else
	r.x = s; r.y = s; r.z = s; r.w = s;
#endif
	return r;
}

inline static I32
i32x4_dot(const I32x4 &a, const I32x4 &b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline static I32
i32x4_length_squared(const I32x4 &a) { return i32x4_dot(a, a); }

inline static I32x4
i32x4_abs(const I32x4 &a)
{
	I32x4 r;
#if defined(SIMD_NEON)
	r.simd = vabsq_s32(a.simd);
#elif defined(SIMD_AVX)
	r.simd = _mm_abs_epi32(a.simd);  // SSSE3+
#else
	r.x = i32_abs(a.x); r.y = i32_abs(a.y); r.z = i32_abs(a.z); r.w = i32_abs(a.w);
#endif
	return r;
}

inline static I32x4
i32x4_min(const I32x4 &a, const I32x4 &b)
{
	I32x4 r;
#if defined(SIMD_NEON)
	r.simd = vminq_s32(a.simd, b.simd);
#elif defined(SIMD_AVX)
	r.simd = _mm_min_epi32(a.simd, b.simd);  // SSE4.1+
#else
	r.x = i32_min(a.x, b.x); r.y = i32_min(a.y, b.y);
	r.z = i32_min(a.z, b.z); r.w = i32_min(a.w, b.w);
#endif
	return r;
}

inline static I32x4
i32x4_max(const I32x4 &a, const I32x4 &b)
{
	I32x4 r;
#if defined(SIMD_NEON)
	r.simd = vmaxq_s32(a.simd, b.simd);
#elif defined(SIMD_AVX)
	r.simd = _mm_max_epi32(a.simd, b.simd);  // SSE4.1+
#else
	r.x = i32_max(a.x, b.x); r.y = i32_max(a.y, b.y);
	r.z = i32_max(a.z, b.z); r.w = i32_max(a.w, b.w);
#endif
	return r;
}

inline static I32x4
i32x4_clamp(const I32x4 &v, const I32x4 &lo, const I32x4 &hi)
{
	return i32x4_min(i32x4_max(v, lo), hi);
}

// ---- Constants -------------------------------------------------------------

static constexpr I32x4 I32X4_ZERO = {0, 0, 0, 0};
static constexpr I32x4 I32X4_ONE  = {1, 1, 1, 1};
