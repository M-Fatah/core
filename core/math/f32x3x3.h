#pragma once

#include <core/defines.h>
#include <core/math/f32.h>
#include <core/math/f32x3.h>
#include <core/math/f32x4.h>   // For Simd_F32x4 row storage.

// ============================================================================
// F32x3x3 — 3x3 F32 matrix, row-major, SIMD-backed.
//
// Storage: 3 padded 4-wide rows (48 bytes total). This matches std140 / MSL
// matrix_float3x3 layout exactly — CPU→GPU upload is a direct memcpy. Net cost:
// 12 bytes of padding per matrix.
// ============================================================================

struct alignas(16) F32x3x3
{
	union
	{
		struct
		{
			F32 m00, m01, m02, _pad0;
			F32 m10, m11, m12, _pad1;
			F32 m20, m21, m22, _pad2;
		};
		F32x4 rows[3];
	};
};

inline static F32x3x3
f32x3x3_identity()
{
	return F32x3x3{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f
	};
}

// ---- Element-wise operators ------------------------------------------------

inline static F32x3x3
operator+(const F32x3x3 &A, const F32x3x3 &B)
{
	F32x3x3 R;
	R.rows[0] = A.rows[0] + B.rows[0];
	R.rows[1] = A.rows[1] + B.rows[1];
	R.rows[2] = A.rows[2] + B.rows[2];
	return R;
}

inline static F32x3x3
operator-(const F32x3x3 &M)
{
	F32x3x3 R;
	R.rows[0] = -M.rows[0];
	R.rows[1] = -M.rows[1];
	R.rows[2] = -M.rows[2];
	return R;
}

inline static F32x3x3
operator-(const F32x3x3 &A, const F32x3x3 &B)
{
	F32x3x3 R;
	R.rows[0] = A.rows[0] - B.rows[0];
	R.rows[1] = A.rows[1] - B.rows[1];
	R.rows[2] = A.rows[2] - B.rows[2];
	return R;
}

inline static F32x3x3
operator*(const F32x3x3 &M, F32 s)
{
	F32x3x3 R;
	R.rows[0] = M.rows[0] * s;
	R.rows[1] = M.rows[1] * s;
	R.rows[2] = M.rows[2] * s;
	return R;
}

inline static F32x3x3
operator*(F32 s, const F32x3x3 &M) { return M * s; }

inline static F32x3x3
operator/(const F32x3x3 &M, F32 s) { return M * (1.0f / s); }

inline static bool
operator==(const F32x3x3 &A, const F32x3x3 &B)
{
	return A.m00 == B.m00 && A.m01 == B.m01 && A.m02 == B.m02
	    && A.m10 == B.m10 && A.m11 == B.m11 && A.m12 == B.m12
	    && A.m20 == B.m20 && A.m21 == B.m21 && A.m22 == B.m22;
}

// ---- Vec-mat / mat-mat multiply (row-vector convention) --------------------

inline static F32x3
operator*(const F32x3 &v, const F32x3x3 &M)
{
	return F32x3{
		v.x * M.m00 + v.y * M.m10 + v.z * M.m20,
		v.x * M.m01 + v.y * M.m11 + v.z * M.m21,
		v.x * M.m02 + v.y * M.m12 + v.z * M.m22
	};
}

inline static F32x3x3
operator*(const F32x3x3 &A, const F32x3x3 &B)
{
	F32x3x3 R;

	R.m00 = A.m00 * B.m00 + A.m01 * B.m10 + A.m02 * B.m20;
	R.m01 = A.m00 * B.m01 + A.m01 * B.m11 + A.m02 * B.m21;
	R.m02 = A.m00 * B.m02 + A.m01 * B.m12 + A.m02 * B.m22;
	R._pad0 = 0.0f;

	R.m10 = A.m10 * B.m00 + A.m11 * B.m10 + A.m12 * B.m20;
	R.m11 = A.m10 * B.m01 + A.m11 * B.m11 + A.m12 * B.m21;
	R.m12 = A.m10 * B.m02 + A.m11 * B.m12 + A.m12 * B.m22;
	R._pad1 = 0.0f;

	R.m20 = A.m20 * B.m00 + A.m21 * B.m10 + A.m22 * B.m20;
	R.m21 = A.m20 * B.m01 + A.m21 * B.m11 + A.m22 * B.m21;
	R.m22 = A.m20 * B.m02 + A.m21 * B.m12 + A.m22 * B.m22;
	R._pad2 = 0.0f;

	return R;
}

inline static F32x3x3
f32x3x3_transpose(const F32x3x3 &M)
{
	return F32x3x3{
		M.m00, M.m10, M.m20, 0.0f,
		M.m01, M.m11, M.m21, 0.0f,
		M.m02, M.m12, M.m22, 0.0f
	};
}

inline static F32
f32x3x3_determinant(const F32x3x3 &M)
{
	return M.m00 * (M.m11 * M.m22 - M.m12 * M.m21)
	     - M.m01 * (M.m10 * M.m22 - M.m12 * M.m20)
	     + M.m02 * (M.m10 * M.m21 - M.m11 * M.m20);
}

inline static bool
f32x3x3_is_invertible(const F32x3x3 &M) { return f32x3x3_determinant(M) != 0.0f; }

inline static F32x3x3
f32x3x3_inverse(const F32x3x3 &M)
{
	F32 d = f32x3x3_determinant(M);
	if (d == 0.0f)
		return F32x3x3{};
	F32 inv_d = 1.0f / d;
	return F32x3x3{
		 (M.m11 * M.m22 - M.m12 * M.m21) * inv_d, -(M.m01 * M.m22 - M.m02 * M.m21) * inv_d,  (M.m01 * M.m12 - M.m02 * M.m11) * inv_d, 0.0f,
		-(M.m10 * M.m22 - M.m12 * M.m20) * inv_d,  (M.m00 * M.m22 - M.m02 * M.m20) * inv_d, -(M.m00 * M.m12 - M.m02 * M.m10) * inv_d, 0.0f,
		 (M.m10 * M.m21 - M.m11 * M.m20) * inv_d, -(M.m00 * M.m21 - M.m01 * M.m20) * inv_d,  (M.m00 * M.m11 - M.m01 * M.m10) * inv_d, 0.0f
	};
}

inline static bool
f32x3x3_approx_equal(const F32x3x3 &A, const F32x3x3 &B, F32 epsilon)
{
	return f32_approx_equal(A.m00, B.m00, epsilon) && f32_approx_equal(A.m01, B.m01, epsilon) && f32_approx_equal(A.m02, B.m02, epsilon)
	    && f32_approx_equal(A.m10, B.m10, epsilon) && f32_approx_equal(A.m11, B.m11, epsilon) && f32_approx_equal(A.m12, B.m12, epsilon)
	    && f32_approx_equal(A.m20, B.m20, epsilon) && f32_approx_equal(A.m21, B.m21, epsilon) && f32_approx_equal(A.m22, B.m22, epsilon);
}
