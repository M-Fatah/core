#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/math/f64.h"
#include "core/math/f64x3.h"

union F64x3x3
{
	struct
	{
		F64 m00, m01, m02;
		F64 m10, m11, m12;
		F64 m20, m21, m22;
	};
	F64x3 rows[3];

	inline F64x3 &
	operator[](U64 index)
	{
		validate(index < 3, "[MATH][F64x3x3]: Row index out of bounds.");
		return rows[index];
	}

	inline const F64x3 &
	operator[](U64 index) const
	{
		validate(index < 3, "[MATH][F64x3x3]: Row index out of bounds.");
		return rows[index];
	}
};

inline static F64x3x3
operator+(const F64x3x3 &A, const F64x3x3 &B)
{
	return F64x3x3 {
		.m00 = A.m00 + B.m00, .m01 = A.m01 + B.m01, .m02 = A.m02 + B.m02,
		.m10 = A.m10 + B.m10, .m11 = A.m11 + B.m11, .m12 = A.m12 + B.m12,
		.m20 = A.m20 + B.m20, .m21 = A.m21 + B.m21, .m22 = A.m22 + B.m22
	};
}

inline static F64x3x3
operator-(const F64x3x3 &M)
{
	return F64x3x3 {
		.m00 = -M.m00, .m01 = -M.m01, .m02 = -M.m02,
		.m10 = -M.m10, .m11 = -M.m11, .m12 = -M.m12,
		.m20 = -M.m20, .m21 = -M.m21, .m22 = -M.m22
	};
}

inline static F64x3x3
operator-(const F64x3x3 &A, const F64x3x3 &B)
{
	return F64x3x3 {
		.m00 = A.m00 - B.m00, .m01 = A.m01 - B.m01, .m02 = A.m02 - B.m02,
		.m10 = A.m10 - B.m10, .m11 = A.m11 - B.m11, .m12 = A.m12 - B.m12,
		.m20 = A.m20 - B.m20, .m21 = A.m21 - B.m21, .m22 = A.m22 - B.m22
	};
}

inline static F64x3x3
operator*(const F64x3x3 &M, F64 s)
{
	return F64x3x3 {
		.m00 = M.m00 * s, .m01 = M.m01 * s, .m02 = M.m02 * s,
		.m10 = M.m10 * s, .m11 = M.m11 * s, .m12 = M.m12 * s,
		.m20 = M.m20 * s, .m21 = M.m21 * s, .m22 = M.m22 * s
	};
}

inline static F64x3x3
operator*(F64 s, const F64x3x3 &M)
{
	return M * s;
}

inline static F64x3
operator*(const F64x3 &v, const F64x3x3 &M)
{
	return F64x3 {
		.x = v.x * M.m00 + v.y * M.m10 + v.z * M.m20,
		.y = v.x * M.m01 + v.y * M.m11 + v.z * M.m21,
		.z = v.x * M.m02 + v.y * M.m12 + v.z * M.m22
	};
}

inline static F64x3x3
operator*(const F64x3x3 &A, const F64x3x3 &B)
{
	return F64x3x3 {
		.m00 = A.m00 * B.m00 + A.m01 * B.m10 + A.m02 * B.m20, .m01 = A.m00 * B.m01 + A.m01 * B.m11 + A.m02 * B.m21, .m02 = A.m00 * B.m02 + A.m01 * B.m12 + A.m02 * B.m22,
		.m10 = A.m10 * B.m00 + A.m11 * B.m10 + A.m12 * B.m20, .m11 = A.m10 * B.m01 + A.m11 * B.m11 + A.m12 * B.m21, .m12 = A.m10 * B.m02 + A.m11 * B.m12 + A.m12 * B.m22,
		.m20 = A.m20 * B.m00 + A.m21 * B.m10 + A.m22 * B.m20, .m21 = A.m20 * B.m01 + A.m21 * B.m11 + A.m22 * B.m21, .m22 = A.m20 * B.m02 + A.m21 * B.m12 + A.m22 * B.m22
	};
}

inline static F64x3x3
operator/(const F64x3x3 &M, F64 s)
{
	validate(s != 0.0, "[MATH][F64x3x3]: scalar divisor must be non-zero.");
	return M * (1.0 / s);
}

inline static bool
operator==(const F64x3x3 &A, const F64x3x3 &B)
{
	return A.m00 == B.m00 && A.m01 == B.m01 && A.m02 == B.m02
		&& A.m10 == B.m10 && A.m11 == B.m11 && A.m12 == B.m12
		&& A.m20 == B.m20 && A.m21 == B.m21 && A.m22 == B.m22;
}

inline static F64x3x3
f64x3x3_identity()
{
	return F64x3x3 {
		.m00 = 1.0, .m01 = 0.0, .m02 = 0.0,
		.m10 = 0.0, .m11 = 1.0, .m12 = 0.0,
		.m20 = 0.0, .m21 = 0.0, .m22 = 1.0
	};
}

inline static F64x3x3
f64x3x3_transpose(const F64x3x3 &M)
{
	return F64x3x3 {
		.m00 = M.m00, .m01 = M.m10, .m02 = M.m20,
		.m10 = M.m01, .m11 = M.m11, .m12 = M.m21,
		.m20 = M.m02, .m21 = M.m12, .m22 = M.m22
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
f64x3x3_is_invertible(const F64x3x3 &M)
{
	return f64x3x3_determinant(M) != 0.0;
}

inline static F64x3x3
f64x3x3_inverse(const F64x3x3 &M)
{
	F64 d = f64x3x3_determinant(M);
	validate(d != 0.0, "[MATH][F64x3x3]: Matrix must be invertible.");
	F64 inv_d = 1.0 / d;
	return F64x3x3 {
		.m00 =  (M.m11 * M.m22 - M.m12 * M.m21) * inv_d, .m01 = -(M.m01 * M.m22 - M.m02 * M.m21) * inv_d, .m02 =  (M.m01 * M.m12 - M.m02 * M.m11) * inv_d,
		.m10 = -(M.m10 * M.m22 - M.m12 * M.m20) * inv_d, .m11 =  (M.m00 * M.m22 - M.m02 * M.m20) * inv_d, .m12 = -(M.m00 * M.m12 - M.m02 * M.m10) * inv_d,
		.m20 =  (M.m10 * M.m21 - M.m11 * M.m20) * inv_d, .m21 = -(M.m00 * M.m21 - M.m01 * M.m20) * inv_d, .m22 =  (M.m00 * M.m11 - M.m01 * M.m10) * inv_d
	};
}

inline static bool
f64x3x3_approx_equal(const F64x3x3 &A, const F64x3x3 &B, F64 epsilon)
{
	return f64x3_approx_equal(A.rows[0], B.rows[0], epsilon)
		&& f64x3_approx_equal(A.rows[1], B.rows[1], epsilon)
		&& f64x3_approx_equal(A.rows[2], B.rows[2], epsilon);
}