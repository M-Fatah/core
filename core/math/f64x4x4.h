#pragma once

#include <core/defines.h>
#include <core/math/f64.h>
#include <core/math/f64x3.h>
#include <core/math/f64x4.h>

// ============================================================================
// F64x4x4 — 4x4 F64 matrix, row-major, SIMD-backed.
// 4 rows × F64x4 = 128 bytes. Same conventions as F32x4x4.
// ============================================================================

struct alignas(32) F64x4x4
{
	union
	{
		struct
		{
			F64 m00, m01, m02, m03;
			F64 m10, m11, m12, m13;
			F64 m20, m21, m22, m23;
			F64 m30, m31, m32, m33;
		};
		F64x4 rows[4];
	};
};

inline static const F64 &
f64x4x4_at(const F64x4x4 &M, I32 i) { return *((const F64 *)&M + i); }

inline static F64 &
f64x4x4_at(F64x4x4 &M, I32 i) { return *((F64 *)&M + i); }

inline static F64x4x4
f64x4x4_identity()
{
	return F64x4x4{
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0
	};
}

// ---- Element-wise operators ------------------------------------------------

inline static F64x4x4
operator+(const F64x4x4 &A, const F64x4x4 &B)
{
	F64x4x4 R;
	R.rows[0] = A.rows[0] + B.rows[0];
	R.rows[1] = A.rows[1] + B.rows[1];
	R.rows[2] = A.rows[2] + B.rows[2];
	R.rows[3] = A.rows[3] + B.rows[3];
	return R;
}

inline static F64x4x4
operator-(const F64x4x4 &M)
{
	F64x4x4 R;
	R.rows[0] = -M.rows[0];
	R.rows[1] = -M.rows[1];
	R.rows[2] = -M.rows[2];
	R.rows[3] = -M.rows[3];
	return R;
}

inline static F64x4x4
operator-(const F64x4x4 &A, const F64x4x4 &B)
{
	F64x4x4 R;
	R.rows[0] = A.rows[0] - B.rows[0];
	R.rows[1] = A.rows[1] - B.rows[1];
	R.rows[2] = A.rows[2] - B.rows[2];
	R.rows[3] = A.rows[3] - B.rows[3];
	return R;
}

inline static F64x4x4
operator*(const F64x4x4 &M, F64 s)
{
	F64x4x4 R;
	R.rows[0] = M.rows[0] * s;
	R.rows[1] = M.rows[1] * s;
	R.rows[2] = M.rows[2] * s;
	R.rows[3] = M.rows[3] * s;
	return R;
}

inline static F64x4x4 operator*(F64 s, const F64x4x4 &M) { return M * s; }
inline static F64x4x4 operator/(const F64x4x4 &M, F64 s) { return M * (1.0 / s); }

inline static bool
operator==(const F64x4x4 &A, const F64x4x4 &B)
{
	return A.rows[0] == B.rows[0] && A.rows[1] == B.rows[1]
	    && A.rows[2] == B.rows[2] && A.rows[3] == B.rows[3];
}

// ---- Vec-mat / mat-mat multiply (row-vector) -------------------------------

inline static F64x4
operator*(const F64x4 &v, const F64x4x4 &M)
{
	return v.x * M.rows[0] + v.y * M.rows[1] + v.z * M.rows[2] + v.w * M.rows[3];
}

inline static F64x4x4
operator*(const F64x4x4 &A, const F64x4x4 &B)
{
	F64x4x4 R;
	R.rows[0] = A.rows[0] * B;
	R.rows[1] = A.rows[1] * B;
	R.rows[2] = A.rows[2] * B;
	R.rows[3] = A.rows[3] * B;
	return R;
}

// ---- Transpose / determinant / inverse -------------------------------------

inline static F64x4x4
f64x4x4_transpose(const F64x4x4 &M)
{
	return F64x4x4{
		M.m00, M.m10, M.m20, M.m30,
		M.m01, M.m11, M.m21, M.m31,
		M.m02, M.m12, M.m22, M.m32,
		M.m03, M.m13, M.m23, M.m33
	};
}

inline static F64
f64x4x4_determinant(const F64x4x4 &M)
{
	return (M.m00 * M.m11 - M.m01 * M.m10) * (M.m22 * M.m33 - M.m23 * M.m32)
	     - (M.m00 * M.m12 - M.m02 * M.m10) * (M.m21 * M.m33 - M.m23 * M.m31)
	     + (M.m00 * M.m13 - M.m03 * M.m10) * (M.m21 * M.m32 - M.m22 * M.m31)
	     + (M.m01 * M.m12 - M.m02 * M.m11) * (M.m20 * M.m33 - M.m23 * M.m30)
	     - (M.m01 * M.m13 - M.m03 * M.m11) * (M.m20 * M.m32 - M.m22 * M.m30)
	     + (M.m02 * M.m13 - M.m03 * M.m12) * (M.m20 * M.m31 - M.m21 * M.m30);
}

inline static bool
f64x4x4_is_invertible(const F64x4x4 &M) { return f64x4x4_determinant(M) != 0.0; }

inline static F64x4x4
f64x4x4_inverse(const F64x4x4 &M)
{
	F64 d = f64x4x4_determinant(M);
	if (d == 0.0)
		return F64x4x4{};

	F64x4x4 adj = F64x4x4{
		+ M.m11 * (M.m22 * M.m33 - M.m23 * M.m32) - M.m12 * (M.m21 * M.m33 - M.m23 * M.m31) + M.m13 * (M.m21 * M.m32 - M.m22 * M.m31),
		- M.m01 * (M.m22 * M.m33 - M.m23 * M.m32) + M.m02 * (M.m21 * M.m33 - M.m23 * M.m31) - M.m03 * (M.m21 * M.m32 - M.m22 * M.m31),
		+ M.m01 * (M.m12 * M.m33 - M.m13 * M.m32) - M.m02 * (M.m11 * M.m33 - M.m13 * M.m31) + M.m03 * (M.m11 * M.m32 - M.m12 * M.m31),
		- M.m01 * (M.m12 * M.m23 - M.m13 * M.m22) + M.m02 * (M.m11 * M.m23 - M.m13 * M.m21) - M.m03 * (M.m11 * M.m22 - M.m12 * M.m21),

		- M.m10 * (M.m22 * M.m33 - M.m23 * M.m32) + M.m12 * (M.m20 * M.m33 - M.m23 * M.m30) - M.m13 * (M.m20 * M.m32 - M.m22 * M.m30),
		+ M.m00 * (M.m22 * M.m33 - M.m23 * M.m32) - M.m02 * (M.m20 * M.m33 - M.m23 * M.m30) + M.m03 * (M.m20 * M.m32 - M.m22 * M.m30),
		- M.m00 * (M.m12 * M.m33 - M.m13 * M.m32) + M.m02 * (M.m10 * M.m33 - M.m13 * M.m30) - M.m03 * (M.m10 * M.m32 - M.m12 * M.m30),
		+ M.m00 * (M.m12 * M.m23 - M.m13 * M.m22) - M.m02 * (M.m10 * M.m23 - M.m13 * M.m20) + M.m03 * (M.m10 * M.m22 - M.m12 * M.m20),

		+ M.m10 * (M.m21 * M.m33 - M.m23 * M.m31) - M.m11 * (M.m20 * M.m33 - M.m23 * M.m30) + M.m13 * (M.m20 * M.m31 - M.m21 * M.m30),
		- M.m00 * (M.m21 * M.m33 - M.m23 * M.m31) + M.m01 * (M.m20 * M.m33 - M.m23 * M.m30) - M.m03 * (M.m20 * M.m31 - M.m21 * M.m30),
		+ M.m00 * (M.m11 * M.m33 - M.m13 * M.m31) - M.m01 * (M.m10 * M.m33 - M.m13 * M.m30) + M.m03 * (M.m10 * M.m31 - M.m11 * M.m30),
		- M.m00 * (M.m11 * M.m23 - M.m13 * M.m21) + M.m01 * (M.m10 * M.m23 - M.m13 * M.m20) - M.m03 * (M.m10 * M.m21 - M.m11 * M.m20),

		- M.m10 * (M.m21 * M.m32 - M.m22 * M.m31) + M.m11 * (M.m20 * M.m32 - M.m22 * M.m30) - M.m12 * (M.m20 * M.m31 - M.m21 * M.m30),
		+ M.m00 * (M.m21 * M.m32 - M.m22 * M.m31) - M.m01 * (M.m20 * M.m32 - M.m22 * M.m30) + M.m02 * (M.m20 * M.m31 - M.m21 * M.m30),
		- M.m00 * (M.m11 * M.m32 - M.m12 * M.m31) + M.m01 * (M.m10 * M.m32 - M.m12 * M.m30) - M.m02 * (M.m10 * M.m31 - M.m11 * M.m30),
		+ M.m00 * (M.m11 * M.m22 - M.m12 * M.m21) - M.m01 * (M.m10 * M.m22 - M.m12 * M.m20) + M.m02 * (M.m10 * M.m21 - M.m11 * M.m20)
	};

	return adj * (1.0 / d);
}

// ---- TRS builders ----------------------------------------------------------

inline static F64x4x4
f64x4x4_translation(F64 dx, F64 dy, F64 dz)
{
	return F64x4x4{
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		dx,  dy,  dz,  1.0
	};
}

inline static F64x4x4
f64x4x4_translation(const F64x3 &t) { return f64x4x4_translation(t.x, t.y, t.z); }

inline static F64x4x4
f64x4x4_scaling(F64 sx, F64 sy, F64 sz)
{
	return F64x4x4{
		sx,  0.0, 0.0, 0.0,
		0.0, sy,  0.0, 0.0,
		0.0, 0.0, sz,  0.0,
		0.0, 0.0, 0.0, 1.0
	};
}

inline static F64x4x4
f64x4x4_scaling(const F64x3 &s) { return f64x4x4_scaling(s.x, s.y, s.z); }
