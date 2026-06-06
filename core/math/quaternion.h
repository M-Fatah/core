#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/math/f32.h"
#include "core/math/f32x3.h"

struct Quaternion
{
	F32 w, x, y, z;
};

inline static Quaternion
operator+(const Quaternion &p, const Quaternion &q)
{
	return Quaternion{.w = p.w + q.w, .x = p.x + q.x, .y = p.y + q.y, .z = p.z + q.z};
}

inline static Quaternion &
operator+=(Quaternion &p, const Quaternion &q)
{
	p = p + q;
	return p;
}

inline static Quaternion
operator-(const Quaternion &q)
{
	return Quaternion{.w = -q.w, .x = -q.x, .y = -q.y, .z = -q.z};
}

inline static Quaternion
operator-(const Quaternion &p, const Quaternion &q)
{
	return Quaternion{.w = p.w - q.w, .x = p.x - q.x, .y = p.y - q.y, .z = p.z - q.z};
}

inline static Quaternion &
operator-=(Quaternion &p, const Quaternion &q)
{
	p = p - q;
	return p;
}

inline static Quaternion
operator*(const Quaternion &q, F32 s)
{
	return Quaternion{.w = q.w * s, .x = q.x * s, .y = q.y * s, .z = q.z * s};
}

inline static Quaternion
operator*(F32 s, const Quaternion &q)
{
	return q * s;
}

inline static Quaternion &
operator*=(Quaternion &q, F32 s)
{
	q = q * s;
	return q;
}

inline static F32x3
operator*(const F32x3 &v, const Quaternion &q)
{
	F32x3 u = F32x3{.x = q.x, .y = q.y, .z = q.z};
	F32x3 t = 2.0f * f32x3_cross(u, v);
	return v + q.w * t + f32x3_cross(u, t);
}

inline static F32x3 &
operator*=(F32x3 &v, const Quaternion &q)
{
	v = v * q;
	return v;
}

inline static Quaternion
operator*(const Quaternion &p, const Quaternion &q)
{
	return Quaternion {
		.w = p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z,
		.x = p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y,
		.y = p.w * q.y - p.x * q.z + p.y * q.w + p.z * q.x,
		.z = p.w * q.z + p.x * q.y - p.y * q.x + p.z * q.w
	};
}

inline static Quaternion &
operator*=(Quaternion &p, const Quaternion &q)
{
	p = p * q;
	return p;
}

inline static Quaternion
operator/(const Quaternion &q, F32 s)
{
	validate(s != 0.0f, "[MATH][Quaternion]: scalar divisor must be non-zero.");
	return q * (1.0f / s);
}

inline static Quaternion &
operator/=(Quaternion &q, F32 s)
{
	q = q / s;
	return q;
}

inline static bool
operator==(const Quaternion &p, const Quaternion &q)
{
	return p.w == q.w && p.x == q.x && p.y == q.y && p.z == q.z;
}

#include "core/math/f32x4x4.h"

inline static Quaternion
quaternion_from_axis_angle(const F32x3 &axis, F32 angle_in_radians)
{
	validate(f32x3_length_squared(axis) != 0.0f, "[MATH][Quaternion]: axis must be non-zero.");
	F32 half = angle_in_radians * 0.5f;
	F32 s    = f32_sin(half);
	return Quaternion{.w = f32_cos(half), .x = s * axis.x, .y = s * axis.y, .z = s * axis.z};
}

inline static Quaternion
quaternion_from_euler_angles(const F32x3 &angles_in_radians)
{
	F32 cx = f32_cos(angles_in_radians.x * 0.5f);
	F32 sx = f32_sin(angles_in_radians.x * 0.5f);
	F32 cy = f32_cos(angles_in_radians.y * 0.5f);
	F32 sy = f32_sin(angles_in_radians.y * 0.5f);
	F32 cz = f32_cos(angles_in_radians.z * 0.5f);
	F32 sz = f32_sin(angles_in_radians.z * 0.5f);

	return Quaternion {
		.w = cx * cy * cz + sx * sy * sz,
		.x = sx * cy * cz - cx * sy * sz,
		.y = cx * sy * cz + sx * cy * sz,
		.z = cx * cy * sz - sx * sy * cz
	};
}

inline static Quaternion
quaternion_from_f32x4x4(const F32x4x4 &M)
{
	F32x3 rx = f32x3_normalize(F32x3{.x = M.m00, .y = M.m01, .z = M.m02});
	F32x3 ry = f32x3_normalize(F32x3{.x = M.m10, .y = M.m11, .z = M.m12});
	F32x3 rz = f32x3_normalize(F32x3{.x = M.m20, .y = M.m21, .z = M.m22});
	F32 trace = rx.x + ry.y + rz.z;
	Quaternion q;
	if (trace > 0.0f)
	{
		F32 s = f32_sqrt(trace + 1.0f) * 2.0f;
		q.w = 0.25f * s;
		q.x = (ry.z - rz.y) / s;
		q.y = (rz.x - rx.z) / s;
		q.z = (rx.y - ry.x) / s;
	}
	else if (rx.x > ry.y && rx.x > rz.z)
	{
		F32 s = f32_sqrt(1.0f + rx.x - ry.y - rz.z) * 2.0f;
		q.w = (ry.z - rz.y) / s;
		q.x = 0.25f * s;
		q.y = (ry.x + rx.y) / s;
		q.z = (rz.x + rx.z) / s;
	}
	else if (ry.y > rz.z)
	{
		F32 s = f32_sqrt(1.0f + ry.y - rx.x - rz.z) * 2.0f;
		q.w = (rz.x - rx.z) / s;
		q.x = (ry.x + rx.y) / s;
		q.y = 0.25f * s;
		q.z = (rz.y + ry.z) / s;
	}
	else
	{
		F32 s = f32_sqrt(1.0f + rz.z - rx.x - ry.y) * 2.0f;
		q.w = (rx.y - ry.x) / s;
		q.x = (rz.x + rx.z) / s;
		q.y = (rz.y + ry.z) / s;
		q.z = 0.25f * s;
	}
	F32 length_squared = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
	validate(length_squared != 0.0f, "[MATH][Quaternion]: Cannot normalize zero-length quaternion.");
	return q / f32_sqrt(length_squared);
}

inline static Quaternion
quaternion_identity()
{
	return Quaternion{.w = 1.0f, .x = 0.0f, .y = 0.0f, .z = 0.0f};
}

inline static Quaternion
quaternion_normalize(const Quaternion &q)
{
	F32 length_squared = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
	validate(length_squared != 0.0f, "[MATH][Quaternion]: Cannot normalize zero-length quaternion.");
	return q / f32_sqrt(length_squared);
}

inline static F32
quaternion_length(const Quaternion &q)
{
	return f32_sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
}

inline static F32
quaternion_length_squared(const Quaternion &q)
{
	return q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
}

inline static bool
quaternion_approx_equal(const Quaternion &a, const Quaternion &b, F32 epsilon)
{
	return f32_approx_equal(a.w, b.w, epsilon)
		&& f32_approx_equal(a.x, b.x, epsilon)
		&& f32_approx_equal(a.y, b.y, epsilon)
		&& f32_approx_equal(a.z, b.z, epsilon);
}

inline static F32
quaternion_dot(const Quaternion &p, const Quaternion &q)
{
	return p.w * q.w + p.x * q.x + p.y * q.y + p.z * q.z;
}

inline static bool
quaternion_rotation_approx_equal(const Quaternion &a, const Quaternion &b, F32 epsilon)
{
	Quaternion na = quaternion_normalize(a);
	Quaternion nb = quaternion_normalize(b);
	return f32_approx_equal(f32_abs(quaternion_dot(na, nb)), 1.0f, epsilon);
}

inline static Quaternion
quaternion_conjugate(const Quaternion &q)
{
	return Quaternion{.w = q.w, .x = -q.x, .y = -q.y, .z = -q.z};
}

inline static Quaternion
quaternion_inverse(const Quaternion &q)
{
	F32 length_squared = quaternion_length_squared(q);
	validate(length_squared != 0.0f, "[MATH][Quaternion]: Cannot invert zero-length quaternion.");
	return quaternion_conjugate(q) / length_squared;
}

inline static F32
quaternion_angle(const Quaternion &q)
{
	Quaternion n = quaternion_normalize(q);
	return 2.0f * f32_acos(f32_clamp(n.w, -1.0f, 1.0f));
}

inline static F32
quaternion_angle(const Quaternion &a, const Quaternion &b)
{
	Quaternion na = quaternion_normalize(a);
	Quaternion nb = quaternion_normalize(b);
	return 2.0f * f32_acos(f32_clamp(f32_abs(quaternion_dot(na, nb)), -1.0f, 1.0f));
}

inline static void
quaternion_to_axis_angle(const Quaternion &q, F32x3 *out_axis, F32 *out_angle)
{
	validate(out_axis != nullptr, "[MATH][Quaternion]: out_axis must not be null.");
	validate(out_angle != nullptr, "[MATH][Quaternion]: out_angle must not be null.");
	Quaternion n = quaternion_normalize(q);
	F32 angle = 2.0f * f32_acos(f32_clamp(n.w, -1.0f, 1.0f));
	F32 s = f32_sqrt(f32_max(0.0f, 1.0f - n.w * n.w));
	if (s == 0.0f)
	{
		*out_axis = F32x3{.x = 1.0f, .y = 0.0f, .z = 0.0f};
		*out_angle = 0.0f;
		return;
	}
	*out_axis = F32x3{.x = n.x / s, .y = n.y / s, .z = n.z / s};
	*out_angle = angle;
}

inline static F32x3
quaternion_to_euler_angles(const Quaternion &q)
{
	F32x3 r;
	r.x = f32_atan2(2.0f * (q.w * q.x + q.y * q.z), 1.0f - 2.0f * (q.x * q.x + q.y * q.y));

	F32 siny = 2.0f * (q.w * q.y - q.z * q.x);
	if (f32_abs(siny) >= 1.0f)
		r.y = f32_sign(siny) * F32_PI * 0.5f;
	else
		r.y = f32_asin(siny);

	r.z = f32_atan2(2.0f * (q.w * q.z + q.x * q.y), 1.0f - 2.0f * (q.y * q.y + q.z * q.z));
	return r;
}

inline static Quaternion
quaternion_from_to_rotation(const F32x3 &from, const F32x3 &to)
{
	F32x3 u = f32x3_normalize(from);
	F32x3 v = f32x3_normalize(to);
	F32   d = f32x3_dot(u, v);

	if (d < -0.9999f)
	{
		F32x3 axis = f32x3_cross(F32x3{.x = 1.0f, .y = 0.0f, .z = 0.0f}, u);
		if (f32x3_length_squared(axis) < 1e-6f)
			axis = f32x3_cross(F32x3{.x = 0.0f, .y = 1.0f, .z = 0.0f}, u);
		return quaternion_from_axis_angle(f32x3_normalize(axis), F32_PI);
	}

	F32x3 c = f32x3_cross(u, v);
	F32   s = f32_sqrt((1.0f + d) * 2.0f);
	F32   inv_s = 1.0f / s;
	return Quaternion{.w = s * 0.5f, .x = c.x * inv_s, .y = c.y * inv_s, .z = c.z * inv_s};
}

inline static Quaternion
quaternion_look_rotation(const F32x3 &forward, const F32x3 &up)
{
	validate(f32x3_length_squared(forward) != 0.0f, "[MATH][Quaternion]: look_rotation forward must be non-zero.");
	F32x3 axis_z = f32x3_normalize(-forward);
	F32x3 axis_x_source = f32x3_cross(up, axis_z);
	validate(f32x3_length_squared(axis_x_source) != 0.0f, "[MATH][Quaternion]: look_rotation up must not be parallel to forward.");
	F32x3 axis_x = f32x3_normalize(axis_x_source);
	F32x3 axis_y = f32x3_cross(axis_z, axis_x);

	F32 trace = axis_x.x + axis_y.y + axis_z.z;
	Quaternion q;
	if (trace > 0.0f)
	{
		F32 s = f32_sqrt(trace + 1.0f) * 2.0f;
		q.w = 0.25f * s;
		q.x = (axis_y.z - axis_z.y) / s;
		q.y = (axis_z.x - axis_x.z) / s;
		q.z = (axis_x.y - axis_y.x) / s;
	}
	else if (axis_x.x > axis_y.y && axis_x.x > axis_z.z)
	{
		F32 s = f32_sqrt(1.0f + axis_x.x - axis_y.y - axis_z.z) * 2.0f;
		q.w = (axis_y.z - axis_z.y) / s;
		q.x = 0.25f * s;
		q.y = (axis_y.x + axis_x.y) / s;
		q.z = (axis_z.x + axis_x.z) / s;
	}
	else if (axis_y.y > axis_z.z)
	{
		F32 s = f32_sqrt(1.0f + axis_y.y - axis_x.x - axis_z.z) * 2.0f;
		q.w = (axis_z.x - axis_x.z) / s;
		q.x = (axis_y.x + axis_x.y) / s;
		q.y = 0.25f * s;
		q.z = (axis_z.y + axis_y.z) / s;
	}
	else
	{
		F32 s = f32_sqrt(1.0f + axis_z.z - axis_x.x - axis_y.y) * 2.0f;
		q.w = (axis_x.y - axis_y.x) / s;
		q.x = (axis_z.x + axis_x.z) / s;
		q.y = (axis_z.y + axis_y.z) / s;
		q.z = 0.25f * s;
	}
	return quaternion_normalize(q);
}

inline static Quaternion
quaternion_nlerp(Quaternion a, Quaternion b, F32 t)
{
	if (quaternion_dot(a, b) < 0.0f)
		b = -b;
	return quaternion_normalize(a + (b - a) * t);
}

inline static Quaternion
quaternion_slerp(Quaternion a, Quaternion b, F32 t)
{
	F32 dot = quaternion_dot(a, b);
	validate(dot >= -1.0001f && dot <= 1.0001f, "[MATH][Quaternion]: slerp inputs must be normalized.");

	if (dot < 0.0f)
	{
		b = -b;
		dot = -dot;
	}

	constexpr F32 SLERP_LINEAR_THRESHOLD = 0.9995f;
	if (dot > SLERP_LINEAR_THRESHOLD)
	{
		Quaternion r = Quaternion {
			.w = a.w + t * (b.w - a.w),
			.x = a.x + t * (b.x - a.x),
			.y = a.y + t * (b.y - a.y),
			.z = a.z + t * (b.z - a.z)
		};
		return quaternion_normalize(r);
	}

	F32 theta_0   = f32_acos(dot);
	F32 theta     = theta_0 * t;
	F32 sin_theta = f32_sin(theta);
	F32 sin_0     = f32_sin(theta_0);

	F32 s0 = f32_cos(theta) - dot * sin_theta / sin_0;
	F32 s1 = sin_theta / sin_0;

	return Quaternion {
		.w = s0 * a.w + s1 * b.w,
		.x = s0 * a.x + s1 * b.x,
		.y = s0 * a.y + s1 * b.y,
		.z = s0 * a.z + s1 * b.z
	};
}

inline static Quaternion
quaternion_rotate_towards(const Quaternion &current, const Quaternion &target, F32 max_angle_radians)
{
	validate(max_angle_radians >= 0.0f, "[MATH][Quaternion]: rotate_towards max angle must be non-negative.");
	F32 dot = f32_clamp(quaternion_dot(current, target), -1.0f, 1.0f);
	if (dot < 0.0f)
		dot = -dot;
	F32 angle = 2.0f * f32_acos(dot);
	if (angle <= max_angle_radians || angle == 0.0f)
		return target;
	return quaternion_slerp(current, target, max_angle_radians / angle);
}