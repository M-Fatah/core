#pragma once

#include "core/defines.h"
#include "core/math/f64.h"
#include "core/math/f64x3.h"
#include "core/math/f64x3x3.h"
#include "core/math/f64x4.h"
#include "core/validate.h"

union alignas(32) F64x4x4
{
	struct
	{
		F64 m00, m01, m02, m03;
		F64 m10, m11, m12, m13;
		F64 m20, m21, m22, m23;
		F64 m30, m31, m32, m33;
	};
	F64x4 rows[4];

	inline F64x4 &
	operator[](U64 index)
	{
		validate(index < 4, "[MATH][F64x4x4]: Row index out of bounds.");
		return rows[index];
	}

	inline const F64x4 &
	operator[](U64 index) const
	{
		validate(index < 4, "[MATH][F64x4x4]: Row index out of bounds.");
		return rows[index];
	}
};

inline static F64x4x4
operator+(const F64x4x4 &A, const F64x4x4 &B)
{
	return F64x4x4 {
		.m00 = A.m00 + B.m00, .m01 = A.m01 + B.m01, .m02 = A.m02 + B.m02, .m03 = A.m03 + B.m03,
		.m10 = A.m10 + B.m10, .m11 = A.m11 + B.m11, .m12 = A.m12 + B.m12, .m13 = A.m13 + B.m13,
		.m20 = A.m20 + B.m20, .m21 = A.m21 + B.m21, .m22 = A.m22 + B.m22, .m23 = A.m23 + B.m23,
		.m30 = A.m30 + B.m30, .m31 = A.m31 + B.m31, .m32 = A.m32 + B.m32, .m33 = A.m33 + B.m33
	};
}

inline static F64x4x4 &
operator+=(F64x4x4 &A, const F64x4x4 &B)
{
	A = A + B;
	return A;
}

inline static F64x4x4
operator-(const F64x4x4 &M)
{
	return F64x4x4 {
		.m00 = -M.m00, .m01 = -M.m01, .m02 = -M.m02, .m03 = -M.m03,
		.m10 = -M.m10, .m11 = -M.m11, .m12 = -M.m12, .m13 = -M.m13,
		.m20 = -M.m20, .m21 = -M.m21, .m22 = -M.m22, .m23 = -M.m23,
		.m30 = -M.m30, .m31 = -M.m31, .m32 = -M.m32, .m33 = -M.m33
	};
}

inline static F64x4x4
operator-(const F64x4x4 &A, const F64x4x4 &B)
{
	return F64x4x4 {
		.m00 = A.m00 - B.m00, .m01 = A.m01 - B.m01, .m02 = A.m02 - B.m02, .m03 = A.m03 - B.m03,
		.m10 = A.m10 - B.m10, .m11 = A.m11 - B.m11, .m12 = A.m12 - B.m12, .m13 = A.m13 - B.m13,
		.m20 = A.m20 - B.m20, .m21 = A.m21 - B.m21, .m22 = A.m22 - B.m22, .m23 = A.m23 - B.m23,
		.m30 = A.m30 - B.m30, .m31 = A.m31 - B.m31, .m32 = A.m32 - B.m32, .m33 = A.m33 - B.m33
	};
}

inline static F64x4x4 &
operator-=(F64x4x4 &A, const F64x4x4 &B)
{
	A = A - B;
	return A;
}

inline static F64x4x4
operator*(const F64x4x4 &M, F64 s)
{
	return F64x4x4 {
		.m00 = M.m00 * s, .m01 = M.m01 * s, .m02 = M.m02 * s, .m03 = M.m03 * s,
		.m10 = M.m10 * s, .m11 = M.m11 * s, .m12 = M.m12 * s, .m13 = M.m13 * s,
		.m20 = M.m20 * s, .m21 = M.m21 * s, .m22 = M.m22 * s, .m23 = M.m23 * s,
		.m30 = M.m30 * s, .m31 = M.m31 * s, .m32 = M.m32 * s, .m33 = M.m33 * s
	};
}

inline static F64x4x4
operator*(F64 s, const F64x4x4 &M)
{
	return M * s;
}

inline static F64x4x4 &
operator*=(F64x4x4 &M, F64 s)
{
	M = M * s;
	return M;
}

inline static F64x4
operator*(const F64x4 &v, const F64x4x4 &M)
{
	return v.x * M.rows[0] + v.y * M.rows[1] + v.z * M.rows[2] + v.w * M.rows[3];
}

inline static F64x4 &
operator*=(F64x4 &v, const F64x4x4 &M)
{
	v = v * M;
	return v;
}

inline static F64x4x4
operator*(const F64x4x4 &A, const F64x4x4 &B)
{
	return F64x4x4 {
		.m00 = A.m00 * B.m00 + A.m01 * B.m10 + A.m02 * B.m20 + A.m03 * B.m30,
		.m01 = A.m00 * B.m01 + A.m01 * B.m11 + A.m02 * B.m21 + A.m03 * B.m31,
		.m02 = A.m00 * B.m02 + A.m01 * B.m12 + A.m02 * B.m22 + A.m03 * B.m32,
		.m03 = A.m00 * B.m03 + A.m01 * B.m13 + A.m02 * B.m23 + A.m03 * B.m33,

		.m10 = A.m10 * B.m00 + A.m11 * B.m10 + A.m12 * B.m20 + A.m13 * B.m30,
		.m11 = A.m10 * B.m01 + A.m11 * B.m11 + A.m12 * B.m21 + A.m13 * B.m31,
		.m12 = A.m10 * B.m02 + A.m11 * B.m12 + A.m12 * B.m22 + A.m13 * B.m32,
		.m13 = A.m10 * B.m03 + A.m11 * B.m13 + A.m12 * B.m23 + A.m13 * B.m33,

		.m20 = A.m20 * B.m00 + A.m21 * B.m10 + A.m22 * B.m20 + A.m23 * B.m30,
		.m21 = A.m20 * B.m01 + A.m21 * B.m11 + A.m22 * B.m21 + A.m23 * B.m31,
		.m22 = A.m20 * B.m02 + A.m21 * B.m12 + A.m22 * B.m22 + A.m23 * B.m32,
		.m23 = A.m20 * B.m03 + A.m21 * B.m13 + A.m22 * B.m23 + A.m23 * B.m33,

		.m30 = A.m30 * B.m00 + A.m31 * B.m10 + A.m32 * B.m20 + A.m33 * B.m30,
		.m31 = A.m30 * B.m01 + A.m31 * B.m11 + A.m32 * B.m21 + A.m33 * B.m31,
		.m32 = A.m30 * B.m02 + A.m31 * B.m12 + A.m32 * B.m22 + A.m33 * B.m32,
		.m33 = A.m30 * B.m03 + A.m31 * B.m13 + A.m32 * B.m23 + A.m33 * B.m33
	};
}

inline static F64x4x4 &
operator*=(F64x4x4 &A, const F64x4x4 &B)
{
	A = A * B;
	return A;
}

inline static F64x4x4
operator/(const F64x4x4 &M, F64 s)
{
	validate(s != 0.0, "[MATH][F64x4x4]: scalar divisor must be non-zero.");
	return M * (1.0 / s);
}

inline static F64x4x4 &
operator/=(F64x4x4 &M, F64 s)
{
	M = M / s;
	return M;
}

inline static bool
operator==(const F64x4x4 &A, const F64x4x4 &B)
{
	return A.rows[0] == B.rows[0]
		&& A.rows[1] == B.rows[1]
		&& A.rows[2] == B.rows[2]
		&& A.rows[3] == B.rows[3];
}

inline static F64x4x4
f64x4x4_identity()
{
	return F64x4x4 {
		.m00 = 1.0, .m01 = 0.0, .m02 = 0.0, .m03 = 0.0,
		.m10 = 0.0, .m11 = 1.0, .m12 = 0.0, .m13 = 0.0,
		.m20 = 0.0, .m21 = 0.0, .m22 = 1.0, .m23 = 0.0,
		.m30 = 0.0, .m31 = 0.0, .m32 = 0.0, .m33 = 1.0
	};
}

inline static F64x4x4
f64x4x4_transpose(const F64x4x4 &M)
{
	return F64x4x4 {
		.m00 = M.m00, .m01 = M.m10, .m02 = M.m20, .m03 = M.m30,
		.m10 = M.m01, .m11 = M.m11, .m12 = M.m21, .m13 = M.m31,
		.m20 = M.m02, .m21 = M.m12, .m22 = M.m22, .m23 = M.m32,
		.m30 = M.m03, .m31 = M.m13, .m32 = M.m23, .m33 = M.m33
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
f64x4x4_is_invertible(const F64x4x4 &M)
{
	return f64x4x4_determinant(M) != 0.0;
}

inline static F64x4x4
f64x4x4_inverse(const F64x4x4 &M)
{
	F64 d = f64x4x4_determinant(M);
	validate(d != 0.0, "[MATH][F64x4x4]: Matrix must be invertible.");

	F64x4x4 adj = F64x4x4 {
		.m00 = + M.m11 * (M.m22 * M.m33 - M.m23 * M.m32) - M.m12 * (M.m21 * M.m33 - M.m23 * M.m31) + M.m13 * (M.m21 * M.m32 - M.m22 * M.m31),
		.m01 = - M.m01 * (M.m22 * M.m33 - M.m23 * M.m32) + M.m02 * (M.m21 * M.m33 - M.m23 * M.m31) - M.m03 * (M.m21 * M.m32 - M.m22 * M.m31),
		.m02 = + M.m01 * (M.m12 * M.m33 - M.m13 * M.m32) - M.m02 * (M.m11 * M.m33 - M.m13 * M.m31) + M.m03 * (M.m11 * M.m32 - M.m12 * M.m31),
		.m03 = - M.m01 * (M.m12 * M.m23 - M.m13 * M.m22) + M.m02 * (M.m11 * M.m23 - M.m13 * M.m21) - M.m03 * (M.m11 * M.m22 - M.m12 * M.m21),

		.m10 = - M.m10 * (M.m22 * M.m33 - M.m23 * M.m32) + M.m12 * (M.m20 * M.m33 - M.m23 * M.m30) - M.m13 * (M.m20 * M.m32 - M.m22 * M.m30),
		.m11 = + M.m00 * (M.m22 * M.m33 - M.m23 * M.m32) - M.m02 * (M.m20 * M.m33 - M.m23 * M.m30) + M.m03 * (M.m20 * M.m32 - M.m22 * M.m30),
		.m12 = - M.m00 * (M.m12 * M.m33 - M.m13 * M.m32) + M.m02 * (M.m10 * M.m33 - M.m13 * M.m30) - M.m03 * (M.m10 * M.m32 - M.m12 * M.m30),
		.m13 = + M.m00 * (M.m12 * M.m23 - M.m13 * M.m22) - M.m02 * (M.m10 * M.m23 - M.m13 * M.m20) + M.m03 * (M.m10 * M.m22 - M.m12 * M.m20),

		.m20 = + M.m10 * (M.m21 * M.m33 - M.m23 * M.m31) - M.m11 * (M.m20 * M.m33 - M.m23 * M.m30) + M.m13 * (M.m20 * M.m31 - M.m21 * M.m30),
		.m21 = - M.m00 * (M.m21 * M.m33 - M.m23 * M.m31) + M.m01 * (M.m20 * M.m33 - M.m23 * M.m30) - M.m03 * (M.m20 * M.m31 - M.m21 * M.m30),
		.m22 = + M.m00 * (M.m11 * M.m33 - M.m13 * M.m31) - M.m01 * (M.m10 * M.m33 - M.m13 * M.m30) + M.m03 * (M.m10 * M.m31 - M.m11 * M.m30),
		.m23 = - M.m00 * (M.m11 * M.m23 - M.m13 * M.m21) + M.m01 * (M.m10 * M.m23 - M.m13 * M.m20) - M.m03 * (M.m10 * M.m21 - M.m11 * M.m20),

		.m30 = - M.m10 * (M.m21 * M.m32 - M.m22 * M.m31) + M.m11 * (M.m20 * M.m32 - M.m22 * M.m30) - M.m12 * (M.m20 * M.m31 - M.m21 * M.m30),
		.m31 = + M.m00 * (M.m21 * M.m32 - M.m22 * M.m31) - M.m01 * (M.m20 * M.m32 - M.m22 * M.m30) + M.m02 * (M.m20 * M.m31 - M.m21 * M.m30),
		.m32 = - M.m00 * (M.m11 * M.m32 - M.m12 * M.m31) + M.m01 * (M.m10 * M.m32 - M.m12 * M.m30) - M.m02 * (M.m10 * M.m31 - M.m11 * M.m30),
		.m33 = + M.m00 * (M.m11 * M.m22 - M.m12 * M.m21) - M.m01 * (M.m10 * M.m22 - M.m12 * M.m20) + M.m02 * (M.m10 * M.m21 - M.m11 * M.m20)
	};

	return adj * (1.0 / d);
}

inline static F64x4x4
f64x4x4_affine_inverse(const F64x4x4 &M)
{
	validate(M.m03 == 0.0 && M.m13 == 0.0 && M.m23 == 0.0 && M.m33 == 1.0, "[MATH][F64x4x4]: Matrix must be affine.");
	F64x3x3 linear = F64x3x3 {
		.m00 = M.m00, .m01 = M.m01, .m02 = M.m02,
		.m10 = M.m10, .m11 = M.m11, .m12 = M.m12,
		.m20 = M.m20, .m21 = M.m21, .m22 = M.m22
	};
	F64x3x3 linear_inv = f64x3x3_inverse(linear);
	F64x3 translation = F64x3{.x = M.m30, .y = M.m31, .z = M.m32};
	F64x3 inverse_translation = -(translation * linear_inv);
	return F64x4x4 {
		.m00 = linear_inv.m00,        .m01 = linear_inv.m01,        .m02 = linear_inv.m02,        .m03 = 0.0,
		.m10 = linear_inv.m10,        .m11 = linear_inv.m11,        .m12 = linear_inv.m12,        .m13 = 0.0,
		.m20 = linear_inv.m20,        .m21 = linear_inv.m21,        .m22 = linear_inv.m22,        .m23 = 0.0,
		.m30 = inverse_translation.x, .m31 = inverse_translation.y, .m32 = inverse_translation.z, .m33 = 1.0
	};
}

inline static F64x4x4
f64x4x4_translation(F64 dx, F64 dy, F64 dz)
{
	return F64x4x4 {
		.m00 = 1.0, .m01 = 0.0, .m02 = 0.0, .m03 = 0.0,
		.m10 = 0.0, .m11 = 1.0, .m12 = 0.0, .m13 = 0.0,
		.m20 = 0.0, .m21 = 0.0, .m22 = 1.0, .m23 = 0.0,
		.m30 =  dx, .m31 =  dy, .m32 =  dz, .m33 = 1.0
	};
}

inline static F64x4x4
f64x4x4_translation(const F64x3 &t)
{
	return f64x4x4_translation(t.x, t.y, t.z);
}

inline static F64x4x4
f64x4x4_rotation_x(F64 angle_in_radians)
{
	F64 c = f64_cos(angle_in_radians);
	F64 s = f64_sin(angle_in_radians);
	return F64x4x4 {
		.m00 = 1.0, .m01 = 0.0, .m02 = 0.0, .m03 = 0.0,
		.m10 = 0.0, .m11 =   c, .m12 =   s, .m13 = 0.0,
		.m20 = 0.0, .m21 =  -s, .m22 =   c, .m23 = 0.0,
		.m30 = 0.0, .m31 = 0.0, .m32 = 0.0, .m33 = 1.0
	};
}

inline static F64x4x4
f64x4x4_rotation_y(F64 angle_in_radians)
{
	F64 c = f64_cos(angle_in_radians);
	F64 s = f64_sin(angle_in_radians);
	return F64x4x4 {
		.m00 =   c, .m01 = 0.0, .m02 =  -s, .m03 = 0.0,
		.m10 = 0.0, .m11 = 1.0, .m12 = 0.0, .m13 = 0.0,
		.m20 =   s, .m21 = 0.0, .m22 =   c, .m23 = 0.0,
		.m30 = 0.0, .m31 = 0.0, .m32 = 0.0, .m33 = 1.0
	};
}

inline static F64x4x4
f64x4x4_rotation_z(F64 angle_in_radians)
{
	F64 c = f64_cos(angle_in_radians);
	F64 s = f64_sin(angle_in_radians);
	return F64x4x4 {
		.m00 =   c, .m01 =   s, .m02 = 0.0, .m03 = 0.0,
		.m10 =  -s, .m11 =   c, .m12 = 0.0, .m13 = 0.0,
		.m20 = 0.0, .m21 = 0.0, .m22 = 1.0, .m23 = 0.0,
		.m30 = 0.0, .m31 = 0.0, .m32 = 0.0, .m33 = 1.0
	};
}

inline static F64x4x4
f64x4x4_scaling(F64 x, F64 y, F64 z)
{
	return F64x4x4 {
		.m00 =   x, .m01 = 0.0, .m02 = 0.0, .m03 = 0.0,
		.m10 = 0.0, .m11 =   y, .m12 = 0.0, .m13 = 0.0,
		.m20 = 0.0, .m21 = 0.0, .m22 =   z, .m23 = 0.0,
		.m30 = 0.0, .m31 = 0.0, .m32 = 0.0, .m33 = 1.0
	};
}

inline static F64x4x4
f64x4x4_scaling(const F64x3 &s)
{
	return f64x4x4_scaling(s.x, s.y, s.z);
}

inline static F64x3
f64x4x4_transform_point(const F64x4x4 &M, const F64x3 &point)
{
	F64x4 result = F64x4{.x = point.x, .y = point.y, .z = point.z, .w = 1.0} * M;
	validate(result.w != 0.0, "[MATH][F64x4x4]: Transformed point w must be non-zero.");
	return F64x3{.x = result.x / result.w, .y = result.y / result.w, .z = result.z / result.w};
}

inline static F64x3
f64x4x4_transform_vector(const F64x4x4 &M, const F64x3 &vector)
{
	F64x4 result = F64x4{.x = vector.x, .y = vector.y, .z = vector.z, .w = 0.0} * M;
	return F64x3{.x = result.x, .y = result.y, .z = result.z};
}

inline static F64x3
f64x4x4_transform_normal(const F64x4x4 &M, const F64x3 &normal)
{
	F64x3x3 linear = F64x3x3 {
		.m00 = M.m00, .m01 = M.m01, .m02 = M.m02,
		.m10 = M.m10, .m11 = M.m11, .m12 = M.m12,
		.m20 = M.m20, .m21 = M.m21, .m22 = M.m22
	};
	F64x3x3 normal_matrix = f64x3x3_transpose(f64x3x3_inverse(linear));
	return f64x3_normalize(normal * normal_matrix);
}

inline static F64x4x4
f64x4x4_orthographic(F64 left, F64 right, F64 bottom, F64 top, F64 znear, F64 zfar)
{
	validate(right != left, "[MATH][F64x4x4]: Orthographic width must be non-zero.");
	validate(top != bottom, "[MATH][F64x4x4]: Orthographic height must be non-zero.");
	validate(zfar != znear, "[MATH][F64x4x4]: Orthographic depth must be non-zero.");
	return F64x4x4 {
		.m00 =             2.0 / (right - left), .m01 =                              0.0, .m02 =                     0.0, .m03 = 0.0,
		.m10 =                              0.0, .m11 =             2.0 / (top - bottom), .m12 =                     0.0, .m13 = 0.0,
		.m20 =                              0.0, .m21 =                              0.0, .m22 =   -1.0 / (zfar - znear), .m23 = 0.0,
		.m30 = -(right + left) / (right - left), .m31 = -(top + bottom) / (top - bottom), .m32 = -znear / (zfar - znear), .m33 = 1.0
	};
}

inline static F64x4x4
f64x4x4_perspective(F64 fovy_radians, F64 aspect, F64 znear, F64 zfar)
{
	validate(fovy_radians != 0.0, "[MATH][F64x4x4]: Perspective fovy must be non-zero.");
	validate(aspect != 0.0, "[MATH][F64x4x4]: Perspective aspect must be non-zero.");
	validate(zfar != znear, "[MATH][F64x4x4]: Perspective depth must be non-zero.");
	F64 h = f64_tan(fovy_radians * 0.5);
	validate(h != 0.0, "[MATH][F64x4x4]: Perspective tangent must be non-zero.");
	F64 w = aspect * h;
	return F64x4x4 {
		.m00 = 1.0 / w, .m01 =     0.0, .m02 =                              0.0, .m03 =  0.0,
		.m10 =     0.0, .m11 = 1.0 / h, .m12 =                              0.0, .m13 =  0.0,
		.m20 =     0.0, .m21 =     0.0, .m22 =           -zfar / (zfar - znear), .m23 = -1.0,
		.m30 =     0.0, .m31 =     0.0, .m32 = -(zfar * znear) / (zfar - znear), .m33 =  0.0
	};
}

inline static F64x4x4
f64x4x4_look_at(const F64x3 &eye, const F64x3 &target, const F64x3 &up)
{
	F64x3 axis_z_source = eye - target;
	validate(f64x3_length_squared(axis_z_source) != 0.0, "[MATH][F64x4x4]: look_at eye and target must differ.");
	F64x3 axis_z = f64x3_normalize(axis_z_source);
	F64x3 axis_x_source = f64x3_cross(up, axis_z);
	validate(f64x3_length_squared(axis_x_source) != 0.0, "[MATH][F64x4x4]: look_at up must not be parallel to view direction.");
	F64x3 axis_x = f64x3_normalize(axis_x_source);
	F64x3 axis_y = f64x3_cross(axis_z, axis_x);
	F64x3 t = F64x3{.x = -f64x3_dot(eye, axis_x), .y = -f64x3_dot(eye, axis_y), .z = -f64x3_dot(eye, axis_z)};
	return F64x4x4 {
		.m00 = axis_x.x, .m01 = axis_y.x, .m02 = axis_z.x, .m03 = 0.0,
		.m10 = axis_x.y, .m11 = axis_y.y, .m12 = axis_z.y, .m13 = 0.0,
		.m20 = axis_x.z, .m21 = axis_y.z, .m22 = axis_z.z, .m23 = 0.0,
		.m30 =      t.x, .m31 =      t.y, .m32 =      t.z, .m33 = 1.0
	};
}

inline static bool
f64x4x4_approx_equal(const F64x4x4 &A, const F64x4x4 &B, F64 epsilon)
{
	return f64x4_approx_equal(A.rows[0], B.rows[0], epsilon)
		&& f64x4_approx_equal(A.rows[1], B.rows[1], epsilon)
		&& f64x4_approx_equal(A.rows[2], B.rows[2], epsilon)
		&& f64x4_approx_equal(A.rows[3], B.rows[3], epsilon);
}