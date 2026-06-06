#pragma once

#include "core/defines.h"
#include "core/math/f32.h"
#include "core/math/f32x3.h"
#include "core/validate.h"

union F32x3x3
{
	struct
	{
		F32 m00, m01, m02;
		F32 m10, m11, m12;
		F32 m20, m21, m22;
	};
	F32x3 rows[3];

	inline F32x3 &
	operator[](U64 index)
	{
		validate(index < 3, "[MATH][F32x3x3]: Row index out of bounds.");
		return rows[index];
	}

	inline const F32x3 &
	operator[](U64 index) const
	{
		validate(index < 3, "[MATH][F32x3x3]: Row index out of bounds.");
		return rows[index];
	}
};

inline static F32x3x3
operator+(const F32x3x3 &A, const F32x3x3 &B)
{
	return F32x3x3 {
		.m00 = A.m00 + B.m00, .m01 = A.m01 + B.m01, .m02 = A.m02 + B.m02,
		.m10 = A.m10 + B.m10, .m11 = A.m11 + B.m11, .m12 = A.m12 + B.m12,
		.m20 = A.m20 + B.m20, .m21 = A.m21 + B.m21, .m22 = A.m22 + B.m22
	};
}

inline static F32x3x3
operator-(const F32x3x3 &M)
{
	return F32x3x3 {
		.m00 = -M.m00, .m01 = -M.m01, .m02 = -M.m02,
		.m10 = -M.m10, .m11 = -M.m11, .m12 = -M.m12,
		.m20 = -M.m20, .m21 = -M.m21, .m22 = -M.m22
	};
}

inline static F32x3x3
operator-(const F32x3x3 &A, const F32x3x3 &B)
{
	return F32x3x3 {
		.m00 = A.m00 - B.m00, .m01 = A.m01 - B.m01, .m02 = A.m02 - B.m02,
		.m10 = A.m10 - B.m10, .m11 = A.m11 - B.m11, .m12 = A.m12 - B.m12,
		.m20 = A.m20 - B.m20, .m21 = A.m21 - B.m21, .m22 = A.m22 - B.m22
	};
}

inline static F32x3x3
operator*(const F32x3x3 &M, F32 s)
{
	return F32x3x3 {
		.m00 = M.m00 * s, .m01 = M.m01 * s, .m02 = M.m02 * s,
		.m10 = M.m10 * s, .m11 = M.m11 * s, .m12 = M.m12 * s,
		.m20 = M.m20 * s, .m21 = M.m21 * s, .m22 = M.m22 * s
	};
}

inline static F32x3x3
operator*(F32 s, const F32x3x3 &M)
{
	return M * s;
}

inline static F32x3
operator*(const F32x3 &v, const F32x3x3 &M)
{
	return F32x3 {
		.x = v.x * M.m00 + v.y * M.m10 + v.z * M.m20,
		.y = v.x * M.m01 + v.y * M.m11 + v.z * M.m21,
		.z = v.x * M.m02 + v.y * M.m12 + v.z * M.m22
	};
}

inline static F32x3x3
operator*(const F32x3x3 &A, const F32x3x3 &B)
{
	return F32x3x3 {
		.m00 = A.m00 * B.m00 + A.m01 * B.m10 + A.m02 * B.m20, .m01 = A.m00 * B.m01 + A.m01 * B.m11 + A.m02 * B.m21, .m02 = A.m00 * B.m02 + A.m01 * B.m12 + A.m02 * B.m22,
		.m10 = A.m10 * B.m00 + A.m11 * B.m10 + A.m12 * B.m20, .m11 = A.m10 * B.m01 + A.m11 * B.m11 + A.m12 * B.m21, .m12 = A.m10 * B.m02 + A.m11 * B.m12 + A.m12 * B.m22,
		.m20 = A.m20 * B.m00 + A.m21 * B.m10 + A.m22 * B.m20, .m21 = A.m20 * B.m01 + A.m21 * B.m11 + A.m22 * B.m21, .m22 = A.m20 * B.m02 + A.m21 * B.m12 + A.m22 * B.m22
	};
}

inline static F32x3x3
operator/(const F32x3x3 &M, F32 s)
{
	validate(s != 0.0f, "[MATH][F32x3x3]: scalar divisor must be non-zero.");
	return M * (1.0f / s);
}

inline static bool
operator==(const F32x3x3 &A, const F32x3x3 &B)
{
	return A.m00 == B.m00 && A.m01 == B.m01 && A.m02 == B.m02
		&& A.m10 == B.m10 && A.m11 == B.m11 && A.m12 == B.m12
		&& A.m20 == B.m20 && A.m21 == B.m21 && A.m22 == B.m22;
}

inline static F32x3x3
f32x3x3_identity()
{
	return F32x3x3 {
		.m00 = 1.0f, .m01 = 0.0f, .m02 = 0.0f,
		.m10 = 0.0f, .m11 = 1.0f, .m12 = 0.0f,
		.m20 = 0.0f, .m21 = 0.0f, .m22 = 1.0f
	};
}

inline static F32x3x3
f32x3x3_transpose(const F32x3x3 &M)
{
	return F32x3x3 {
		.m00 = M.m00, .m01 = M.m10, .m02 = M.m20,
		.m10 = M.m01, .m11 = M.m11, .m12 = M.m21,
		.m20 = M.m02, .m21 = M.m12, .m22 = M.m22
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
f32x3x3_is_invertible(const F32x3x3 &M)
{
	return f32x3x3_determinant(M) != 0.0f;
}

inline static F32x3x3
f32x3x3_inverse(const F32x3x3 &M)
{
	F32 d = f32x3x3_determinant(M);
	validate(d != 0.0f, "[MATH][F32x3x3]: Matrix must be invertible.");
	F32 inv_d = 1.0f / d;
	return F32x3x3 {
		.m00 =  (M.m11 * M.m22 - M.m12 * M.m21) * inv_d, .m01 = -(M.m01 * M.m22 - M.m02 * M.m21) * inv_d, .m02 =  (M.m01 * M.m12 - M.m02 * M.m11) * inv_d,
		.m10 = -(M.m10 * M.m22 - M.m12 * M.m20) * inv_d, .m11 =  (M.m00 * M.m22 - M.m02 * M.m20) * inv_d, .m12 = -(M.m00 * M.m12 - M.m02 * M.m10) * inv_d,
		.m20 =  (M.m10 * M.m21 - M.m11 * M.m20) * inv_d, .m21 = -(M.m00 * M.m21 - M.m01 * M.m20) * inv_d, .m22 =  (M.m00 * M.m11 - M.m01 * M.m10) * inv_d
	};
}

inline static bool
f32x3x3_approx_equal(const F32x3x3 &A, const F32x3x3 &B, F32 epsilon)
{
	return f32x3_approx_equal(A.rows[0], B.rows[0], epsilon)
		&& f32x3_approx_equal(A.rows[1], B.rows[1], epsilon)
		&& f32x3_approx_equal(A.rows[2], B.rows[2], epsilon);
}