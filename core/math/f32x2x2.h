#pragma once

#include "core/defines.h"
#include "core/math/f32.h"
#include "core/math/f32x2.h"
#include "core/validate.h"

union F32x2x2
{
	struct
	{
		F32 m00, m01;
		F32 m10, m11;
	};
	F32x2 rows[2];

	inline F32x2 &
	operator[](U64 index)
	{
		validate(index < 2, "[MATH][F32x2x2]: Row index out of bounds.");
		return rows[index];
	}

	inline const F32x2 &
	operator[](U64 index) const
	{
		validate(index < 2, "[MATH][F32x2x2]: Row index out of bounds.");
		return rows[index];
	}
};

inline static F32x2x2
operator+(const F32x2x2 &A, const F32x2x2 &B)
{
	return F32x2x2 {
		.m00 = A.m00 + B.m00, .m01 = A.m01 + B.m01,
		.m10 = A.m10 + B.m10, .m11 = A.m11 + B.m11
	};
}

inline static F32x2x2
operator-(const F32x2x2 &M)
{
	return F32x2x2 {
		.m00 = -M.m00, .m01 = -M.m01,
		.m10 = -M.m10, .m11 = -M.m11
	};
}

inline static F32x2x2
operator-(const F32x2x2 &A, const F32x2x2 &B)
{
	return F32x2x2 {
		.m00 = A.m00 - B.m00, .m01 = A.m01 - B.m01,
		.m10 = A.m10 - B.m10, .m11 = A.m11 - B.m11
	};
}

inline static F32x2x2
operator*(const F32x2x2 &M, F32 s)
{
	return F32x2x2 {
		.m00 = M.m00 * s, .m01 = M.m01 * s,
		.m10 = M.m10 * s, .m11 = M.m11 * s
	};
}

inline static F32x2x2
operator*(F32 s, const F32x2x2 &M)
{
	return M * s;
}

inline static F32x2
operator*(const F32x2 &v, const F32x2x2 &M)
{
	return F32x2 {
		.x = v.x * M.m00 + v.y * M.m10,
		.y = v.x * M.m01 + v.y * M.m11
	};
}

inline static F32x2x2
operator*(const F32x2x2 &A, const F32x2x2 &B)
{
	return F32x2x2 {
		.m00 = A.m00 * B.m00 + A.m01 * B.m10, .m01 = A.m00 * B.m01 + A.m01 * B.m11,
		.m10 = A.m10 * B.m00 + A.m11 * B.m10, .m11 = A.m10 * B.m01 + A.m11 * B.m11
	};
}

inline static F32x2x2
operator/(const F32x2x2 &M, F32 s)
{
	validate(s != 0.0f, "[MATH][F32x2x2]: scalar divisor must be non-zero.");
	return M * (1.0f / s);
}

inline static bool
operator==(const F32x2x2 &A, const F32x2x2 &B)
{
	return A.m00 == B.m00 && A.m01 == B.m01 && A.m10 == B.m10 && A.m11 == B.m11;
}

inline static F32x2x2
f32x2x2_identity()
{
	return F32x2x2 {
		.m00 = 1.0f, .m01 = 0.0f,
		.m10 = 0.0f, .m11 = 1.0f
	};
}

inline static F32x2x2
f32x2x2_transpose(const F32x2x2 &M)
{
	return F32x2x2 {
		.m00 = M.m00, .m01 = M.m10,
		.m10 = M.m01, .m11 = M.m11
	};
}

inline static F32
f32x2x2_determinant(const F32x2x2 &M)
{
	return M.m00 * M.m11 - M.m01 * M.m10;
}

inline static bool
f32x2x2_is_invertible(const F32x2x2 &M)
{
	return f32x2x2_determinant(M) != 0.0f;
}

inline static F32x2x2
f32x2x2_inverse(const F32x2x2 &M)
{
	F32 d = f32x2x2_determinant(M);
	validate(d != 0.0f, "[MATH][F32x2x2]: Matrix must be invertible.");
	F32 inv_d = 1.0f / d;
	return F32x2x2 {
		.m00 =  M.m11 * inv_d, .m01 = -M.m01 * inv_d,
		.m10 = -M.m10 * inv_d, .m11 =  M.m00 * inv_d
	};
}

inline static bool
f32x2x2_approx_equal(const F32x2x2 &A, const F32x2x2 &B, F32 epsilon)
{
	return f32x2_approx_equal(A.rows[0], B.rows[0], epsilon)
		&& f32x2_approx_equal(A.rows[1], B.rows[1], epsilon);
}