#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/math/f64.h"
#include "core/math/f64x2.h"

union F64x2x2
{
	struct
	{
		F64 m00, m01;
		F64 m10, m11;
	};
	F64x2 rows[2];

	inline F64x2 &
	operator[](U64 row)
	{
		validate(row < 2, "[MATH][F64x2x2]: Row index out of bounds.");
		return rows[row];
	}

	inline const F64x2 &
	operator[](U64 row) const
	{
		validate(row < 2, "[MATH][F64x2x2]: Row index out of bounds.");
		return rows[row];
	}
};

inline static F64x2x2
operator+(const F64x2x2 &A, const F64x2x2 &B)
{
	return F64x2x2 {
		.m00 = A.m00 + B.m00, .m01 = A.m01 + B.m01,
		.m10 = A.m10 + B.m10, .m11 = A.m11 + B.m11
	};
}

inline static F64x2x2
operator-(const F64x2x2 &M)
{
	return F64x2x2 {
		.m00 = -M.m00, .m01 = -M.m01,
		.m10 = -M.m10, .m11 = -M.m11
	};
}

inline static F64x2x2
operator-(const F64x2x2 &A, const F64x2x2 &B)
{
	return F64x2x2 {
		.m00 = A.m00 - B.m00, .m01 = A.m01 - B.m01,
		.m10 = A.m10 - B.m10, .m11 = A.m11 - B.m11
	};
}

inline static F64x2x2
operator*(const F64x2x2 &M, F64 s)
{
	return F64x2x2 {
		.m00 = M.m00 * s, .m01 = M.m01 * s,
		.m10 = M.m10 * s, .m11 = M.m11 * s
	};
}

inline static F64x2x2
operator*(F64 s, const F64x2x2 &M)
{
	return M * s;
}

inline static F64x2
operator*(const F64x2 &v, const F64x2x2 &M)
{
	return F64x2 {
		.x = v.x * M.m00 + v.y * M.m10,
		.y = v.x * M.m01 + v.y * M.m11
	};
}

inline static F64x2x2
operator*(const F64x2x2 &A, const F64x2x2 &B)
{
	return F64x2x2 {
		.m00 = A.m00 * B.m00 + A.m01 * B.m10, .m01 = A.m00 * B.m01 + A.m01 * B.m11,
		.m10 = A.m10 * B.m00 + A.m11 * B.m10, .m11 = A.m10 * B.m01 + A.m11 * B.m11
	};
}

inline static F64x2x2
operator/(const F64x2x2 &M, F64 s)
{
	validate(s != 0.0, "[MATH][F64x2x2]: scalar divisor must be non-zero.");
	return M * (1.0 / s);
}

inline static bool
operator==(const F64x2x2 &A, const F64x2x2 &B)
{
	return A.m00 == B.m00 && A.m01 == B.m01 && A.m10 == B.m10 && A.m11 == B.m11;
}

inline static F64x2x2
f64x2x2_identity()
{
	return F64x2x2 {
		.m00 = 1.0, .m01 = 0.0,
		.m10 = 0.0, .m11 = 1.0
	};
}

inline static F64x2x2
f64x2x2_transpose(const F64x2x2 &M)
{
	return F64x2x2 {
		.m00 = M.m00, .m01 = M.m10,
		.m10 = M.m01, .m11 = M.m11
	};
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
	validate(d != 0.0, "[MATH][F64x2x2]: Matrix must be invertible.");
	F64 inv_d = 1.0 / d;
	return F64x2x2 {
		.m00 =  M.m11 * inv_d, .m01 = -M.m01 * inv_d,
		.m10 = -M.m10 * inv_d, .m11 =  M.m00 * inv_d
	};
}

inline static bool
f64x2x2_approx_equal(const F64x2x2 &A, const F64x2x2 &B, F64 epsilon)
{
	return f64x2_approx_equal(A.rows[0], B.rows[0], epsilon)
		&& f64x2_approx_equal(A.rows[1], B.rows[1], epsilon);
}