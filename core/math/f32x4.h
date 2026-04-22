#pragma once

#include <core/defines.h>
#include <core/math/f32.h>

// ============================================================================
// F32x4 — 4D F32 vector, SIMD-backed.
//
// Storage: alignas(16). Union with scalar fields (x, y, z, w) and the SIMD lane
// register. Field access is unchanged from scalar code; hot paths read/write
// `simd` directly.
//
// Arch gates are set by CMake (see core/CMakeLists.txt). The scalar fallback
// is always compiled (for SIMD_FORCE_SCALAR parity testing).
// ============================================================================

#if defined(SIMD_FORCE_SCALAR)
	struct Simd_F32x4 { F32 v[4]; };
#elif defined(SIMD_NEON)
	#include <arm_neon.h>
	typedef float32x4_t Simd_F32x4;
#elif defined(SIMD_AVX)
	#include <immintrin.h>
	typedef __m128 Simd_F32x4;
#else
	struct Simd_F32x4 { F32 v[4]; };
#endif

struct alignas(16) F32x4
{
	union
	{
		struct { F32 x, y, z, w; };
		Simd_F32x4 simd;
	};
};

// ---- Operators -------------------------------------------------------------

inline static F32x4
operator+(const F32x4 &a, const F32x4 &b)
{
	F32x4 r;
#if defined(SIMD_NEON)
	r.simd = vaddq_f32(a.simd, b.simd);
#elif defined(SIMD_AVX)
	r.simd = _mm_add_ps(a.simd, b.simd);
#else
	r.x = a.x + b.x; r.y = a.y + b.y; r.z = a.z + b.z; r.w = a.w + b.w;
#endif
	return r;
}

inline static F32x4 &
operator+=(F32x4 &a, const F32x4 &b)
{
	a = a + b;
	return a;
}

inline static F32x4
operator-(const F32x4 &a)
{
	F32x4 r;
#if defined(SIMD_NEON)
	r.simd = vnegq_f32(a.simd);
#elif defined(SIMD_AVX)
	r.simd = _mm_sub_ps(_mm_setzero_ps(), a.simd);
#else
	r.x = -a.x; r.y = -a.y; r.z = -a.z; r.w = -a.w;
#endif
	return r;
}

inline static F32x4
operator-(const F32x4 &a, const F32x4 &b)
{
	F32x4 r;
#if defined(SIMD_NEON)
	r.simd = vsubq_f32(a.simd, b.simd);
#elif defined(SIMD_AVX)
	r.simd = _mm_sub_ps(a.simd, b.simd);
#else
	r.x = a.x - b.x; r.y = a.y - b.y; r.z = a.z - b.z; r.w = a.w - b.w;
#endif
	return r;
}

inline static F32x4 &
operator-=(F32x4 &a, const F32x4 &b)
{
	a = a - b;
	return a;
}

inline static F32x4
operator*(const F32x4 &a, F32 s)
{
	F32x4 r;
#if defined(SIMD_NEON)
	r.simd = vmulq_n_f32(a.simd, s);
#elif defined(SIMD_AVX)
	r.simd = _mm_mul_ps(a.simd, _mm_set1_ps(s));
#else
	r.x = a.x * s; r.y = a.y * s; r.z = a.z * s; r.w = a.w * s;
#endif
	return r;
}

inline static F32x4
operator*(F32 s, const F32x4 &a)
{
	return a * s;
}

inline static F32x4 &
operator*=(F32x4 &a, F32 s)
{
	a = a * s;
	return a;
}

inline static F32x4
operator/(const F32x4 &a, F32 s)
{
	// Multiply by reciprocal — faster than lane-wise divide on most ISAs.
	return a * (1.0f / s);
}

inline static F32x4 &
operator/=(F32x4 &a, F32 s)
{
	a = a / s;
	return a;
}

inline static bool
operator==(const F32x4 &a, const F32x4 &b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

// ---- Free functions --------------------------------------------------------

inline static F32x4
f32x4_from_f32(F32 s)
{
	F32x4 r;
#if defined(SIMD_NEON)
	r.simd = vdupq_n_f32(s);
#elif defined(SIMD_AVX)
	r.simd = _mm_set1_ps(s);
#else
	r.x = s; r.y = s; r.z = s; r.w = s;
#endif
	return r;
}

inline static F32
f32x4_dot(const F32x4 &a, const F32x4 &b)
{
#if defined(SIMD_NEON)
	return vaddvq_f32(vmulq_f32(a.simd, b.simd));
#elif defined(SIMD_AVX)
	// _mm_dp_ps is SSE4.1; guaranteed under AVX baseline. Mask 0xFF = "multiply
	// all 4 lanes, sum into lane 0" and extract as scalar.
	return _mm_cvtss_f32(_mm_dp_ps(a.simd, b.simd, 0xFF));
#else
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
#endif
}

inline static F32
f32x4_length_squared(const F32x4 &a)
{
	return f32x4_dot(a, a);
}

inline static F32
f32x4_length(const F32x4 &a)
{
	return f32_sqrt(f32x4_length_squared(a));
}

inline static F32x4
f32x4_normalize(const F32x4 &a)
{
	return a / f32x4_length(a);
}

inline static F32x4
f32x4_min(const F32x4 &a, const F32x4 &b)
{
	F32x4 r;
#if defined(SIMD_NEON)
	r.simd = vminq_f32(a.simd, b.simd);
#elif defined(SIMD_AVX)
	r.simd = _mm_min_ps(a.simd, b.simd);
#else
	r.x = f32_min(a.x, b.x); r.y = f32_min(a.y, b.y);
	r.z = f32_min(a.z, b.z); r.w = f32_min(a.w, b.w);
#endif
	return r;
}

inline static F32x4
f32x4_max(const F32x4 &a, const F32x4 &b)
{
	F32x4 r;
#if defined(SIMD_NEON)
	r.simd = vmaxq_f32(a.simd, b.simd);
#elif defined(SIMD_AVX)
	r.simd = _mm_max_ps(a.simd, b.simd);
#else
	r.x = f32_max(a.x, b.x); r.y = f32_max(a.y, b.y);
	r.z = f32_max(a.z, b.z); r.w = f32_max(a.w, b.w);
#endif
	return r;
}

inline static F32x4
f32x4_lerp(const F32x4 &a, const F32x4 &b, F32 t)
{
	return a + (b - a) * t;
}

inline static F32x4
f32x4_clamp(const F32x4 &v, const F32x4 &lo, const F32x4 &hi)
{
	return f32x4_min(f32x4_max(v, lo), hi);
}

inline static bool
f32x4_approx_equal(const F32x4 &a, const F32x4 &b, F32 epsilon)
{
	return f32_approx_equal(a.x, b.x, epsilon)
	    && f32_approx_equal(a.y, b.y, epsilon)
	    && f32_approx_equal(a.z, b.z, epsilon)
	    && f32_approx_equal(a.w, b.w, epsilon);
}

// ---- Constants -------------------------------------------------------------

static constexpr F32x4 F32X4_ZERO = {0.0f, 0.0f, 0.0f, 0.0f};
static constexpr F32x4 F32X4_ONE  = {1.0f, 1.0f, 1.0f, 1.0f};
