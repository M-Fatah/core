#pragma once

#include <core/defines.h>
#include <core/math/f64.h>
#include <core/math/f64x2.h>

// ============================================================================
// F64x2x2 — 2x2 F64 matrix, row-major, scalar (2x2 is not worth SIMD).
// Memory layout: [m00 m01 | m10 m11], 32 bytes.
// ============================================================================

struct F64x2x2
{
	F64 m00, m01;
	F64 m10, m11;
};

inline static F64x2x2
f64x2x2_identity()
{
	return F64x2x2{1.0, 0.0, 0.0, 1.0};
}

// ---- Operators -------------------------------------------------------------

inline static F64x2x2
operator+(const F64x2x2 &A, const F64x2x2 &B)
{
	return F64x2x2{A.m00 + B.m00, A.m01 + B.m01, A.m10 + B.m10, A.m11 + B.m11};
}

inline static F64x2x2
operator-(const F64x2x2 &M)
{
	return F64x2x2{-M.m00, -M.m01, -M.m10, -M.m11};
}

inline static F64x2x2
operator-(const F64x2x2 &A, const F64x2x2 &B)
{
	return F64x2x2{A.m00 - B.m00, A.m01 - B.m01, A.m10 - B.m10, A.m11 - B.m11};
}

inline static F64x2x2
operator*(const F64x2x2 &M, F64 s)
{
	return F64x2x2{M.m00 * s, M.m01 * s, M.m10 * s, M.m11 * s};
}

inline static F64x2x2
operator*(F64 s, const F64x2x2 &M) { return M * s; }

inline static F64x2x2
operator/(const F64x2x2 &M, F64 s) { return M * (1.0 / s); }

inline static bool
operator==(const F64x2x2 &A, const F64x2x2 &B)
{
	return A.m00 == B.m00 && A.m01 == B.m01 && A.m10 == B.m10 && A.m11 == B.m11;
}

inline static F64x2
operator*(const F64x2 &v, const F64x2x2 &M)
{
	return F64x2{v.x * M.m00 + v.y * M.m10, v.x * M.m01 + v.y * M.m11};
}

inline static F64x2x2
operator*(const F64x2x2 &A, const F64x2x2 &B)
{
	return F64x2x2{
		A.m00 * B.m00 + A.m01 * B.m10,  A.m00 * B.m01 + A.m01 * B.m11,
		A.m10 * B.m00 + A.m11 * B.m10,  A.m10 * B.m01 + A.m11 * B.m11
	};
}

// ---- Transpose / determinant / inverse -------------------------------------

inline static F64x2x2
f64x2x2_transpose(const F64x2x2 &M)
{
	return F64x2x2{M.m00, M.m10, M.m01, M.m11};
}

inline static F64
f64x2x2_determinant(const F64x2x2 &M)
{
	return M.m00 * M.m11 - M.m01 * M.m10;
}

inline static bool
f64x2x2_is_invertible(const F64x2x2 &M)
{
	return f64x2x2_determinant(M) != 0.0;
}

inline static F64x2x2
f64x2x2_inverse(const F64x2x2 &M)
{
	F64 d = f64x2x2_determinant(M);
	if (d == 0.0)
		return F64x2x2{};
	F64 inv_d = 1.0 / d;
	return F64x2x2{ M.m11 * inv_d, -M.m01 * inv_d, -M.m10 * inv_d, M.m00 * inv_d };
}
