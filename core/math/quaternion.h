#pragma once

#include <core/defines.h>
#include <core/math/f32.h>
#include <core/math/f32x3.h>
#include <core/math/f32x4x4.h>

// ============================================================================
// Quaternion — unit-quaternion rotation representation.
//
// Storage order is (w, x, y, z). Unit quaternions satisfy w² + x² + y² + z² = 1
// and represent a rotation of angle 2·acos(w) about axis (x, y, z)/sin(acos(w)).
//
// Composition follows the engine's row-vector convention: rotating a vector by
// two quaternions in sequence is written `v * q1 * q2` on the CPU — `q1`
// applied first, `q2` applied second. Quaternion multiplication is non-commutative.
// ============================================================================

struct Quaternion
{
	F32 w, x, y, z;
};

// ---- Operators -------------------------------------------------------------

inline static Quaternion
operator+(const Quaternion &p, const Quaternion &q)
{
	return Quaternion{p.w + q.w, p.x + q.x, p.y + q.y, p.z + q.z};
}

inline static Quaternion &
operator+=(Quaternion &p, const Quaternion &q) { p = p + q; return p; }

inline static Quaternion
operator-(const Quaternion &q) { return Quaternion{-q.w, -q.x, -q.y, -q.z}; }

inline static Quaternion
operator-(const Quaternion &p, const Quaternion &q)
{
	return Quaternion{p.w - q.w, p.x - q.x, p.y - q.y, p.z - q.z};
}

inline static Quaternion &
operator-=(Quaternion &p, const Quaternion &q) { p = p - q; return p; }

inline static Quaternion
operator*(const Quaternion &q, F32 s)
{
	return Quaternion{q.w * s, q.x * s, q.y * s, q.z * s};
}

inline static Quaternion
operator*(F32 s, const Quaternion &q) { return q * s; }

inline static Quaternion &
operator*=(Quaternion &q, F32 s) { q = q * s; return q; }

inline static Quaternion
operator/(const Quaternion &q, F32 s) { return q * (1.0f / s); }

inline static Quaternion &
operator/=(Quaternion &q, F32 s) { q = q / s; return q; }

// Quaternion product — non-commutative; composition semantics match row-vector
// convention (see header comment).
inline static Quaternion
operator*(const Quaternion &p, const Quaternion &q)
{
	return Quaternion{
		p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z,
		p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y,
		p.w * q.y - p.x * q.z + p.y * q.w + p.z * q.x,
		p.w * q.z + p.x * q.y - p.y * q.x + p.z * q.w
	};
}

inline static Quaternion &
operator*=(Quaternion &p, const Quaternion &q) { p = p * q; return p; }

inline static bool
operator==(const Quaternion &p, const Quaternion &q)
{
	return p.w == q.w && p.x == q.x && p.y == q.y && p.z == q.z;
}

// ---- Rotate a vector by a quaternion (v' = v * q) --------------------------
// Uses the standard formula v' = v + 2 * q.xyz × (q.xyz × v + q.w * v).

inline static F32x3
operator*(const F32x3 &v, const Quaternion &q)
{
	F32x3 u = {q.x, q.y, q.z};
	F32x3 t = 2.0f * f32x3_cross(u, v);
	return v + q.w * t + f32x3_cross(u, t);
}

inline static F32x3 &
operator*=(F32x3 &v, const Quaternion &q) { v = v * q; return v; }

// ---- Construction ----------------------------------------------------------

inline static Quaternion
quaternion_identity() { return Quaternion{1.0f, 0.0f, 0.0f, 0.0f}; }

inline static Quaternion
quaternion_from_axis_angle(const F32x3 &axis, F32 angle_in_radians)
{
	F32 half = angle_in_radians * 0.5f;
	F32 s    = f32_sin(half);
	return Quaternion{f32_cos(half), s * axis.x, s * axis.y, s * axis.z};
}

// Right-handed XYZ-intrinsic Euler → Quaternion. Angles are in radians.
// Composition: rotate about X first, then Y, then Z (roll last). No -z hack.
// Fixes the legacy `quat_from_angles` which was documented as left-handed and
// compensated the z imaginary with a negation (see plan).
inline static Quaternion
quaternion_from_euler_angles(const F32x3 &angles_in_radians)
{
	F32 cx = f32_cos(angles_in_radians.x * 0.5f);
	F32 sx = f32_sin(angles_in_radians.x * 0.5f);
	F32 cy = f32_cos(angles_in_radians.y * 0.5f);
	F32 sy = f32_sin(angles_in_radians.y * 0.5f);
	F32 cz = f32_cos(angles_in_radians.z * 0.5f);
	F32 sz = f32_sin(angles_in_radians.z * 0.5f);

	return Quaternion{
		cx * cy * cz + sx * sy * sz,   // w
		sx * cy * cz - cx * sy * sz,   // x
		cx * sy * cz + sx * cy * sz,   // y
		cx * cy * sz - sx * sy * cz    // z
	};
}

// Extract XYZ-intrinsic Euler angles (radians) from a unit quaternion. Handles
// gimbal-lock at ±90° pitch by clamping asin.
inline static F32x3
quaternion_to_euler_angles(const Quaternion &q)
{
	F32x3 r;
	r.x = f32_atan2(2.0f * (q.w * q.x + q.y * q.z), 1.0f - 2.0f * (q.x * q.x + q.y * q.y));

	F32 siny = 2.0f * (q.w * q.y - q.z * q.x);
	if (f32_abs(siny) >= 1.0f)
		r.y = f32_sign(siny) * F32_PI_OVER_2;
	else
		r.y = f32_asin(siny);

	r.z = f32_atan2(2.0f * (q.w * q.z + q.x * q.y), 1.0f - 2.0f * (q.y * q.y + q.z * q.z));
	return r;
}

// ---- Magnitude / normalization --------------------------------------------

inline static F32
quaternion_dot(const Quaternion &p, const Quaternion &q)
{
	return p.w * q.w + p.x * q.x + p.y * q.y + p.z * q.z;
}

inline static F32
quaternion_length_squared(const Quaternion &q) { return quaternion_dot(q, q); }

inline static F32
quaternion_length(const Quaternion &q) { return f32_sqrt(quaternion_length_squared(q)); }

inline static Quaternion
quaternion_normalize(const Quaternion &q) { return q / quaternion_length(q); }

inline static Quaternion
quaternion_inverse(const Quaternion &q)
{
	// For unit quaternions this is the conjugate; we divide by ||q||² to stay correct for non-unit.
	return Quaternion{q.w, -q.x, -q.y, -q.z} / quaternion_length_squared(q);
}

// ---- SLERP -----------------------------------------------------------------
// Shortest-path spherical linear interpolation. Falls back to nlerp near t=0.

inline static Quaternion
quaternion_slerp(Quaternion a, Quaternion b, F32 t)
{
	F32 dot = quaternion_dot(a, b);

	// If the dot is negative, negate one endpoint to take the shorter arc.
	if (dot < 0.0f)
	{
		b = -b;
		dot = -dot;
	}

	// If the inputs are nearly parallel, linearly interpolate and re-normalize.
	constexpr F32 SLERP_LINEAR_THRESHOLD = 0.9995f;
	if (dot > SLERP_LINEAR_THRESHOLD)
	{
		Quaternion r = {
			a.w + t * (b.w - a.w),
			a.x + t * (b.x - a.x),
			a.y + t * (b.y - a.y),
			a.z + t * (b.z - a.z)
		};
		return quaternion_normalize(r);
	}

	F32 theta_0   = f32_acos(dot);
	F32 theta     = theta_0 * t;
	F32 sin_theta = f32_sin(theta);
	F32 sin_0     = f32_sin(theta_0);

	F32 s0 = f32_cos(theta) - dot * sin_theta / sin_0;
	F32 s1 = sin_theta / sin_0;

	return Quaternion{
		s0 * a.w + s1 * b.w,
		s0 * a.x + s1 * b.x,
		s0 * a.y + s1 * b.y,
		s0 * a.z + s1 * b.z
	};
}

inline static bool
quaternion_approx_equal(const Quaternion &a, const Quaternion &b, F32 epsilon)
{
	return f32_approx_equal(a.w, b.w, epsilon)
	    && f32_approx_equal(a.x, b.x, epsilon)
	    && f32_approx_equal(a.y, b.y, epsilon)
	    && f32_approx_equal(a.z, b.z, epsilon);
}

// ---- Staples (new in v1) ---------------------------------------------------

// Shortest-arc rotation mapping `from` to `to`. Both vectors need not be unit —
// the function normalizes internally.
inline static Quaternion
quaternion_from_to_rotation(const F32x3 &from, const F32x3 &to)
{
	F32x3 u = f32x3_normalize(from);
	F32x3 v = f32x3_normalize(to);
	F32   d = f32x3_dot(u, v);

	// Anti-parallel: pick any axis perpendicular to u.
	if (d < -0.9999f)
	{
		F32x3 axis = f32x3_cross(F32X3_RIGHT, u);
		if (f32x3_length_squared(axis) < 1e-6f)
			axis = f32x3_cross(F32X3_UP, u);
		return quaternion_from_axis_angle(f32x3_normalize(axis), F32_PI);
	}

	F32x3 c = f32x3_cross(u, v);
	F32   s = f32_sqrt((1.0f + d) * 2.0f);
	F32   inv_s = 1.0f / s;
	return Quaternion{s * 0.5f, c.x * inv_s, c.y * inv_s, c.z * inv_s};
}

// Build an orientation whose local -Z points along `forward` and local +Y is
// aligned with `up` (Gram-Schmidt orthogonalization). Matches the camera
// convention from `f32x4x4_look_at` but returns a quaternion for object orientation.
inline static Quaternion
quaternion_look_rotation(const F32x3 &forward, const F32x3 &up)
{
	// Local -Z = forward (canonical convention: camera looks down -Z).
	F32x3 axis_z = f32x3_normalize(-forward);
	F32x3 axis_x = f32x3_normalize(f32x3_cross(up, axis_z));
	F32x3 axis_y = f32x3_cross(axis_z, axis_x);

	// Build quaternion from a basis via the standard rotation-matrix-to-quat
	// algorithm applied to the 3x3 rotation matrix whose columns are (axis_x,
	// axis_y, axis_z).
	F32 trace = axis_x.x + axis_y.y + axis_z.z;
	Quaternion q;
	if (trace > 0.0f)
	{
		F32 s = f32_sqrt(trace + 1.0f) * 2.0f;  // s = 4 * w
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

// Rotate `current` toward `target`, clamped so no single step exceeds
// `max_angle_radians`. Handy for smooth AI aim, camera follow, etc.
inline static Quaternion
quaternion_rotate_towards(const Quaternion &current, const Quaternion &target, F32 max_angle_radians)
{
	F32 dot = f32_clamp(quaternion_dot(current, target), -1.0f, 1.0f);
	if (dot < 0.0f)
		dot = -dot;
	F32 angle = 2.0f * f32_acos(dot);
	if (angle <= max_angle_radians || angle == 0.0f)
		return target;
	return quaternion_slerp(current, target, max_angle_radians / angle);
}

// ---- Constants -------------------------------------------------------------

static constexpr Quaternion QUATERNION_IDENTITY = {1.0f, 0.0f, 0.0f, 0.0f};

// ============================================================================
// F32x4x4 ↔ Quaternion conversions. Lives here to avoid circular include
// between f32x4x4.h and quaternion.h.
// ============================================================================

inline static F32x4x4
f32x4x4_from_quaternion(const Quaternion &q_in)
{
	Quaternion q = quaternion_normalize(q_in);
	F32 w = q.w, x = q.x, y = q.y, z = q.z;

	return F32x4x4{
		1.0f - 2.0f * y * y - 2.0f * z * z,   2.0f * x * y + 2.0f * z * w,         2.0f * x * z - 2.0f * y * w,         0.0f,
		2.0f * x * y - 2.0f * z * w,          1.0f - 2.0f * x * x - 2.0f * z * z,  2.0f * y * z + 2.0f * x * w,         0.0f,
		2.0f * x * z + 2.0f * y * w,          2.0f * y * z - 2.0f * x * w,         1.0f - 2.0f * x * x - 2.0f * y * y,  0.0f,
		0.0f,                                 0.0f,                                 0.0f,                                 1.0f
	};
}

// Decompose a TRS (translation × rotation × scale) matrix built via the
// row-vector convention into its components. Returns false on degenerate input.
inline static bool
f32x4x4_decompose(const F32x4x4 &M, F32x3 *out_translation, Quaternion *out_rotation, F32x3 *out_scale)
{
	if (out_translation)
		*out_translation = F32x3{M.m30, M.m31, M.m32};

	F32x3 row0 = {M.m00, M.m01, M.m02};
	F32x3 row1 = {M.m10, M.m11, M.m12};
	F32x3 row2 = {M.m20, M.m21, M.m22};

	F32 sx = f32x3_length(row0);
	F32 sy = f32x3_length(row1);
	F32 sz = f32x3_length(row2);

	if (sx < 1e-6f || sy < 1e-6f || sz < 1e-6f)
		return false;

	if (out_scale)
		*out_scale = F32x3{sx, sy, sz};

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
		*out_rotation = quaternion_normalize(*out_rotation);
	}
	return true;
}
