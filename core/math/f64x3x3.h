#pragma once

#include <core/defines.h>
#include <core/math/f64.h>
#include <core/math/f64x3.h>
#include <core/math/f64x4.h>  // For padded F64x4 row storage.

// ============================================================================
// F64x3x3 — 3x3 F64 matrix, row-major, SIMD-backed.
// Storage: 3 × F64x4 padded rows (96 bytes total, matches std140 / MSL layout).
// ============================================================================

struct alignas(32) F64x3x3
{
	union
	{
		struct
		{
			F64 m00, m01, m02, _pad0;
			F64 m10, m11, m12, _pad1;
			F64 m20, m21, m22, _pad2;
		};
		F64x4 rows[3];
	};
};

inline static F64x3x3
f64x3x3_identity()
{
	return F64x3x3{
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0
	};
}

// ---- Operators -------------------------------------------------------------

inline static F64x3x3 operator+(const F64x3x3 &A, const F64x3x3 &B)
{
	F64x3x3 R;
	R.rows[0] = A.rows[0] + B.rows[0];
	R.rows[1] = A.rows[1] + B.rows[1];
	R.rows[2] = A.rows[2] + B.rows[2];
	return R;
}

inline static F64x3x3 operator-(const F64x3x3 &M)
{
	F64x3x3 R;
	R.rows[0] = -M.rows[0];
	R.rows[1] = -M.rows[1];
	R.rows[2] = -M.rows[2];
	return R;
}

inline static F64x3x3 operator-(const F64x3x3 &A, const F64x3x3 &B)
{
	F64x3x3 R;
	R.rows[0] = A.rows[0] - B.rows[0];
	R.rows[1] = A.rows[1] - B.rows[1];
	R.rows[2] = A.rows[2] - B.rows[2];
	return R;
}

inline static F64x3x3 operator*(const F64x3x3 &M, F64 s)
{
	F64x3x3 R;
	R.rows[0] = M.rows[0] * s;
	R.rows[1] = M.rows[1] * s;
	R.rows[2] = M.rows[2] * s;
	return R;
}

inline static F64x3x3 operator*(F64 s, const F64x3x3 &M) { return M * s; }
inline static F64x3x3 operator/(const F64x3x3 &M, F64 s) { return M * (1.0 / s); }

inline static bool operator==(const F64x3x3 &A, const F64x3x3 &B)
{
	return A.m00 == B.m00 && A.m01 == B.m01 && A.m02 == B.m02
	    && A.m10 == B.m10 && A.m11 == B.m11 && A.m12 == B.m12
	    && A.m20 == B.m20 && A.m21 == B.m21 && A.m22 == B.m22;
}

inline static F64x3
operator*(const F64x3 &v, const F64x3x3 &M)
{
	return F64x3{
		v.x * M.m00 + v.y * M.m10 + v.z * M.m20,
		v.x * M.m01 + v.y * M.m11 + v.z * M.m21,
		v.x * M.m02 + v.y * M.m12 + v.z * M.m22
	};
}

inline static F64x3x3
operator*(const F64x3x3 &A, const F64x3x3 &B)
{
	F64x3x3 R;

	R.m00 = A.m00 * B.m00 + A.m01 * B.m10 + A.m02 * B.m20;
	R.m01 = A.m00 * B.m01 + A.m01 * B.m11 + A.m02 * B.m21;
	R.m02 = A.m00 * B.m02 + A.m01 * B.m12 + A.m02 * B.m22;
	R._pad0 = 0.0;

	R.m10 = A.m10 * B.m00 + A.m11 * B.m10 + A.m12 * B.m20;
	R.m11 = A.m10 * B.m01 + A.m11 * B.m11 + A.m12 * B.m21;
	R.m12 = A.m10 * B.m02 + A.m11 * B.m12 + A.m12 * B.m22;
	R._pad1 = 0.0;

	R.m20 = A.m20 * B.m00 + A.m21 * B.m10 + A.m22 * B.m20;
	R.m21 = A.m20 * B.m01 + A.m21 * B.m11 + A.m22 * B.m21;
	R.m22 = A.m20 * B.m02 + A.m21 * B.m12 + A.m22 * B.m22;
	R._pad2 = 0.0;

	return R;
}

inline static F64x3x3
f64x3x3_transpose(const F64x3x3 &M)
{
	return F64x3x3{
		M.m00, M.m10, M.m20, 0.0,
		M.m01, M.m11, M.m21, 0.0,
		M.m02, M.m12, M.m22, 0.0
	};
}

inline static F64
f64x3x3_determinant(const F64x3x3 &M)
{
	return M.m00 * (M.m11 * M.m22 - M.m12 * M.m21)
	     - M.m01 * (M.m10 * M.m22 - M.m12 * M.m20)
	     + M.m02 * (M.m10 * M.m21 - M.m11 * M.m20);
}

inline static bool
f64x3x3_is_invertible(const F64x3x3 &M) { return f64x3x3_determinant(M) != 0.0; }

inline static F64x3x3
f64x3x3_inverse(const F64x3x3 &M)
{
	F64 d = f64x3x3_determinant(M);
	if (d == 0.0)
		return F64x3x3{};
	F64 inv_d = 1.0 / d;
	return F64x3x3{
		 (M.m11 * M.m22 - M.m12 * M.m21) * inv_d, -(M.m01 * M.m22 - M.m02 * M.m21) * inv_d,  (M.m01 * M.m12 - M.m02 * M.m11) * inv_d, 0.0,
		-(M.m10 * M.m22 - M.m12 * M.m20) * inv_d,  (M.m00 * M.m22 - M.m02 * M.m20) * inv_d, -(M.m00 * M.m12 - M.m02 * M.m10) * inv_d, 0.0,
		 (M.m10 * M.m21 - M.m11 * M.m20) * inv_d, -(M.m00 * M.m21 - M.m01 * M.m20) * inv_d,  (M.m00 * M.m11 - M.m01 * M.m10) * inv_d, 0.0
	};
}
