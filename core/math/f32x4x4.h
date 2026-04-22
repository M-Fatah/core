#pragma once

#include <core/defines.h>
#include <core/math/f32.h>
#include <core/math/f32x3.h>
#include <core/math/f32x4.h>

// ============================================================================
// F32x4x4 — 4x4 F32 matrix, row-major, SIMD-backed.
//
// Memory layout: [m00 m01 m02 m03 | m10 m11 m12 m13 | m20 m21 m22 m23 | m30 m31 m32 m33]
// Each row is a SIMD-backed F32x4, aligned to 16 bytes. Total: 64 bytes.
//
// Multiplication convention: row-vector. `v * M` transforms the point `v`.
// Translation lives in the last row (m30, m31, m32). See docs/math.md.
// ============================================================================

struct alignas(16) F32x4x4
{
	union
	{
		struct
		{
			F32 m00, m01, m02, m03;
			F32 m10, m11, m12, m13;
			F32 m20, m21, m22, m23;
			F32 m30, m31, m32, m33;
		};
		F32x4 rows[4];
	};
};

// ---- Indexing --------------------------------------------------------------

inline static const F32 &
f32x4x4_at(const F32x4x4 &M, I32 i)
{
	return *((const F32 *)&M + i);
}

inline static F32 &
f32x4x4_at(F32x4x4 &M, I32 i)
{
	return *((F32 *)&M + i);
}

// ---- Identity --------------------------------------------------------------

inline static F32x4x4
f32x4x4_identity()
{
	return F32x4x4{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
}

// ---- Element-wise operators ------------------------------------------------

inline static F32x4x4
operator+(const F32x4x4 &A, const F32x4x4 &B)
{
	F32x4x4 R;
	R.rows[0] = A.rows[0] + B.rows[0];
	R.rows[1] = A.rows[1] + B.rows[1];
	R.rows[2] = A.rows[2] + B.rows[2];
	R.rows[3] = A.rows[3] + B.rows[3];
	return R;
}

inline static F32x4x4 &
operator+=(F32x4x4 &A, const F32x4x4 &B) { A = A + B; return A; }

inline static F32x4x4
operator-(const F32x4x4 &M)
{
	F32x4x4 R;
	R.rows[0] = -M.rows[0];
	R.rows[1] = -M.rows[1];
	R.rows[2] = -M.rows[2];
	R.rows[3] = -M.rows[3];
	return R;
}

inline static F32x4x4
operator-(const F32x4x4 &A, const F32x4x4 &B)
{
	F32x4x4 R;
	R.rows[0] = A.rows[0] - B.rows[0];
	R.rows[1] = A.rows[1] - B.rows[1];
	R.rows[2] = A.rows[2] - B.rows[2];
	R.rows[3] = A.rows[3] - B.rows[3];
	return R;
}

inline static F32x4x4 &
operator-=(F32x4x4 &A, const F32x4x4 &B) { A = A - B; return A; }

inline static F32x4x4
operator*(const F32x4x4 &M, F32 s)
{
	F32x4x4 R;
	R.rows[0] = M.rows[0] * s;
	R.rows[1] = M.rows[1] * s;
	R.rows[2] = M.rows[2] * s;
	R.rows[3] = M.rows[3] * s;
	return R;
}

inline static F32x4x4
operator*(F32 s, const F32x4x4 &M) { return M * s; }

inline static F32x4x4 &
operator*=(F32x4x4 &M, F32 s) { M = M * s; return M; }

inline static F32x4x4
operator/(const F32x4x4 &M, F32 s) { return M * (1.0f / s); }

inline static F32x4x4 &
operator/=(F32x4x4 &M, F32 s) { M = M / s; return M; }

inline static bool
operator==(const F32x4x4 &A, const F32x4x4 &B)
{
	return A.rows[0] == B.rows[0]
	    && A.rows[1] == B.rows[1]
	    && A.rows[2] == B.rows[2]
	    && A.rows[3] == B.rows[3];
}

// ---- Vec-mat multiply (row-vector convention) ------------------------------
// v * M = v.x*row0 + v.y*row1 + v.z*row2 + v.w*row3

inline static F32x4
operator*(const F32x4 &v, const F32x4x4 &M)
{
	return v.x * M.rows[0] + v.y * M.rows[1] + v.z * M.rows[2] + v.w * M.rows[3];
}

inline static F32x4 &
operator*=(F32x4 &v, const F32x4x4 &M) { v = v * M; return v; }

// ---- Mat-mat multiply ------------------------------------------------------
// Row i of (A * B) = A.rows[i] * B

inline static F32x4x4
operator*(const F32x4x4 &A, const F32x4x4 &B)
{
	F32x4x4 R;
	R.rows[0] = A.rows[0] * B;
	R.rows[1] = A.rows[1] * B;
	R.rows[2] = A.rows[2] * B;
	R.rows[3] = A.rows[3] * B;
	return R;
}

inline static F32x4x4 &
operator*=(F32x4x4 &A, const F32x4x4 &B) { A = A * B; return A; }

// ---- Transpose / determinant / inverse -------------------------------------

inline static F32x4x4
f32x4x4_transpose(const F32x4x4 &M)
{
	return F32x4x4{
		M.m00, M.m10, M.m20, M.m30,
		M.m01, M.m11, M.m21, M.m31,
		M.m02, M.m12, M.m22, M.m32,
		M.m03, M.m13, M.m23, M.m33
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
	if (d == 0.0f)
		return F32x4x4{};

	// Matrix of cofactors, transposed (adjoint), divided by determinant.
	F32x4x4 adj = F32x4x4{
		// m00
		+ M.m11 * (M.m22 * M.m33 - M.m23 * M.m32)
		- M.m12 * (M.m21 * M.m33 - M.m23 * M.m31)
		+ M.m13 * (M.m21 * M.m32 - M.m22 * M.m31),
		// m10
		- M.m01 * (M.m22 * M.m33 - M.m23 * M.m32)
		+ M.m02 * (M.m21 * M.m33 - M.m23 * M.m31)
		- M.m03 * (M.m21 * M.m32 - M.m22 * M.m31),
		// m20
		+ M.m01 * (M.m12 * M.m33 - M.m13 * M.m32)
		- M.m02 * (M.m11 * M.m33 - M.m13 * M.m31)
		+ M.m03 * (M.m11 * M.m32 - M.m12 * M.m31),
		// m30
		- M.m01 * (M.m12 * M.m23 - M.m13 * M.m22)
		+ M.m02 * (M.m11 * M.m23 - M.m13 * M.m21)
		- M.m03 * (M.m11 * M.m22 - M.m12 * M.m21),

		// m01
		- M.m10 * (M.m22 * M.m33 - M.m23 * M.m32)
		+ M.m12 * (M.m20 * M.m33 - M.m23 * M.m30)
		- M.m13 * (M.m20 * M.m32 - M.m22 * M.m30),
		// m11
		+ M.m00 * (M.m22 * M.m33 - M.m23 * M.m32)
		- M.m02 * (M.m20 * M.m33 - M.m23 * M.m30)
		+ M.m03 * (M.m20 * M.m32 - M.m22 * M.m30),
		// m21
		- M.m00 * (M.m12 * M.m33 - M.m13 * M.m32)
		+ M.m02 * (M.m10 * M.m33 - M.m13 * M.m30)
		- M.m03 * (M.m10 * M.m32 - M.m12 * M.m30),
		// m31
		+ M.m00 * (M.m12 * M.m23 - M.m13 * M.m22)
		- M.m02 * (M.m10 * M.m23 - M.m13 * M.m20)
		+ M.m03 * (M.m10 * M.m22 - M.m12 * M.m20),

		// m02
		+ M.m10 * (M.m21 * M.m33 - M.m23 * M.m31)
		- M.m11 * (M.m20 * M.m33 - M.m23 * M.m30)
		+ M.m13 * (M.m20 * M.m31 - M.m21 * M.m30),
		// m12
		- M.m00 * (M.m21 * M.m33 - M.m23 * M.m31)
		+ M.m01 * (M.m20 * M.m33 - M.m23 * M.m30)
		- M.m03 * (M.m20 * M.m31 - M.m21 * M.m30),
		// m22
		+ M.m00 * (M.m11 * M.m33 - M.m13 * M.m31)
		- M.m01 * (M.m10 * M.m33 - M.m13 * M.m30)
		+ M.m03 * (M.m10 * M.m31 - M.m11 * M.m30),
		// m32
		- M.m00 * (M.m11 * M.m23 - M.m13 * M.m21)
		+ M.m01 * (M.m10 * M.m23 - M.m13 * M.m20)
		- M.m03 * (M.m10 * M.m21 - M.m11 * M.m20),

		// m03
		- M.m10 * (M.m21 * M.m32 - M.m22 * M.m31)
		+ M.m11 * (M.m20 * M.m32 - M.m22 * M.m30)
		- M.m12 * (M.m20 * M.m31 - M.m21 * M.m30),
		// m13
		+ M.m00 * (M.m21 * M.m32 - M.m22 * M.m31)
		- M.m01 * (M.m20 * M.m32 - M.m22 * M.m30)
		+ M.m02 * (M.m20 * M.m31 - M.m21 * M.m30),
		// m23
		- M.m00 * (M.m11 * M.m32 - M.m12 * M.m31)
		+ M.m01 * (M.m10 * M.m32 - M.m12 * M.m30)
		- M.m02 * (M.m10 * M.m31 - M.m11 * M.m30),
		// m33
		+ M.m00 * (M.m11 * M.m22 - M.m12 * M.m21)
		- M.m01 * (M.m10 * M.m22 - M.m12 * M.m20)
		+ M.m02 * (M.m10 * M.m21 - M.m11 * M.m20)
	};

	return adj * (1.0f / d);
}

// ---- Basis axes (extract rotation axes from a transform) -------------------

inline static F32x3
f32x4x4_axis_x(const F32x4x4 &M) { return f32x3_normalize(F32x3{M.m00, M.m01, M.m02}); }

inline static F32x3
f32x4x4_axis_y(const F32x4x4 &M) { return f32x3_normalize(F32x3{M.m10, M.m11, M.m12}); }

inline static F32x3
f32x4x4_axis_z(const F32x4x4 &M) { return f32x3_normalize(F32x3{M.m20, M.m21, M.m22}); }

// ---- TRS builders ----------------------------------------------------------

inline static F32x4x4
f32x4x4_translation(F32 dx, F32 dy, F32 dz)
{
	return F32x4x4{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		dx,   dy,   dz,   1.0f
	};
}

inline static F32x4x4
f32x4x4_translation(const F32x3 &t) { return f32x4x4_translation(t.x, t.y, t.z); }

inline static F32x4x4
f32x4x4_rotation_x(F32 angle_in_radians)
{
	F32 c = f32_cos(angle_in_radians);
	F32 s = f32_sin(angle_in_radians);
	return F32x4x4{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f,    c,    s, 0.0f,
		0.0f,   -s,    c, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
}

inline static F32x4x4
f32x4x4_rotation_y(F32 angle_in_radians)
{
	F32 c = f32_cos(angle_in_radians);
	F32 s = f32_sin(angle_in_radians);
	return F32x4x4{
		   c, 0.0f,   -s, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		   s, 0.0f,    c, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
}

inline static F32x4x4
f32x4x4_rotation_z(F32 angle_in_radians)
{
	F32 c = f32_cos(angle_in_radians);
	F32 s = f32_sin(angle_in_radians);
	return F32x4x4{
		    c,    s, 0.0f, 0.0f,
		   -s,    c, 0.0f, 0.0f,
		 0.0f, 0.0f, 1.0f, 0.0f,
		 0.0f, 0.0f, 0.0f, 1.0f
	};
}

inline static F32x4x4
f32x4x4_scaling(F32 sx, F32 sy, F32 sz)
{
	return F32x4x4{
		sx,   0.0f, 0.0f, 0.0f,
		0.0f, sy,   0.0f, 0.0f,
		0.0f, 0.0f, sz,   0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
}

inline static F32x4x4
f32x4x4_scaling(const F32x3 &s) { return f32x4x4_scaling(s.x, s.y, s.z); }

// ---- Projection / view builders --------------------------------------------
// Canonical convention: right-handed, Y-up world, Y-up NDC, Z in [0, 1].

// Projection helpers emit Y-up clip space (Metal / D3D12 / OpenGL native).
// For Vulkan (Y-down native NDC), the RHI backend handles the mismatch via a
// negative-height viewport (`VK_KHR_maintenance1`) so shaders stay backend-
// agnostic. See notes in engine/src/rhi/rhi_vulkan.cpp.

inline static F32x4x4
f32x4x4_orthographic(F32 left, F32 right, F32 bottom, F32 top, F32 znear, F32 zfar)
{
	F32x4x4 M = {};
	M.m00 =  2.0f / (right - left);
	M.m30 = -(right + left) / (right - left);

	M.m11 =  2.0f / (top - bottom);
	M.m31 = -(top + bottom) / (top - bottom);

	M.m22 = -1.0f / (zfar - znear);
	M.m32 = -znear / (zfar - znear);

	M.m33 = 1.0f;
	return M;
}

inline static F32x4x4
f32x4x4_perspective(F32 fovy_radians, F32 aspect, F32 znear, F32 zfar)
{
	F32x4x4 M = {};
	F32 h = f32_tan(fovy_radians * 0.5f);
	F32 w = aspect * h;

	M.m00 = 1.0f / w;
	M.m11 = 1.0f / h;
	M.m22 = -zfar / (zfar - znear);
	M.m23 = -1.0f;
	M.m32 = -(zfar * znear) / (zfar - znear);
	return M;
}

inline static F32x4x4
f32x4x4_look_at(const F32x3 &eye, const F32x3 &target, const F32x3 &up)
{
	F32x3 axis_z = f32x3_normalize(eye - target);
	F32x3 axis_x = f32x3_normalize(f32x3_cross(up, axis_z));
	F32x3 axis_y = f32x3_cross(axis_z, axis_x);

	F32x3 t = {
		-f32x3_dot(eye, axis_x),
		-f32x3_dot(eye, axis_y),
		-f32x3_dot(eye, axis_z)
	};

	return F32x4x4{
		axis_x.x, axis_y.x, axis_z.x, 0.0f,
		axis_x.y, axis_y.y, axis_z.y, 0.0f,
		axis_x.z, axis_y.z, axis_z.z, 0.0f,
		t.x,      t.y,      t.z,      1.0f
	};
}

// ---- Screen / view-space helpers -------------------------------------------
// `viewport` is packed as F32x4{x, y, width, height}. NDC convention is Y-up,
// Z in [0,1] — screen space returned by project is pixel-space with Y matching
// the viewport's orientation.

inline static F32x3
f32x3_project(const F32x3 &world, const F32x4x4 &view_projection, const F32x4 &viewport)
{
	F32x4 clip = F32x4{world.x, world.y, world.z, 1.0f} * view_projection;
	if (clip.w == 0.0f)
		return F32x3{0.0f, 0.0f, 0.0f};

	F32x3 ndc = {clip.x / clip.w, clip.y / clip.w, clip.z / clip.w};

	return F32x3{
		viewport.x + (ndc.x * 0.5f + 0.5f) * viewport.z,
		viewport.y + (ndc.y * 0.5f + 0.5f) * viewport.w,
		ndc.z
	};
}

inline static F32x3
f32x3_unproject(const F32x3 &screen, const F32x4x4 &view_projection_inverse, const F32x4 &viewport)
{
	F32x4 ndc = F32x4{
		((screen.x - viewport.x) / viewport.z) * 2.0f - 1.0f,
		((screen.y - viewport.y) / viewport.w) * 2.0f - 1.0f,
		screen.z,
		1.0f
	};
	F32x4 world = ndc * view_projection_inverse;
	if (world.w == 0.0f)
		return F32x3{0.0f, 0.0f, 0.0f};
	return F32x3{world.x / world.w, world.y / world.w, world.z / world.w};
}

// ---- Approx equality -------------------------------------------------------

inline static bool
f32x4x4_approx_equal(const F32x4x4 &A, const F32x4x4 &B, F32 epsilon)
{
	return f32x4_approx_equal(A.rows[0], B.rows[0], epsilon)
	    && f32x4_approx_equal(A.rows[1], B.rows[1], epsilon)
	    && f32x4_approx_equal(A.rows[2], B.rows[2], epsilon)
	    && f32x4_approx_equal(A.rows[3], B.rows[3], epsilon);
}
