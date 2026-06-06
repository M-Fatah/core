#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/math/f32.h"

#if defined(SIMD_FORCE_SCALAR)
	struct Simd_F32x4 { F32 components[4]; };
#elif defined(SIMD_NEON)
	#include <arm_neon.h>
	typedef float32x4_t Simd_F32x4;
#elif defined(SIMD_AVX)
	#include <immintrin.h>
	typedef __m128 Simd_F32x4;
#else
	struct Simd_F32x4 { F32 components[4]; };
#endif

union alignas(16) F32x4
{
	struct
	{
		F32 x, y, z, w;
	};
	F32 components[4];
	Simd_F32x4 simd;

	inline F32 &
	operator[](U64 index)
	{
		validate(index < 4, "[MATH][F32x4]: Component index out of bounds.");
		return components[index];
	}

	inline const F32 &
	operator[](U64 index) const
	{
		validate(index < 4, "[MATH][F32x4]: Component index out of bounds.");
		return components[index];
	}
};

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
	validate(s != 0.0f, "[MATH][F32x4]: scalar divisor must be non-zero.");
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

inline static F32x4
f32x4_normalize(const F32x4 &a)
{
	F32 length_squared = a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w;
	validate(length_squared != 0.0f, "[MATH][F32x4]: Cannot normalize zero-length vector.");
	return a / f32_sqrt(length_squared);
}

inline static F32
f32x4_length(const F32x4 &a)
{
	return f32_sqrt(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
}

inline static F32
f32x4_length_squared(const F32x4 &a)
{
	return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w;
}

inline static F32
f32x4_distance(const F32x4 &a, const F32x4 &b)
{
	return f32_sqrt(f32x4_length_squared(b - a));
}

inline static F32
f32x4_distance_squared(const F32x4 &a, const F32x4 &b)
{
	return f32x4_length_squared(b - a);
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

inline static F32
f32x4_dot(const F32x4 &a, const F32x4 &b)
{
	#if defined(SIMD_NEON)
		return vaddvq_f32(vmulq_f32(a.simd, b.simd));
	#elif defined(SIMD_AVX)
		__m128 mul = _mm_mul_ps(a.simd, b.simd);
		__m128 shuf = _mm_movehdup_ps(mul);
		__m128 sums = _mm_add_ps(mul, shuf);
		shuf = _mm_movehl_ps(shuf, sums);
		sums = _mm_add_ss(sums, shuf);
		return _mm_cvtss_f32(sums);
	#else
		return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	#endif
}

inline static F32x4
f32x4_reflect(const F32x4 &v, const F32x4 &normal)
{
	return v - 2.0f * f32x4_dot(v, normal) * normal;
}

inline static F32x4
f32x4_project(const F32x4 &v, const F32x4 &onto)
{
	F32 length_squared = f32x4_length_squared(onto);
	validate(length_squared != 0.0f, "[MATH][F32x4]: Cannot project onto zero-length vector.");
	return onto * (f32x4_dot(v, onto) / length_squared);
}

inline static F32x4
f32x4_reject(const F32x4 &v, const F32x4 &onto)
{
	return v - f32x4_project(v, onto);
}

inline static F32x4
f32x4_lerp(const F32x4 &a, const F32x4 &b, F32 t)
{
	return a + (b - a) * t;
}