#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/math/f64.h"
#include "core/math/f64x2.h"

#if defined(SIMD_FORCE_SCALAR)
	struct Simd_F64x4 { F64 components[4]; };
#elif defined(SIMD_NEON)
	#include <arm_neon.h>
	struct Simd_F64x4
	{
		float64x2_t lo;
		float64x2_t hi;
	};
#elif defined(SIMD_AVX)
	#include <immintrin.h>
	typedef __m256d Simd_F64x4;
#else
	struct Simd_F64x4 { F64 components[4]; };
#endif

union alignas(32) F64x4
{
	struct
	{
		F64 x, y, z, w;
	};
	F64 components[4];
	Simd_F64x4 simd;

	inline F64 &
	operator[](U64 index)
	{
		validate(index < 4, "[MATH][F64x4]: Component index out of bounds.");
		return components[index];
	}

	inline const F64 &
	operator[](U64 index) const
	{
		validate(index < 4, "[MATH][F64x4]: Component index out of bounds.");
		return components[index];
	}
};

inline static F64x4
operator+(const F64x4 &a, const F64x4 &b)
{
	F64x4 r;
	#if defined(SIMD_NEON)
		r.simd.lo = vaddq_f64(a.simd.lo, b.simd.lo);
		r.simd.hi = vaddq_f64(a.simd.hi, b.simd.hi);
	#elif defined(SIMD_AVX)
		r.simd = _mm256_add_pd(a.simd, b.simd);
	#else
		r.x = a.x + b.x; r.y = a.y + b.y; r.z = a.z + b.z; r.w = a.w + b.w;
	#endif
	return r;
}

inline static F64x4 &
operator+=(F64x4 &a, const F64x4 &b)
{
	a = a + b;
	return a;
}

inline static F64x4
operator-(const F64x4 &a)
{
	F64x4 r;
	#if defined(SIMD_NEON)
		r.simd.lo = vnegq_f64(a.simd.lo);
		r.simd.hi = vnegq_f64(a.simd.hi);
	#elif defined(SIMD_AVX)
		r.simd = _mm256_sub_pd(_mm256_setzero_pd(), a.simd);
	#else
		r.x = -a.x; r.y = -a.y; r.z = -a.z; r.w = -a.w;
	#endif
	return r;
}

inline static F64x4
operator-(const F64x4 &a, const F64x4 &b)
{
	F64x4 r;
	#if defined(SIMD_NEON)
		r.simd.lo = vsubq_f64(a.simd.lo, b.simd.lo);
		r.simd.hi = vsubq_f64(a.simd.hi, b.simd.hi);
	#elif defined(SIMD_AVX)
		r.simd = _mm256_sub_pd(a.simd, b.simd);
	#else
		r.x = a.x - b.x; r.y = a.y - b.y; r.z = a.z - b.z; r.w = a.w - b.w;
	#endif
	return r;
}

inline static F64x4 &
operator-=(F64x4 &a, const F64x4 &b)
{
	a = a - b;
	return a;
}

inline static F64x4
operator*(const F64x4 &a, F64 s)
{
	F64x4 r;
	#if defined(SIMD_NEON)
		r.simd.lo = vmulq_n_f64(a.simd.lo, s);
		r.simd.hi = vmulq_n_f64(a.simd.hi, s);
	#elif defined(SIMD_AVX)
		r.simd = _mm256_mul_pd(a.simd, _mm256_set1_pd(s));
	#else
		r.x = a.x * s; r.y = a.y * s; r.z = a.z * s; r.w = a.w * s;
	#endif
	return r;
}

inline static F64x4
operator*(F64 s, const F64x4 &a)
{
	return a * s;
}

inline static F64x4 &
operator*=(F64x4 &a, F64 s)
{
	a = a * s;
	return a;
}

inline static F64x4
operator/(const F64x4 &a, F64 s)
{
	validate(s != 0.0, "[MATH][F64x4]: scalar divisor must be non-zero.");
	return a * (1.0 / s);
}

inline static F64x4 &
operator/=(F64x4 &a, F64 s)
{
	a = a / s;
	return a;
}

inline static bool
operator==(const F64x4 &a, const F64x4 &b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

inline static F64x4
f64x4_from_f64(F64 s)
{
	F64x4 r;
	#if defined(SIMD_NEON)
		r.simd.lo = vdupq_n_f64(s);
		r.simd.hi = vdupq_n_f64(s);
	#elif defined(SIMD_AVX)
		r.simd = _mm256_set1_pd(s);
	#else
		r.x = s; r.y = s; r.z = s; r.w = s;
	#endif
	return r;
}

inline static F64x4
f64x4_normalize(const F64x4 &a)
{
	F64 length_squared = a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w;
	validate(length_squared != 0.0, "[MATH][F64x4]: Cannot normalize zero-length vector.");
	return a / f64_sqrt(length_squared);
}

inline static F64
f64x4_length(const F64x4 &a)
{
	return f64_sqrt(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
}

inline static F64
f64x4_length_squared(const F64x4 &a)
{
	return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w;
}

inline static F64
f64x4_distance(const F64x4 &a, const F64x4 &b)
{
	return f64_sqrt(f64x4_length_squared(b - a));
}

inline static F64
f64x4_distance_squared(const F64x4 &a, const F64x4 &b)
{
	return f64x4_length_squared(b - a);
}

inline static F64x4
f64x4_min(const F64x4 &a, const F64x4 &b)
{
	F64x4 r;
	#if defined(SIMD_NEON)
		r.simd.lo = vminq_f64(a.simd.lo, b.simd.lo);
		r.simd.hi = vminq_f64(a.simd.hi, b.simd.hi);
	#elif defined(SIMD_AVX)
		r.simd = _mm256_min_pd(a.simd, b.simd);
	#else
		r.x = f64_min(a.x, b.x); r.y = f64_min(a.y, b.y);
		r.z = f64_min(a.z, b.z); r.w = f64_min(a.w, b.w);
	#endif
	return r;
}

inline static F64x4
f64x4_max(const F64x4 &a, const F64x4 &b)
{
	F64x4 r;
	#if defined(SIMD_NEON)
		r.simd.lo = vmaxq_f64(a.simd.lo, b.simd.lo);
		r.simd.hi = vmaxq_f64(a.simd.hi, b.simd.hi);
	#elif defined(SIMD_AVX)
		r.simd = _mm256_max_pd(a.simd, b.simd);
	#else
		r.x = f64_max(a.x, b.x); r.y = f64_max(a.y, b.y);
		r.z = f64_max(a.z, b.z); r.w = f64_max(a.w, b.w);
	#endif
	return r;
}

inline static F64x4
f64x4_clamp(const F64x4 &v, const F64x4 &lo, const F64x4 &hi)
{
	return f64x4_min(f64x4_max(v, lo), hi);
}

inline static bool
f64x4_approx_equal(const F64x4 &a, const F64x4 &b, F64 epsilon)
{
	return f64_approx_equal(a.x, b.x, epsilon)
		&& f64_approx_equal(a.y, b.y, epsilon)
		&& f64_approx_equal(a.z, b.z, epsilon)
		&& f64_approx_equal(a.w, b.w, epsilon);
}

inline static F64
f64x4_dot(const F64x4 &a, const F64x4 &b)
{
	#if defined(SIMD_NEON)
		return vaddvq_f64(vmulq_f64(a.simd.lo, b.simd.lo))
			+ vaddvq_f64(vmulq_f64(a.simd.hi, b.simd.hi));
	#elif defined(SIMD_AVX)
		__m256d m   = _mm256_mul_pd(a.simd, b.simd);
		__m128d low = _mm256_castpd256_pd128(m);
		__m128d hi  = _mm256_extractf128_pd(m, 1);
		__m128d sum = _mm_add_pd(low, hi);
		return _mm_cvtsd_f64(_mm_add_sd(sum, _mm_unpackhi_pd(sum, sum)));
	#else
		return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	#endif
}

inline static F64x4
f64x4_reflect(const F64x4 &v, const F64x4 &normal)
{
	return v - 2.0 * f64x4_dot(v, normal) * normal;
}

inline static F64x4
f64x4_project(const F64x4 &v, const F64x4 &onto)
{
	F64 length_squared = f64x4_length_squared(onto);
	validate(length_squared != 0.0, "[MATH][F64x4]: Cannot project onto zero-length vector.");
	return onto * (f64x4_dot(v, onto) / length_squared);
}

inline static F64x4
f64x4_reject(const F64x4 &v, const F64x4 &onto)
{
	return v - f64x4_project(v, onto);
}

inline static F64x4
f64x4_lerp(const F64x4 &a, const F64x4 &b, F64 t)
{
	return a + (b - a) * t;
}