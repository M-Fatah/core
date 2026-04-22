#pragma once

#include <core/defines.h>
#include <core/math/f32.h>
#include <core/math/f32x2.h>

// ============================================================================
// F32x2x2 — 2x2 F32 matrix, row-major, scalar (2x2 is not worth SIMD).
// Memory layout: [m00 m01 | m10 m11], 16 bytes.
// ============================================================================

struct F32x2x2
{
	F32 m00, m01;
	F32 m10, m11;
};

inline static F32x2x2
f32x2x2_identity()
{
	return F32x2x2{1.0f, 0.0f, 0.0f, 1.0f};
}

// ---- Operators -------------------------------------------------------------

inline static F32x2x2
operator+(const F32x2x2 &A, const F32x2x2 &B)
{
	return F32x2x2{A.m00 + B.m00, A.m01 + B.m01, A.m10 + B.m10, A.m11 + B.m11};
}

inline static F32x2x2
operator-(const F32x2x2 &M)
{
	return F32x2x2{-M.m00, -M.m01, -M.m10, -M.m11};
}

inline static F32x2x2
operator-(const F32x2x2 &A, const F32x2x2 &B)
{
	return F32x2x2{A.m00 - B.m00, A.m01 - B.m01, A.m10 - B.m10, A.m11 - B.m11};
}

inline static F32x2x2
operator*(const F32x2x2 &M, F32 s)
{
	return F32x2x2{M.m00 * s, M.m01 * s, M.m10 * s, M.m11 * s};
}

inline static F32x2x2
operator*(F32 s, const F32x2x2 &M) { return M * s; }

inline static F32x2x2
operator/(const F32x2x2 &M, F32 s) { return M * (1.0f / s); }

inline static bool
operator==(const F32x2x2 &A, const F32x2x2 &B)
{
	return A.m00 == B.m00 && A.m01 == B.m01 && A.m10 == B.m10 && A.m11 == B.m11;
}

// v * M (row-vector convention).
inline static F32x2
operator*(const F32x2 &v, const F32x2x2 &M)
{
	return F32x2{
		v.x * M.m00 + v.y * M.m10,
		v.x * M.m01 + v.y * M.m11
	};
}

// A * B = row i of result is A.row[i] * B.
inline static F32x2x2
operator*(const F32x2x2 &A, const F32x2x2 &B)
{
	return F32x2x2{
		A.m00 * B.m00 + A.m01 * B.m10,  A.m00 * B.m01 + A.m01 * B.m11,
		A.m10 * B.m00 + A.m11 * B.m10,  A.m10 * B.m01 + A.m11 * B.m11
	};
}

inline static F32x2x2
f32x2x2_transpose(const F32x2x2 &M)
{
	return F32x2x2{M.m00, M.m10, M.m01, M.m11};
}

inline static F32
f32x2x2_determinant(const F32x2x2 &M) { return M.m00 * M.m11 - M.m01 * M.m10; }

inline static bool
f32x2x2_is_invertible(const F32x2x2 &M) { return f32x2x2_determinant(M) != 0.0f; }

inline static F32x2x2
f32x2x2_inverse(const F32x2x2 &M)
{
	F32 d = f32x2x2_determinant(M);
	if (d == 0.0f)
		return F32x2x2{};
	F32 inv_d = 1.0f / d;
	return F32x2x2{
		 M.m11 * inv_d, -M.m01 * inv_d,
		-M.m10 * inv_d,  M.m00 * inv_d
	};
}
