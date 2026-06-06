#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/math/f32.h"
#include "core/math/f32x3.h"
#include "core/math/f32x3x3.h"
#include "core/math/f32x4.h"

union alignas(16) F32x4x4
{
	struct
	{
		F32 m00, m01, m02, m03;
		F32 m10, m11, m12, m13;
		F32 m20, m21, m22, m23;
		F32 m30, m31, m32, m33;
	};
	F32x4 rows[4];

	inline F32x4 &
	operator[](U64 index)
	{
		validate(index < 4, "[MATH][F32x4x4]: Row index out of bounds.");
		return rows[index];
	}

	inline const F32x4 &
	operator[](U64 index) const
	{
		validate(index < 4, "[MATH][F32x4x4]: Row index out of bounds.");
		return rows[index];
	}
};

inline static F32x4x4
operator+(const F32x4x4 &A, const F32x4x4 &B)
{
	return F32x4x4 {
		.m00 = A.m00 + B.m00, .m01 = A.m01 + B.m01, .m02 = A.m02 + B.m02, .m03 = A.m03 + B.m03,
		.m10 = A.m10 + B.m10, .m11 = A.m11 + B.m11, .m12 = A.m12 + B.m12, .m13 = A.m13 + B.m13,
		.m20 = A.m20 + B.m20, .m21 = A.m21 + B.m21, .m22 = A.m22 + B.m22, .m23 = A.m23 + B.m23,
		.m30 = A.m30 + B.m30, .m31 = A.m31 + B.m31, .m32 = A.m32 + B.m32, .m33 = A.m33 + B.m33
	};
}

inline static F32x4x4 &
operator+=(F32x4x4 &A, const F32x4x4 &B)
{
	A = A + B;
	return A;
}

inline static F32x4x4
operator-(const F32x4x4 &M)
{
	return F32x4x4 {
		.m00 = -M.m00, .m01 = -M.m01, .m02 = -M.m02, .m03 = -M.m03,
		.m10 = -M.m10, .m11 = -M.m11, .m12 = -M.m12, .m13 = -M.m13,
		.m20 = -M.m20, .m21 = -M.m21, .m22 = -M.m22, .m23 = -M.m23,
		.m30 = -M.m30, .m31 = -M.m31, .m32 = -M.m32, .m33 = -M.m33
	};
}

inline static F32x4x4
operator-(const F32x4x4 &A, const F32x4x4 &B)
{
	return F32x4x4 {
		.m00 = A.m00 - B.m00, .m01 = A.m01 - B.m01, .m02 = A.m02 - B.m02, .m03 = A.m03 - B.m03,
		.m10 = A.m10 - B.m10, .m11 = A.m11 - B.m11, .m12 = A.m12 - B.m12, .m13 = A.m13 - B.m13,
		.m20 = A.m20 - B.m20, .m21 = A.m21 - B.m21, .m22 = A.m22 - B.m22, .m23 = A.m23 - B.m23,
		.m30 = A.m30 - B.m30, .m31 = A.m31 - B.m31, .m32 = A.m32 - B.m32, .m33 = A.m33 - B.m33
	};
}

inline static F32x4x4 &
operator-=(F32x4x4 &A, const F32x4x4 &B)
{
	A = A - B;
	return A;
}

inline static F32x4x4
operator*(const F32x4x4 &M, F32 s)
{
	return F32x4x4 {
		.m00 = M.m00 * s, .m01 = M.m01 * s, .m02 = M.m02 * s, .m03 = M.m03 * s,
		.m10 = M.m10 * s, .m11 = M.m11 * s, .m12 = M.m12 * s, .m13 = M.m13 * s,
		.m20 = M.m20 * s, .m21 = M.m21 * s, .m22 = M.m22 * s, .m23 = M.m23 * s,
		.m30 = M.m30 * s, .m31 = M.m31 * s, .m32 = M.m32 * s, .m33 = M.m33 * s
	};
}

inline static F32x4x4
operator*(F32 s, const F32x4x4 &M)
{
	return M * s;
}

inline static F32x4x4 &
operator*=(F32x4x4 &M, F32 s)
{
	M = M * s;
	return M;
}

inline static F32x4
operator*(const F32x4 &v, const F32x4x4 &M)
{
	return v.x * M.rows[0] + v.y * M.rows[1] + v.z * M.rows[2] + v.w * M.rows[3];
}

inline static F32x4 &
operator*=(F32x4 &v, const F32x4x4 &M)
{
	v = v * M;
	return v;
}

inline static F32x4x4
operator*(const F32x4x4 &A, const F32x4x4 &B)
{
	return F32x4x4 {
		.m00 = A.m00 * B.m00 + A.m01 * B.m10 + A.m02 * B.m20 + A.m03 * B.m30, .m01 = A.m00 * B.m01 + A.m01 * B.m11 + A.m02 * B.m21 + A.m03 * B.m31, .m02 = A.m00 * B.m02 + A.m01 * B.m12 + A.m02 * B.m22 + A.m03 * B.m32, .m03 = A.m00 * B.m03 + A.m01 * B.m13 + A.m02 * B.m23 + A.m03 * B.m33,
		.m10 = A.m10 * B.m00 + A.m11 * B.m10 + A.m12 * B.m20 + A.m13 * B.m30, .m11 = A.m10 * B.m01 + A.m11 * B.m11 + A.m12 * B.m21 + A.m13 * B.m31, .m12 = A.m10 * B.m02 + A.m11 * B.m12 + A.m12 * B.m22 + A.m13 * B.m32, .m13 = A.m10 * B.m03 + A.m11 * B.m13 + A.m12 * B.m23 + A.m13 * B.m33,
		.m20 = A.m20 * B.m00 + A.m21 * B.m10 + A.m22 * B.m20 + A.m23 * B.m30, .m21 = A.m20 * B.m01 + A.m21 * B.m11 + A.m22 * B.m21 + A.m23 * B.m31, .m22 = A.m20 * B.m02 + A.m21 * B.m12 + A.m22 * B.m22 + A.m23 * B.m32, .m23 = A.m20 * B.m03 + A.m21 * B.m13 + A.m22 * B.m23 + A.m23 * B.m33,
		.m30 = A.m30 * B.m00 + A.m31 * B.m10 + A.m32 * B.m20 + A.m33 * B.m30, .m31 = A.m30 * B.m01 + A.m31 * B.m11 + A.m32 * B.m21 + A.m33 * B.m31, .m32 = A.m30 * B.m02 + A.m31 * B.m12 + A.m32 * B.m22 + A.m33 * B.m32, .m33 = A.m30 * B.m03 + A.m31 * B.m13 + A.m32 * B.m23 + A.m33 * B.m33
	};
}

inline static F32x4x4 &
operator*=(F32x4x4 &A, const F32x4x4 &B)
{
	A = A * B;
	return A;
}

inline static F32x4x4
operator/(const F32x4x4 &M, F32 s)
{
	validate(s != 0.0f, "[MATH][F32x4x4]: scalar divisor must be non-zero.");
	return M * (1.0f / s);
}

inline static F32x4x4 &
operator/=(F32x4x4 &M, F32 s)
{
	M = M / s;
	return M;
}

inline static bool
operator==(const F32x4x4 &A, const F32x4x4 &B)
{
	return A.rows[0] == B.rows[0]
		&& A.rows[1] == B.rows[1]
		&& A.rows[2] == B.rows[2]
		&& A.rows[3] == B.rows[3];
}

#include "core/math/quaternion.h"

inline static F32x4x4
f32x4x4_from_quaternion(const Quaternion &q_in)
{
	F32 length_squared = q_in.w * q_in.w + q_in.x * q_in.x + q_in.y * q_in.y + q_in.z * q_in.z;
	validate(length_squared != 0.0f, "[MATH][Quaternion]: Cannot normalize zero-length quaternion.");
	Quaternion q = q_in / f32_sqrt(length_squared);
	F32 w = q.w, x = q.x, y = q.y, z = q.z;

	return F32x4x4 {
		.m00 = 1.0f - 2.0f * y * y - 2.0f * z * z,  .m01 = 2.0f * x * y + 2.0f * z * w,        .m02 = 2.0f * x * z - 2.0f * y * w,
		.m10 = 2.0f * x * y - 2.0f * z * w,         .m11 = 1.0f - 2.0f * x * x - 2.0f * z * z, .m12 = 2.0f * y * z + 2.0f * x * w,
		.m20 = 2.0f * x * z + 2.0f * y * w,         .m21 = 2.0f * y * z - 2.0f * x * w,        .m22 = 1.0f - 2.0f * x * x - 2.0f * y * y,
		.m33 = 1.0f
	};
}

inline static F32x4x4
f32x4x4_from_trs(const F32x3 &translation, const Quaternion &rotation, const F32x3 &scale)
{
	F32x4x4 M = f32x4x4_from_quaternion(rotation);
	M.rows[0] *= scale.x;
	M.rows[1] *= scale.y;
	M.rows[2] *= scale.z;
	M.m30 = translation.x;
	M.m31 = translation.y;
	M.m32 = translation.z;
	return M;
}

inline static F32x4x4
f32x4x4_identity()
{
	return F32x4x4 {
		.m00 = 1.0f, .m01 = 0.0f, .m02 = 0.0f, .m03 = 0.0f,
		.m10 = 0.0f, .m11 = 1.0f, .m12 = 0.0f, .m13 = 0.0f,
		.m20 = 0.0f, .m21 = 0.0f, .m22 = 1.0f, .m23 = 0.0f,
		.m30 = 0.0f, .m31 = 0.0f, .m32 = 0.0f, .m33 = 1.0f
	};
}

inline static F32x4x4
f32x4x4_transpose(const F32x4x4 &M)
{
	return F32x4x4 {
		.m00 = M.m00, .m01 = M.m10, .m02 = M.m20, .m03 = M.m30,
		.m10 = M.m01, .m11 = M.m11, .m12 = M.m21, .m13 = M.m31,
		.m20 = M.m02, .m21 = M.m12, .m22 = M.m22, .m23 = M.m32,
		.m30 = M.m03, .m31 = M.m13, .m32 = M.m23, .m33 = M.m33
	};
}

inline static F32
f32x4x4_determinant(const F32x4x4 &M)
{
	return (M.m00 * M.m11 - M.m01 * M.m10) * (M.m22 * M.m33 - M.m23 * M.m32)
		 - (M.m00 * M.m12 - M.m02 * M.m10) * (M.m21 * M.m33 - M.m23 * M.m31)
		 + (M.m00 * M.m13 - M.m03 * M.m10) * (M.m21 * M.m32 - M.m22 * M.m31)
		 + (M.m01 * M.m12 - M.m02 * M.m11) * (M.m20 * M.m33 - M.m23 * M.m30)
		 - (M.m01 * M.m13 - M.m03 * M.m11) * (M.m20 * M.m32 - M.m22 * M.m30)
		 + (M.m02 * M.m13 - M.m03 * M.m12) * (M.m20 * M.m31 - M.m21 * M.m30);
}

inline static bool
f32x4x4_is_invertible(const F32x4x4 &M)
{
	return f32x4x4_determinant(M) != 0.0f;
}

inline static F32x4x4
f32x4x4_inverse(const F32x4x4 &M)
{
	F32 d = f32x4x4_determinant(M);
	validate(d != 0.0f, "[MATH][F32x4x4]: Matrix must be invertible.");

	F32x4x4 adj = F32x4x4 {
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

	return adj * (1.0f / d);
}

inline static F32x4x4
f32x4x4_affine_inverse(const F32x4x4 &M)
{
	validate(M.m03 == 0.0f && M.m13 == 0.0f && M.m23 == 0.0f && M.m33 == 1.0f, "[MATH][F32x4x4]: Matrix must be affine.");
	F32x3x3 linear = F32x3x3 {
		.m00 = M.m00, .m01 = M.m01, .m02 = M.m02,
		.m10 = M.m10, .m11 = M.m11, .m12 = M.m12,
		.m20 = M.m20, .m21 = M.m21, .m22 = M.m22
	};
	F32x3x3 linear_inv = f32x3x3_inverse(linear);
	F32x3 translation = F32x3{.x = M.m30, .y = M.m31, .z = M.m32};
	F32x3 inverse_translation = -(translation * linear_inv);
	return F32x4x4 {
		.m00 = linear_inv.m00,        .m01 = linear_inv.m01,        .m02 = linear_inv.m02,        .m03 = 0.0f,
		.m10 = linear_inv.m10,        .m11 = linear_inv.m11,        .m12 = linear_inv.m12,        .m13 = 0.0f,
		.m20 = linear_inv.m20,        .m21 = linear_inv.m21,        .m22 = linear_inv.m22,        .m23 = 0.0f,
		.m30 = inverse_translation.x, .m31 = inverse_translation.y, .m32 = inverse_translation.z, .m33 = 1.0f
	};
}

inline static F32x3
f32x4x4_axis_x(const F32x4x4 &M)
{
	return f32x3_normalize(F32x3{.x = M.m00, .y = M.m01, .z = M.m02});
}

inline static F32x3
f32x4x4_axis_y(const F32x4x4 &M)
{
	return f32x3_normalize(F32x3{.x = M.m10, .y = M.m11, .z = M.m12});
}

inline static F32x3
f32x4x4_axis_z(const F32x4x4 &M)
{
	return f32x3_normalize(F32x3{.x = M.m20, .y = M.m21, .z = M.m22});
}

inline static F32x3
f32x4x4_transform_point(const F32x4x4 &M, const F32x3 &point)
{
	F32x4 result = F32x4{.x = point.x, .y = point.y, .z = point.z, .w = 1.0f} * M;
	validate(result.w != 0.0f, "[MATH][F32x4x4]: Transformed point w must be non-zero.");
	return F32x3{.x = result.x / result.w, .y = result.y / result.w, .z = result.z / result.w};
}

inline static F32x3
f32x4x4_transform_vector(const F32x4x4 &M, const F32x3 &vector)
{
	F32x4 result = F32x4{.x = vector.x, .y = vector.y, .z = vector.z, .w = 0.0f} * M;
	return F32x3{.x = result.x, .y = result.y, .z = result.z};
}

inline static F32x3
f32x4x4_transform_normal(const F32x4x4 &M, const F32x3 &normal)
{
	F32x3x3 linear = F32x3x3 {
		.m00 = M.m00, .m01 = M.m01, .m02 = M.m02,
		.m10 = M.m10, .m11 = M.m11, .m12 = M.m12,
		.m20 = M.m20, .m21 = M.m21, .m22 = M.m22
	};
	F32x3x3 normal_matrix = f32x3x3_transpose(f32x3x3_inverse(linear));
	return f32x3_normalize(normal * normal_matrix);
}

inline static F32x4x4
f32x4x4_translation(F32 dx, F32 dy, F32 dz)
{
	return F32x4x4 {
		.m00 = 1.0f, .m01 = 0.0f, .m02 = 0.0f, .m03 = 0.0f,
		.m10 = 0.0f, .m11 = 1.0f, .m12 = 0.0f, .m13 = 0.0f,
		.m20 = 0.0f, .m21 = 0.0f, .m22 = 1.0f, .m23 = 0.0f,
		.m30 =   dx, .m31 =   dy, .m32 =   dz, .m33 = 1.0f
	};
}

inline static F32x4x4
f32x4x4_translation(const F32x3 &t)
{
	return f32x4x4_translation(t.x, t.y, t.z);
}

inline static F32x4x4
f32x4x4_rotation_x(F32 angle_in_radians)
{
	F32 c = f32_cos(angle_in_radians);
	F32 s = f32_sin(angle_in_radians);
	return F32x4x4 {
		.m00 = 1.0f, .m01 = 0.0f, .m02 = 0.0f, .m03 = 0.0f,
		.m10 = 0.0f, .m11 =    c, .m12 =    s, .m13 = 0.0f,
		.m20 = 0.0f, .m21 =   -s, .m22 =    c, .m23 = 0.0f,
		.m30 = 0.0f, .m31 = 0.0f, .m32 = 0.0f, .m33 = 1.0f
	};
}

inline static F32x4x4
f32x4x4_rotation_y(F32 angle_in_radians)
{
	F32 c = f32_cos(angle_in_radians);
	F32 s = f32_sin(angle_in_radians);
	return F32x4x4 {
		.m00 =    c, .m01 = 0.0f, .m02 =   -s, .m03 = 0.0f,
		.m10 = 0.0f, .m11 = 1.0f, .m12 = 0.0f, .m13 = 0.0f,
		.m20 =    s, .m21 = 0.0f, .m22 =    c, .m23 = 0.0f,
		.m30 = 0.0f, .m31 = 0.0f, .m32 = 0.0f, .m33 = 1.0f
	};
}

inline static F32x4x4
f32x4x4_rotation_z(F32 angle_in_radians)
{
	F32 c = f32_cos(angle_in_radians);
	F32 s = f32_sin(angle_in_radians);
	return F32x4x4 {
		.m00 =    c, .m01 =    s, .m02 = 0.0f, .m03 = 0.0f,
		.m10 =   -s, .m11 =    c, .m12 = 0.0f, .m13 = 0.0f,
		.m20 = 0.0f, .m21 = 0.0f, .m22 = 1.0f, .m23 = 0.0f,
		.m30 = 0.0f, .m31 = 0.0f, .m32 = 0.0f, .m33 = 1.0f
	};
}

inline static F32x4x4
f32x4x4_scaling(F32 x, F32 y, F32 z)
{
	return F32x4x4 {
		.m00 =    x, .m01 = 0.0f, .m02 = 0.0f, .m03 = 0.0f,
		.m10 = 0.0f, .m11 =    y, .m12 = 0.0f, .m13 = 0.0f,
		.m20 = 0.0f, .m21 = 0.0f, .m22 =    z, .m23 = 0.0f,
		.m30 = 0.0f, .m31 = 0.0f, .m32 = 0.0f, .m33 = 1.0f
	};
}

inline static F32x4x4
f32x4x4_scaling(const F32x3 &s)
{
	return f32x4x4_scaling(s.x, s.y, s.z);
}

inline static bool
f32x4x4_decompose(const F32x4x4 &M, F32x3 *out_translation, Quaternion *out_rotation, F32x3 *out_scale)
{
	if (out_translation)
		*out_translation = F32x3{.x = M.m30, .y = M.m31, .z = M.m32};

	F32x3 row0 = F32x3{.x = M.m00, .y = M.m01, .z = M.m02};
	F32x3 row1 = F32x3{.x = M.m10, .y = M.m11, .z = M.m12};
	F32x3 row2 = F32x3{.x = M.m20, .y = M.m21, .z = M.m22};

	F32 sx = f32x3_length(row0);
	F32 sy = f32x3_length(row1);
	F32 sz = f32x3_length(row2);

	validate(sx != 0.0f && sy != 0.0f && sz != 0.0f, "[MATH][F32x4x4]: Cannot decompose matrix with zero scale.");

	if (out_scale)
		*out_scale = F32x3{.x = sx, .y = sy, .z = sz};

	F32x3 rx = row0 / sx;
	F32x3 ry = row1 / sy;
	F32x3 rz = row2 / sz;

	if (out_rotation)
	{
		F32 trace = rx.x + ry.y + rz.z;
		if (trace > 0.0f)
		{
			F32 s = f32_sqrt(trace + 1.0f) * 2.0f;
			out_rotation->w = 0.25f * s;
			out_rotation->x = (ry.z - rz.y) / s;
			out_rotation->y = (rz.x - rx.z) / s;
			out_rotation->z = (rx.y - ry.x) / s;
		}
		else if (rx.x > ry.y && rx.x > rz.z)
		{
			F32 s = f32_sqrt(1.0f + rx.x - ry.y - rz.z) * 2.0f;
			out_rotation->w = (ry.z - rz.y) / s;
			out_rotation->x = 0.25f * s;
			out_rotation->y = (ry.x + rx.y) / s;
			out_rotation->z = (rz.x + rx.z) / s;
		}
		else if (ry.y > rz.z)
		{
			F32 s = f32_sqrt(1.0f + ry.y - rx.x - rz.z) * 2.0f;
			out_rotation->w = (rz.x - rx.z) / s;
			out_rotation->x = (ry.x + rx.y) / s;
			out_rotation->y = 0.25f * s;
			out_rotation->z = (rz.y + ry.z) / s;
		}
		else
		{
			F32 s = f32_sqrt(1.0f + rz.z - rx.x - ry.y) * 2.0f;
			out_rotation->w = (rx.y - ry.x) / s;
			out_rotation->x = (rz.x + rx.z) / s;
			out_rotation->y = (rz.y + ry.z) / s;
			out_rotation->z = 0.25f * s;
		}
		F32 length_squared = out_rotation->w * out_rotation->w + out_rotation->x * out_rotation->x + out_rotation->y * out_rotation->y + out_rotation->z * out_rotation->z;
		validate(length_squared != 0.0f, "[MATH][Quaternion]: Cannot normalize zero-length quaternion.");
		*out_rotation = *out_rotation / f32_sqrt(length_squared);
	}
	return true;
}

inline static F32x4x4
f32x4x4_orthographic(F32 left, F32 right, F32 bottom, F32 top, F32 znear, F32 zfar)
{
	validate(right != left, "[MATH][F32x4x4]: Orthographic width must be non-zero.");
	validate(top != bottom, "[MATH][F32x4x4]: Orthographic height must be non-zero.");
	validate(zfar != znear, "[MATH][F32x4x4]: Orthographic depth must be non-zero.");
	return F32x4x4 {
		.m00 =            2.0f / (right - left), .m01 =                             0.0f, .m02 =                    0.0f, .m03 = 0.0f,
		.m10 =                             0.0f, .m11 =            2.0f / (top - bottom), .m12 =                    0.0f, .m13 = 0.0f,
		.m20 =                             0.0f, .m21 =                             0.0f, .m22 =  -1.0f / (zfar - znear), .m23 = 0.0f,
		.m30 = -(right + left) / (right - left), .m31 = -(top + bottom) / (top - bottom), .m32 = -znear / (zfar - znear), .m33 = 1.0f
	};
}

inline static F32x4x4
f32x4x4_perspective(F32 fovy_radians, F32 aspect, F32 znear, F32 zfar)
{
	validate(fovy_radians != 0.0f, "[MATH][F32x4x4]: Perspective fovy must be non-zero.");
	validate(aspect != 0.0f, "[MATH][F32x4x4]: Perspective aspect must be non-zero.");
	validate(zfar != znear, "[MATH][F32x4x4]: Perspective depth must be non-zero.");
	F32 h = f32_tan(fovy_radians * 0.5f);
	validate(h != 0.0f, "[MATH][F32x4x4]: Perspective tangent must be non-zero.");
	F32 w = aspect * h;
	return F32x4x4 {
		.m00 = 1.0f / w, .m01 =     0.0f, .m02 =                             0.0f, .m03 =  0.0f,
		.m10 =     0.0f, .m11 = 1.0f / h, .m12 =                             0.0f, .m13 =  0.0f,
		.m20 =     0.0f, .m21 =     0.0f, .m22 =           -zfar / (zfar - znear), .m23 = -1.0f,
		.m30 =     0.0f, .m31 =     0.0f, .m32 = -(zfar * znear) / (zfar - znear), .m33 =  0.0f
	};
}

inline static F32x4x4
f32x4x4_look_at(const F32x3 &eye, const F32x3 &target, const F32x3 &up)
{
	F32x3 axis_z_source = eye - target;
	validate(f32x3_length_squared(axis_z_source) != 0.0f, "[MATH][F32x4x4]: look_at eye and target must differ.");
	F32x3 axis_z = f32x3_normalize(axis_z_source);
	F32x3 axis_x_source = f32x3_cross(up, axis_z);
	validate(f32x3_length_squared(axis_x_source) != 0.0f, "[MATH][F32x4x4]: look_at up must not be parallel to view direction.");
	F32x3 axis_x = f32x3_normalize(axis_x_source);
	F32x3 axis_y = f32x3_cross(axis_z, axis_x);

	F32x3 t = F32x3 {.x = -f32x3_dot(eye, axis_x), .y = -f32x3_dot(eye, axis_y), .z = -f32x3_dot(eye, axis_z)};
	return F32x4x4 {
		.m00 = axis_x.x, .m01 = axis_y.x, .m02 = axis_z.x, .m03 = 0.0f,
		.m10 = axis_x.y, .m11 = axis_y.y, .m12 = axis_z.y, .m13 = 0.0f,
		.m20 = axis_x.z, .m21 = axis_y.z, .m22 = axis_z.z, .m23 = 0.0f,
		.m30 =      t.x, .m31 =      t.y, .m32 =      t.z, .m33 = 1.0f
	};
}

inline static F32x3
f32x3_project(const F32x3 &world, const F32x4x4 &view_projection, const F32x4 &viewport)
{
	validate(viewport.z != 0.0f && viewport.w != 0.0f, "[MATH][F32x4x4]: Project viewport size must be non-zero.");
	F32x4 clip = F32x4{.x = world.x, .y = world.y, .z = world.z, .w = 1.0f} * view_projection;
	validate(clip.w != 0.0f, "[MATH][F32x4x4]: Project clip w must be non-zero.");

	F32x3 ndc = F32x3{.x = clip.x / clip.w, .y = clip.y / clip.w, .z = clip.z / clip.w};
	return F32x3 {
		.x = viewport.x + (ndc.x * 0.5f + 0.5f) * viewport.z,
		.y = viewport.y + (ndc.y * 0.5f + 0.5f) * viewport.w,
		.z = ndc.z
	};
}

inline static F32x3
f32x3_unproject(const F32x3 &screen, const F32x4x4 &view_projection_inverse, const F32x4 &viewport)
{
	validate(viewport.z != 0.0f && viewport.w != 0.0f, "[MATH][F32x4x4]: Unproject viewport size must be non-zero.");
	F32x4 ndc = F32x4 {
		.x = ((screen.x - viewport.x) / viewport.z) * 2.0f - 1.0f,
		.y = ((screen.y - viewport.y) / viewport.w) * 2.0f - 1.0f,
		.z = screen.z,
		.w = 1.0f
	};
	F32x4 world = ndc * view_projection_inverse;
	validate(world.w != 0.0f, "[MATH][F32x4x4]: Unproject world w must be non-zero.");
	return F32x3{.x = world.x / world.w, .y = world.y / world.w, .z = world.z / world.w};
}

inline static bool
f32x4x4_approx_equal(const F32x4x4 &A, const F32x4x4 &B, F32 epsilon)
{
	return f32x4_approx_equal(A.rows[0], B.rows[0], epsilon)
		&& f32x4_approx_equal(A.rows[1], B.rows[1], epsilon)
		&& f32x4_approx_equal(A.rows[2], B.rows[2], epsilon)
		&& f32x4_approx_equal(A.rows[3], B.rows[3], epsilon);
}