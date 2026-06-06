#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/math/f64.h"

#if defined(SIMD_FORCE_SCALAR)
	struct Simd_F64x2 { F64 components[2]; };
#elif defined(SIMD_NEON)
	#include <arm_neon.h>
	typedef float64x2_t Simd_F64x2;
#elif defined(SIMD_AVX)
	#include <immintrin.h>
	typedef __m128d Simd_F64x2;
#else
	struct Simd_F64x2 { F64 components[2]; };
#endif

union alignas(16) F64x2
{
	struct
	{
		F64 x, y;
	};
	F64 components[2];
	Simd_F64x2 simd;

	inline F64 &
	operator[](U64 index)
	{
		validate(index < 2, "[MATH][F64x2]: Component index out of bounds.");
		return components[index];
	}

	inline const F64 &
	operator[](U64 index) const
	{
		validate(index < 2, "[MATH][F64x2]: Component index out of bounds.");
		return components[index];
	}
};

inline static F64x2
operator+(const F64x2 &a, const F64x2 &b)
{
	F64x2 r;
	#if defined(SIMD_NEON)
		r.simd = vaddq_f64(a.simd, b.simd);
	#elif defined(SIMD_AVX)
		r.simd = _mm_add_pd(a.simd, b.simd);
	#else
		r.x = a.x + b.x; r.y = a.y + b.y;
	#endif
	return r;
}

inline static F64x2 &
operator+=(F64x2 &a, const F64x2 &b)
{
	a = a + b;
	return a;
}

inline static F64x2
operator-(const F64x2 &a)
{
	F64x2 r;
	#if defined(SIMD_NEON)
		r.simd = vnegq_f64(a.simd);
	#elif defined(SIMD_AVX)
		r.simd = _mm_sub_pd(_mm_setzero_pd(), a.simd);
	#else
		r.x = -a.x; r.y = -a.y;
	#endif
	return r;
}

inline static F64x2
operator-(const F64x2 &a, const F64x2 &b)
{
	F64x2 r;
	#if defined(SIMD_NEON)
		r.simd = vsubq_f64(a.simd, b.simd);
	#elif defined(SIMD_AVX)
		r.simd = _mm_sub_pd(a.simd, b.simd);
	#else
		r.x = a.x - b.x; r.y = a.y - b.y;
	#endif
	return r;
}

inline static F64x2 &
operator-=(F64x2 &a, const F64x2 &b)
{
	a = a - b;
	return a;
}

inline static F64x2
operator*(const F64x2 &a, F64 s)
{
	F64x2 r;
	#if defined(SIMD_NEON)
		r.simd = vmulq_n_f64(a.simd, s);
	#elif defined(SIMD_AVX)
		r.simd = _mm_mul_pd(a.simd, _mm_set1_pd(s));
	#else
		r.x = a.x * s; r.y = a.y * s;
	#endif
	return r;
}

inline static F64x2
operator*(F64 s, const F64x2 &a)
{
	return a * s;
}

inline static F64x2 &
operator*=(F64x2 &a, F64 s)
{
	a = a * s;
	return a;
}

inline static F64x2
operator/(const F64x2 &a, F64 s)
{
	validate(s != 0.0, "[MATH][F64x2]: scalar divisor must be non-zero.");
	return a * (1.0 / s);
}

inline static F64x2 &
operator/=(F64x2 &a, F64 s)
{
	a = a / s;
	return a;
}

inline static bool
operator==(const F64x2 &a, const F64x2 &b)
{
	return a.x == b.x && a.y == b.y;
}

inline static F64x2
f64x2_from_f64(F64 s)
{
	F64x2 r;
	#if defined(SIMD_NEON)
		r.simd = vdupq_n_f64(s);
	#elif defined(SIMD_AVX)
		r.simd = _mm_set1_pd(s);
	#else
		r.x = s; r.y = s;
	#endif
	return r;
}

inline static F64x2
f64x2_normalize(const F64x2 &a)
{
	F64 length_squared = a.x * a.x + a.y * a.y;
	validate(length_squared != 0.0, "[MATH][F64x2]: Cannot normalize zero-length vector.");
	return a / f64_sqrt(length_squared);
}

inline static F64
f64x2_length(const F64x2 &a)
{
	return f64_sqrt(a.x * a.x + a.y * a.y);
}

inline static F64
f64x2_length_squared(const F64x2 &a)
{
	return a.x * a.x + a.y * a.y;
}

inline static F64
f64x2_distance(const F64x2 &a, const F64x2 &b)
{
	return f64_sqrt(f64x2_length_squared(b - a));
}

inline static F64
f64x2_distance_squared(const F64x2 &a, const F64x2 &b)
{
	return f64x2_length_squared(b - a);
}

inline static F64x2
f64x2_min(const F64x2 &a, const F64x2 &b)
{
	F64x2 r;
	#if defined(SIMD_NEON)
		r.simd = vminq_f64(a.simd, b.simd);
	#elif defined(SIMD_AVX)
		r.simd = _mm_min_pd(a.simd, b.simd);
	#else
		r.x = f64_min(a.x, b.x); r.y = f64_min(a.y, b.y);
	#endif
	return r;
}

inline static F64x2
f64x2_max(const F64x2 &a, const F64x2 &b)
{
	F64x2 r;
	#if defined(SIMD_NEON)
		r.simd = vmaxq_f64(a.simd, b.simd);
	#elif defined(SIMD_AVX)
		r.simd = _mm_max_pd(a.simd, b.simd);
	#else
		r.x = f64_max(a.x, b.x); r.y = f64_max(a.y, b.y);
	#endif
	return r;
}

inline static F64x2
f64x2_clamp(const F64x2 &v, const F64x2 &lo, const F64x2 &hi)
{
	return f64x2_min(f64x2_max(v, lo), hi);
}

inline static bool
f64x2_approx_equal(const F64x2 &a, const F64x2 &b, F64 epsilon)
{
	return f64_approx_equal(a.x, b.x, epsilon)
		&& f64_approx_equal(a.y, b.y, epsilon);
}

inline static F64
f64x2_dot(const F64x2 &a, const F64x2 &b)
{
	#if defined(SIMD_NEON)
		return vaddvq_f64(vmulq_f64(a.simd, b.simd));
	#elif defined(SIMD_AVX)
		return _mm_cvtsd_f64(_mm_dp_pd(a.simd, b.simd, 0x31));
	#else
		return a.x * b.x + a.y * b.y;
	#endif
}

inline static F64
f64x2_cross(const F64x2 &a, const F64x2 &b)
{
	return a.x * b.y - a.y * b.x;
}

inline static F64x2
f64x2_reflect(const F64x2 &v, const F64x2 &normal)
{
	return v - 2.0 * f64x2_dot(v, normal) * normal;
}

inline static F64x2
f64x2_project(const F64x2 &v, const F64x2 &onto)
{
	F64 length_squared = f64x2_length_squared(onto);
	validate(length_squared != 0.0, "[MATH][F64x2]: Cannot project onto zero-length vector.");
	return onto * (f64x2_dot(v, onto) / length_squared);
}

inline static F64x2
f64x2_reject(const F64x2 &v, const F64x2 &onto)
{
	return v - f64x2_project(v, onto);
}

inline static F64x2
f64x2_lerp(const F64x2 &a, const F64x2 &b, F64 t)
{
	return a + (b - a) * t;
}