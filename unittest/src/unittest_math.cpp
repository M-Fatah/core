#include <core/tester.h>
#include <core/math/f32x4x4.h>
#include <core/math/f32x3x3.h>
#include <core/math/f32x2x2.h>
#include <core/math/f64x4x4.h>
#include <core/math/f64x3x3.h>
#include <core/math/f64x2x2.h>
#include <core/math/f32x4.h>
#include <core/math/f32x3.h>
#include <core/math/f32x2.h>
#include <core/math/f64x4.h>
#include <core/math/f64x3.h>
#include <core/math/f64x2.h>
#include <core/math/i32x4.h>
#include <core/math/i32x3.h>
#include <core/math/i32x2.h>
#include <core/math/u32x4.h>
#include <core/math/u32x3.h>
#include <core/math/u32x2.h>
#include <core/math/quaternion.h>
#include <core/math/random.h>
#include <core/math/f32.h>
#include <core/math/f64.h>
#include <core/math/i32.h>
#include <core/math/u32.h>
#include <core/math/i64.h>
#include <core/math/u64.h>

// ============================================================================
// Scalar helpers — F32
// ============================================================================

TESTER_TEST("[MATH][f32]: constants")
{
	TESTER_CHECK(f32_approx_equal(F32_PI * 2.0f, F32_TAU, F32_EPSILON));
	TESTER_CHECK(f32_approx_equal(F32_PI * 0.5f, F32_PI_OVER_2, F32_EPSILON));
	TESTER_CHECK(f32_approx_equal(F32_PI * F32_TO_DEGREES, 180.0f, 1e-4f));
	TESTER_CHECK(f32_approx_equal(180.0f * F32_TO_RADIANS, F32_PI, 1e-4f));
}

TESTER_TEST("[MATH][f32]: basic arithmetic")
{
	TESTER_CHECK(f32_abs(-3.5f) == 3.5f);
	TESTER_CHECK(f32_abs( 3.5f) == 3.5f);

	TESTER_CHECK(f32_sign(-2.0f) == -1.0f);
	TESTER_CHECK(f32_sign( 0.0f) ==  0.0f);
	TESTER_CHECK(f32_sign( 2.0f) ==  1.0f);

	TESTER_CHECK(f32_min(2.0f, 3.0f) == 2.0f);
	TESTER_CHECK(f32_max(2.0f, 3.0f) == 3.0f);

	TESTER_CHECK(f32_clamp(-5.0f, 0.0f, 10.0f) ==  0.0f);
	TESTER_CHECK(f32_clamp( 5.0f, 0.0f, 10.0f) ==  5.0f);
	TESTER_CHECK(f32_clamp(15.0f, 0.0f, 10.0f) == 10.0f);

	TESTER_CHECK(f32_lerp(0.0f, 10.0f, 0.25f) == 2.5f);
	TESTER_CHECK(f32_lerp(0.0f, 10.0f, 0.0f)  == 0.0f);
	TESTER_CHECK(f32_lerp(0.0f, 10.0f, 1.0f)  == 10.0f);
}

TESTER_TEST("[MATH][f32]: trig + power")
{
	TESTER_CHECK(f32_approx_equal(f32_sqrt(4.0f), 2.0f, 1e-6f));
	TESTER_CHECK(f32_approx_equal(f32_sin(0.0f), 0.0f, 1e-6f));
	TESTER_CHECK(f32_approx_equal(f32_cos(0.0f), 1.0f, 1e-6f));
	TESTER_CHECK(f32_approx_equal(f32_sin(F32_PI_OVER_2), 1.0f, 1e-6f));
	TESTER_CHECK(f32_approx_equal(f32_cos(F32_PI_OVER_2), 0.0f, 1e-6f));
	TESTER_CHECK(f32_approx_equal(f32_power(2.0f, 10.0f), 1024.0f, 1e-3f));
	TESTER_CHECK(f32_approx_equal(f32_modulo(10.5f, 3.0f), 1.5f, 1e-6f));
	TESTER_CHECK(f32_approx_equal(f32_atan2(1.0f, 0.0f), F32_PI_OVER_2, 1e-6f));
}

TESTER_TEST("[MATH][f32]: special values")
{
	TESTER_CHECK(f32_is_nan(F32_NAN));
	TESTER_CHECK(!f32_is_nan(0.0f));
	TESTER_CHECK(!f32_is_nan(F32_INFINITY));

	TESTER_CHECK(f32_is_infinite(F32_INFINITY));
	TESTER_CHECK(f32_is_infinite(F32_NEG_INFINITY));
	TESTER_CHECK(!f32_is_infinite(1.0f));
	TESTER_CHECK(!f32_is_infinite(F32_NAN));

	TESTER_CHECK(f32_is_finite(0.0f));
	TESTER_CHECK(f32_is_finite(-1e30f));
	TESTER_CHECK(!f32_is_finite(F32_INFINITY));
	TESTER_CHECK(!f32_is_finite(F32_NAN));
}

TESTER_TEST("[MATH][f32]: smoothstep / smootherstep")
{
	TESTER_CHECK(f32_smoothstep(0.0f, 1.0f, -0.5f) == 0.0f);
	TESTER_CHECK(f32_smoothstep(0.0f, 1.0f,  1.5f) == 1.0f);
	TESTER_CHECK(f32_approx_equal(f32_smoothstep(0.0f, 1.0f, 0.5f), 0.5f, 1e-6f));

	TESTER_CHECK(f32_smootherstep(0.0f, 1.0f, -0.5f) == 0.0f);
	TESTER_CHECK(f32_smootherstep(0.0f, 1.0f,  1.5f) == 1.0f);
	TESTER_CHECK(f32_approx_equal(f32_smootherstep(0.0f, 1.0f, 0.5f), 0.5f, 1e-6f));
}

TESTER_TEST("[MATH][f32]: easing endpoints")
{
	// Every easing function maps 0 -> 0 and 1 -> 1.
	TESTER_CHECK(f32_ease_in_quad(0.0f)      == 0.0f);
	TESTER_CHECK(f32_ease_in_quad(1.0f)      == 1.0f);
	TESTER_CHECK(f32_ease_out_quad(0.0f)     == 0.0f);
	TESTER_CHECK(f32_ease_out_quad(1.0f)     == 1.0f);
	TESTER_CHECK(f32_ease_in_out_quad(0.0f)  == 0.0f);
	TESTER_CHECK(f32_ease_in_out_quad(1.0f)  == 1.0f);

	TESTER_CHECK(f32_ease_in_cubic(0.0f)     == 0.0f);
	TESTER_CHECK(f32_ease_in_cubic(1.0f)     == 1.0f);
	TESTER_CHECK(f32_ease_out_cubic(0.0f)    == 0.0f);
	TESTER_CHECK(f32_ease_out_cubic(1.0f)    == 1.0f);
	TESTER_CHECK(f32_ease_in_out_cubic(0.0f) == 0.0f);
	TESTER_CHECK(f32_ease_in_out_cubic(1.0f) == 1.0f);

	TESTER_CHECK(f32_ease_in_elastic(0.0f)     == 0.0f);
	TESTER_CHECK(f32_ease_in_elastic(1.0f)     == 1.0f);
	TESTER_CHECK(f32_ease_out_elastic(0.0f)    == 0.0f);
	TESTER_CHECK(f32_ease_out_elastic(1.0f)    == 1.0f);
	TESTER_CHECK(f32_ease_in_out_elastic(0.0f) == 0.0f);
	TESTER_CHECK(f32_ease_in_out_elastic(1.0f) == 1.0f);
}

TESTER_TEST("[MATH][f32]: smooth_damp converges")
{
	F32 current  = 0.0f;
	F32 velocity = 0.0f;
	F32 target   = 10.0f;
	for (int i = 0; i < 300; ++i)
		current = f32_smooth_damp(current, target, &velocity, 0.1f, 0.016f);
	TESTER_CHECK(f32_approx_equal(current, target, 0.1f));
}

// ============================================================================
// Scalar helpers — F64
// ============================================================================

TESTER_TEST("[MATH][f64]: constants + arithmetic")
{
	TESTER_CHECK(f64_approx_equal(F64_PI * 2.0, F64_TAU, F64_EPSILON));
	TESTER_CHECK(f64_abs(-3.5) == 3.5);
	TESTER_CHECK(f64_sign(-2.0) == -1.0);
	TESTER_CHECK(f64_min(2.0, 3.0) == 2.0);
	TESTER_CHECK(f64_max(2.0, 3.0) == 3.0);
	TESTER_CHECK(f64_clamp(15.0, 0.0, 10.0) == 10.0);
	TESTER_CHECK(f64_lerp(0.0, 10.0, 0.5) == 5.0);
	TESTER_CHECK(f64_approx_equal(f64_sqrt(9.0), 3.0, 1e-9));
	TESTER_CHECK(f64_approx_equal(f64_cos(0.0), 1.0, 1e-12));
}

TESTER_TEST("[MATH][f64]: special values")
{
	TESTER_CHECK(f64_is_nan(F64_NAN));
	TESTER_CHECK(f64_is_infinite(F64_INFINITY));
	TESTER_CHECK(f64_is_finite(0.0));
	TESTER_CHECK(!f64_is_finite(F64_NAN));
}

// ============================================================================
// Scalar helpers — I32 / I64 / U32 / U64
// ============================================================================

TESTER_TEST("[MATH][i32]: basic ops")
{
	TESTER_CHECK(i32_abs(-5) == 5);
	TESTER_CHECK(i32_abs( 5) == 5);
	TESTER_CHECK(i32_sign(-2) == -1);
	TESTER_CHECK(i32_sign( 0) ==  0);
	TESTER_CHECK(i32_sign( 2) ==  1);
	TESTER_CHECK(i32_min(2, 3) == 2);
	TESTER_CHECK(i32_max(2, 3) == 3);
	TESTER_CHECK(i32_clamp(-5, 0, 10) == 0);
	TESTER_CHECK(i32_clamp(15, 0, 10) == 10);
}

TESTER_TEST("[MATH][i64]: basic ops")
{
	TESTER_CHECK(i64_abs(-5ll) == 5ll);
	TESTER_CHECK(i64_sign(-2ll) == -1ll);
	TESTER_CHECK(i64_min(2ll, 3ll) == 2ll);
	TESTER_CHECK(i64_max(2ll, 3ll) == 3ll);
	TESTER_CHECK(i64_clamp(15ll, 0ll, 10ll) == 10ll);
}

TESTER_TEST("[MATH][u32]: basic ops")
{
	TESTER_CHECK(u32_min(2u, 3u) == 2u);
	TESTER_CHECK(u32_max(2u, 3u) == 3u);
	TESTER_CHECK(u32_clamp(5u, 0u, 10u) ==  5u);
	TESTER_CHECK(u32_clamp(15u, 0u, 10u) == 10u);
}

TESTER_TEST("[MATH][u64]: basic ops")
{
	TESTER_CHECK(u64_min(2ull, 3ull) == 2ull);
	TESTER_CHECK(u64_max(2ull, 3ull) == 3ull);
	TESTER_CHECK(u64_clamp(15ull, 0ull, 10ull) == 10ull);
}

// ============================================================================
// Vectors — F32x2
// ============================================================================

TESTER_TEST("[MATH][F32x2]: operators")
{
	F32x2 a = {3.0f, 4.0f};
	F32x2 b = {1.0f, 2.0f};

	TESTER_CHECK(a + b == F32x2{4.0f, 6.0f});
	TESTER_CHECK(a - b == F32x2{2.0f, 2.0f});
	TESTER_CHECK(-a   == F32x2{-3.0f, -4.0f});
	TESTER_CHECK(a * 2.0f == F32x2{6.0f, 8.0f});
	TESTER_CHECK(2.0f * a == F32x2{6.0f, 8.0f});
	TESTER_CHECK(a / 2.0f == F32x2{1.5f, 2.0f});

	F32x2 c = a; c += b;
	TESTER_CHECK(c == F32x2{4.0f, 6.0f});
}

TESTER_TEST("[MATH][F32x2]: dot / cross / length")
{
	F32x2 a = {3.0f, 4.0f};
	TESTER_CHECK(f32x2_length_squared(a) == 25.0f);
	TESTER_CHECK(f32_approx_equal(f32x2_length(a), 5.0f, 1e-6f));
	TESTER_CHECK(f32_approx_equal(f32x2_length(f32x2_normalize(a)), 1.0f, 1e-6f));

	F32x2 b = {1.0f, 2.0f};
	TESTER_CHECK(f32x2_dot(a, b) == 11.0f);

	// 2D cross (scalar z-component of 3D cross).
	TESTER_CHECK(f32x2_cross(F32x2{1.0f, 0.0f}, F32x2{0.0f, 1.0f}) ==  1.0f);
	TESTER_CHECK(f32x2_cross(F32x2{0.0f, 1.0f}, F32x2{1.0f, 0.0f}) == -1.0f);
}

TESTER_TEST("[MATH][F32x2]: min/max/lerp/approx_equal")
{
	F32x2 a = {3.0f, -1.0f};
	F32x2 b = {1.0f,  5.0f};
	TESTER_CHECK(f32x2_min(a, b) == F32x2{1.0f, -1.0f});
	TESTER_CHECK(f32x2_max(a, b) == F32x2{3.0f,  5.0f});
	TESTER_CHECK(f32x2_lerp(F32x2{0.0f, 0.0f}, F32x2{10.0f, 20.0f}, 0.5f) == F32x2{5.0f, 10.0f});
	TESTER_CHECK(f32x2_approx_equal(F32x2{1.0000001f, 2.0f}, F32x2{1.0f, 2.0f}, 1e-5f));
}

// ============================================================================
// Vectors — F32x3 (+ canonical axis constants)
// ============================================================================

TESTER_TEST("[MATH][F32x3]: operators + dot + cross")
{
	F32x3 a = {1.0f, 2.0f, 3.0f};
	F32x3 b = {4.0f, 5.0f, 6.0f};

	TESTER_CHECK(a + b == F32x3{5.0f, 7.0f, 9.0f});
	TESTER_CHECK(a - b == F32x3{-3.0f, -3.0f, -3.0f});
	TESTER_CHECK(-a   == F32x3{-1.0f, -2.0f, -3.0f});
	TESTER_CHECK(a * 2.0f == F32x3{2.0f, 4.0f, 6.0f});

	TESTER_CHECK(f32x3_dot(a, b) == 32.0f);
	TESTER_CHECK(f32x3_cross(F32X3_RIGHT, F32X3_UP) == F32X3_BACKWARD);
	TESTER_CHECK(f32x3_cross(F32X3_UP, F32X3_RIGHT) == F32X3_FORWARD);
}

TESTER_TEST("[MATH][F32x3]: length/normalize/min/max/lerp")
{
	F32x3 v = {2.0f, 3.0f, 6.0f};
	TESTER_CHECK(f32x3_length_squared(v) == 49.0f);
	TESTER_CHECK(f32_approx_equal(f32x3_length(v), 7.0f, 1e-6f));
	TESTER_CHECK(f32_approx_equal(f32x3_length(f32x3_normalize(v)), 1.0f, 1e-6f));

	F32x3 a = {3.0f, -1.0f, 10.0f};
	F32x3 b = {1.0f,  5.0f, -2.0f};
	TESTER_CHECK(f32x3_min(a, b) == F32x3{1.0f, -1.0f, -2.0f});
	TESTER_CHECK(f32x3_max(a, b) == F32x3{3.0f,  5.0f, 10.0f});
	TESTER_CHECK(f32x3_lerp(F32X3_ZERO, F32x3{10.0f, 20.0f, 30.0f}, 0.5f) == F32x3{5.0f, 10.0f, 15.0f});

	F32x3 clamp_v  = {-2.0f, 5.0f, 7.0f};
	F32x3 clamp_lo = { 0.0f, 0.0f, 0.0f};
	F32x3 clamp_hi = { 3.0f, 3.0f, 6.0f};
	TESTER_CHECK(f32x3_clamp(clamp_v, clamp_lo, clamp_hi) == F32x3{0.0f, 3.0f, 6.0f});
}

TESTER_TEST("[MATH][F32x3]: canonical axis constants")
{
	TESTER_CHECK(F32X3_RIGHT    == F32x3{ 1.0f,  0.0f,  0.0f});
	TESTER_CHECK(F32X3_UP       == F32x3{ 0.0f,  1.0f,  0.0f});
	TESTER_CHECK(F32X3_FORWARD  == F32x3{ 0.0f,  0.0f, -1.0f});
	TESTER_CHECK(F32X3_LEFT     == -F32X3_RIGHT);
	TESTER_CHECK(F32X3_DOWN     == -F32X3_UP);
	TESTER_CHECK(F32X3_BACKWARD == -F32X3_FORWARD);
}

// ============================================================================
// Vectors — F32x4 (SIMD-backed)
// ============================================================================

TESTER_TEST("[MATH][F32x4]: alignment + size")
{
	// alignas(16) and exactly 16 bytes — must match std140 / MSL vec4.
	TESTER_CHECK(sizeof(F32x4)  == 16);
	TESTER_CHECK(alignof(F32x4) == 16);
}

TESTER_TEST("[MATH][F32x4]: operators")
{
	F32x4 a = {1.0f, 2.0f, 3.0f, 4.0f};
	F32x4 b = {5.0f, 6.0f, 7.0f, 8.0f};

	TESTER_CHECK(a + b == F32x4{6.0f, 8.0f, 10.0f, 12.0f});
	TESTER_CHECK(b - a == F32x4{4.0f, 4.0f,  4.0f,  4.0f});
	TESTER_CHECK(-a    == F32x4{-1.0f, -2.0f, -3.0f, -4.0f});
	TESTER_CHECK(a * 2.0f == F32x4{2.0f, 4.0f, 6.0f, 8.0f});
	TESTER_CHECK(2.0f * a == F32x4{2.0f, 4.0f, 6.0f, 8.0f});
	TESTER_CHECK(a / 2.0f == F32x4{0.5f, 1.0f, 1.5f, 2.0f});
}

TESTER_TEST("[MATH][F32x4]: dot / length / normalize")
{
	F32x4 a = {1.0f, 2.0f, 3.0f, 4.0f};
	F32x4 b = {5.0f, 6.0f, 7.0f, 8.0f};

	TESTER_CHECK(f32_approx_equal(f32x4_dot(a, b), 70.0f, 1e-5f));

	F32x4 v = {2.0f, 2.0f, 2.0f, 2.0f};  // length = sqrt(16) = 4
	TESTER_CHECK(f32_approx_equal(f32x4_length_squared(v), 16.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(f32x4_length(v), 4.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(f32x4_length(f32x4_normalize(v)), 1.0f, 1e-5f));
}

TESTER_TEST("[MATH][F32x4]: min / max / lerp / approx_equal / from_f32")
{
	F32x4 a = {3.0f, -1.0f, 10.0f, 0.0f};
	F32x4 b = {1.0f,  5.0f, -2.0f, 8.0f};

	TESTER_CHECK(f32x4_min(a, b) == F32x4{1.0f, -1.0f, -2.0f, 0.0f});
	TESTER_CHECK(f32x4_max(a, b) == F32x4{3.0f,  5.0f, 10.0f, 8.0f});

	TESTER_CHECK(f32x4_lerp(F32X4_ZERO, F32X4_ONE, 0.25f) == F32x4{0.25f, 0.25f, 0.25f, 0.25f});
	TESTER_CHECK(f32x4_approx_equal(F32x4{1.0f, 2.0f, 3.0f, 4.0f}, F32x4{1.0f, 2.0f, 3.0f, 4.0f}, 1e-6f));
	TESTER_CHECK(f32x4_from_f32(3.5f) == F32x4{3.5f, 3.5f, 3.5f, 3.5f});
}

// ============================================================================
// Vectors — F64x2 / F64x3 / F64x4
// ============================================================================

TESTER_TEST("[MATH][F64x2]: ops + dot + length")
{
	F64x2 a = {3.0, 4.0};
	TESTER_CHECK(a + F64x2{1.0, 1.0} == F64x2{4.0, 5.0});
	TESTER_CHECK(-a == F64x2{-3.0, -4.0});
	TESTER_CHECK(a * 2.0 == F64x2{6.0, 8.0});
	TESTER_CHECK(f64x2_dot(a, F64x2{1.0, 2.0}) == 11.0);
	TESTER_CHECK(f64_approx_equal(f64x2_length(a), 5.0, 1e-12));
	TESTER_CHECK(f64x2_cross(F64x2{1.0, 0.0}, F64x2{0.0, 1.0}) == 1.0);
}

TESTER_TEST("[MATH][F64x3]: ops + cross")
{
	F64x3 a = {1.0, 2.0, 3.0};
	F64x3 b = {4.0, 5.0, 6.0};
	TESTER_CHECK(a + b == F64x3{5.0, 7.0, 9.0});
	TESTER_CHECK(f64x3_dot(a, b) == 32.0);
	TESTER_CHECK(f64x3_cross(F64X3_RIGHT, F64X3_UP) == F64X3_BACKWARD);
	TESTER_CHECK(F64X3_FORWARD == F64x3{0.0, 0.0, -1.0});

	F64x3 clamp_v  = {-2.0, 5.0, 7.0};
	F64x3 clamp_lo = { 0.0, 0.0, 0.0};
	F64x3 clamp_hi = { 3.0, 3.0, 6.0};
	TESTER_CHECK(f64x3_clamp(clamp_v, clamp_lo, clamp_hi) == F64x3{0.0, 3.0, 6.0});
}

TESTER_TEST("[MATH][F64x4]: alignment + ops + dot")
{
	TESTER_CHECK(sizeof(F64x4)  == 32);
	TESTER_CHECK(alignof(F64x4) == 32);

	F64x4 a = {1.0, 2.0, 3.0, 4.0};
	F64x4 b = {5.0, 6.0, 7.0, 8.0};
	TESTER_CHECK(a + b == F64x4{6.0, 8.0, 10.0, 12.0});
	TESTER_CHECK(b - a == F64x4{4.0, 4.0,  4.0,  4.0});
	TESTER_CHECK(a * 2.0 == F64x4{2.0, 4.0, 6.0, 8.0});
	TESTER_CHECK(f64_approx_equal(f64x4_dot(a, b), 70.0, 1e-10));

	F64x4 v = {2.0, 2.0, 2.0, 2.0};
	TESTER_CHECK(f64_approx_equal(f64x4_length(v), 4.0, 1e-10));
	TESTER_CHECK(f64_approx_equal(f64x4_length(f64x4_normalize(v)), 1.0, 1e-10));

	TESTER_CHECK(f64x4_min(a, b) == a);
	TESTER_CHECK(f64x4_max(a, b) == b);
	TESTER_CHECK(f64x4_from_f64(1.5) == F64x4{1.5, 1.5, 1.5, 1.5});
}

// ============================================================================
// Vectors — I32x2 / I32x3 / I32x4 / U32x2 / U32x3 / U32x4
// ============================================================================

TESTER_TEST("[MATH][I32x2]: basic ops")
{
	I32x2 a = {3, -4};
	I32x2 b = {1,  2};
	TESTER_CHECK(a + b == I32x2{4, -2});
	TESTER_CHECK(a - b == I32x2{2, -6});
	TESTER_CHECK(-a   == I32x2{-3, 4});
	TESTER_CHECK(a * 2 == I32x2{6, -8});
	TESTER_CHECK(i32x2_dot(a, b) == -5);
	TESTER_CHECK(i32x2_length_squared(a) == 25);
	TESTER_CHECK(i32x2_abs(a) == I32x2{3, 4});
	TESTER_CHECK(i32x2_min(a, b) == I32x2{1, -4});
	TESTER_CHECK(i32x2_max(a, b) == I32x2{3,  2});
	TESTER_CHECK(i32x2_clamp(I32x2{-5, 10}, I32x2{0, 0}, I32x2{4, 4}) == I32x2{0, 4});
}

TESTER_TEST("[MATH][I32x3]: basic ops")
{
	I32x3 a = {1, 2, 3};
	I32x3 b = {4, 5, 6};
	TESTER_CHECK(a + b == I32x3{5, 7, 9});
	TESTER_CHECK(i32x3_dot(a, b) == 32);
	TESTER_CHECK(i32x3_abs(I32x3{-1, -2, -3}) == I32x3{1, 2, 3});
	TESTER_CHECK(i32x3_clamp(I32x3{-1, 5, 10}, I32x3{0, 0, 0}, I32x3{3, 3, 3}) == I32x3{0, 3, 3});
}

TESTER_TEST("[MATH][I32x4]: SIMD ops + alignment")
{
	TESTER_CHECK(sizeof(I32x4)  == 16);
	TESTER_CHECK(alignof(I32x4) == 16);

	I32x4 a = {1, 2, 3, 4};
	I32x4 b = {5, 6, 7, 8};
	TESTER_CHECK(a + b == I32x4{6, 8, 10, 12});
	TESTER_CHECK(b - a == I32x4{4, 4,  4,  4});
	TESTER_CHECK(-a    == I32x4{-1, -2, -3, -4});
	TESTER_CHECK(a * 3 == I32x4{3, 6, 9, 12});
	TESTER_CHECK(i32x4_dot(a, b) == 70);
	TESTER_CHECK(i32x4_abs(I32x4{-1, 2, -3, 4}) == I32x4{1, 2, 3, 4});
	TESTER_CHECK(i32x4_min(a, b) == a);
	TESTER_CHECK(i32x4_max(a, b) == b);
	TESTER_CHECK(i32x4_clamp(I32x4{-5, 0, 5, 20}, I32x4{0, 0, 0, 0}, I32x4{10, 10, 10, 10}) == I32x4{0, 0, 5, 10});
	TESTER_CHECK(i32x4_from_i32(7) == I32x4{7, 7, 7, 7});
}

TESTER_TEST("[MATH][U32x2]: basic ops")
{
	U32x2 a = {3u, 4u};
	U32x2 b = {1u, 2u};
	TESTER_CHECK(a + b == U32x2{4u, 6u});
	TESTER_CHECK(a - b == U32x2{2u, 2u});
	TESTER_CHECK(a * 2u == U32x2{6u, 8u});
	TESTER_CHECK(u32x2_dot(a, b) == 11u);
	TESTER_CHECK(u32x2_length_squared(a) == 25u);
	TESTER_CHECK(u32x2_min(a, b) == U32x2{1u, 2u});
	TESTER_CHECK(u32x2_max(a, b) == U32x2{3u, 4u});
	TESTER_CHECK(u32x2_clamp(U32x2{10u, 5u}, U32x2{0u, 0u}, U32x2{4u, 4u}) == U32x2{4u, 4u});
}

TESTER_TEST("[MATH][U32x3]: basic ops")
{
	U32x3 a = {1u, 2u, 3u};
	U32x3 b = {4u, 5u, 6u};
	TESTER_CHECK(a + b == U32x3{5u, 7u, 9u});
	TESTER_CHECK(u32x3_dot(a, b) == 32u);
	TESTER_CHECK(u32x3_clamp(U32x3{10u, 2u, 7u}, U32x3{0u, 0u, 0u}, U32x3{5u, 5u, 5u}) == U32x3{5u, 2u, 5u});
}

TESTER_TEST("[MATH][U32x4]: SIMD ops + alignment")
{
	TESTER_CHECK(sizeof(U32x4)  == 16);
	TESTER_CHECK(alignof(U32x4) == 16);

	U32x4 a = {1u, 2u, 3u, 4u};
	U32x4 b = {5u, 6u, 7u, 8u};
	TESTER_CHECK(a + b == U32x4{6u, 8u, 10u, 12u});
	TESTER_CHECK(b - a == U32x4{4u, 4u,  4u,  4u});
	TESTER_CHECK(a * 3u == U32x4{3u, 6u, 9u, 12u});
	TESTER_CHECK(u32x4_dot(a, b) == 70u);
	TESTER_CHECK(u32x4_min(a, b) == a);
	TESTER_CHECK(u32x4_max(a, b) == b);
	TESTER_CHECK(u32x4_clamp(U32x4{20u, 1u, 8u, 3u}, U32x4{0u, 0u, 0u, 0u}, U32x4{5u, 5u, 5u, 5u}) == U32x4{5u, 1u, 5u, 3u});
	TESTER_CHECK(u32x4_from_u32(5u) == U32x4{5u, 5u, 5u, 5u});
}

// ============================================================================
// Matrices — F32x2x2 / F32x3x3 / F32x4x4 / F64 mirror
// ============================================================================

TESTER_TEST("[MATH][F32x2x2]: identity + ops")
{
	F32x2x2 I = f32x2x2_identity();
	TESTER_CHECK((I == F32x2x2{1.0f, 0.0f, 0.0f, 1.0f}));

	F32x2x2 A = {1.0f, 2.0f, 3.0f, 4.0f};
	F32x2x2 B = {5.0f, 6.0f, 7.0f, 8.0f};
	TESTER_CHECK(A + B == F32x2x2{6.0f, 8.0f, 10.0f, 12.0f});
	TESTER_CHECK(A * 2.0f == F32x2x2{2.0f, 4.0f, 6.0f, 8.0f});
	TESTER_CHECK(f32x2x2_transpose(A) == F32x2x2{1.0f, 3.0f, 2.0f, 4.0f});
	TESTER_CHECK(f32x2x2_determinant(A) == -2.0f);

	F32x2x2 Ainv = f32x2x2_inverse(A);
	F32x2x2 P    = A * Ainv;
	TESTER_CHECK(f32_approx_equal(P.m00, 1.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(P.m01, 0.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(P.m10, 0.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(P.m11, 1.0f, 1e-5f));
}

TESTER_TEST("[MATH][F32x3x3]: layout + identity + mul + inverse")
{
	// Padded row layout: 48 bytes, matches std140/MSL matrix_float3x3.
	TESTER_CHECK(sizeof(F32x3x3)  == 48);
	TESTER_CHECK(alignof(F32x3x3) == 16);

	F32x3x3 I = f32x3x3_identity();
	TESTER_CHECK(I.m00 == 1.0f && I.m11 == 1.0f && I.m22 == 1.0f);
	TESTER_CHECK(I.m01 == 0.0f && I.m10 == 0.0f);

	F32x3 v = {1.0f, 2.0f, 3.0f};
	TESTER_CHECK(v * I == v);

	F32x3x3 A = {
		1.0f, 2.0f, 3.0f, 0.0f,
		0.0f, 1.0f, 4.0f, 0.0f,
		5.0f, 6.0f, 0.0f, 0.0f
	};
	F32x3x3 Ainv = f32x3x3_inverse(A);
	F32x3x3 P    = A * Ainv;
	TESTER_CHECK(f32_approx_equal(P.m00, 1.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(P.m11, 1.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(P.m22, 1.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(P.m01, 0.0f, 1e-5f));
}

TESTER_TEST("[MATH][F32x4x4]: layout + identity")
{
	TESTER_CHECK(sizeof(F32x4x4)  == 64);
	TESTER_CHECK(alignof(F32x4x4) == 16);

	F32x4x4 I = f32x4x4_identity();
	F32x4 v = {1.0f, 2.0f, 3.0f, 4.0f};
	TESTER_CHECK(v * I == v);
	TESTER_CHECK(I * I == I);
}

TESTER_TEST("[MATH][F32x4x4]: mat-mat mul known values")
{
	F32x4x4 A = {
		1.0f, 2.0f, 3.0f, 4.0f,
		5.0f, 6.0f, 7.0f, 8.0f,
		9.0f, 10.0f, 11.0f, 12.0f,
		13.0f, 14.0f, 15.0f, 16.0f
	};
	// A * identity = A.
	TESTER_CHECK(A * f32x4x4_identity() == A);
	// identity * A = A.
	TESTER_CHECK(f32x4x4_identity() * A == A);
}

TESTER_TEST("[MATH][F32x4x4]: translation / scaling / rotation")
{
	F32x4 origin = {0.0f, 0.0f, 0.0f, 1.0f};

	F32x4x4 T = f32x4x4_translation(5.0f, 6.0f, 7.0f);
	F32x4 t_applied = origin * T;
	TESTER_CHECK(f32_approx_equal(t_applied.x, 5.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(t_applied.y, 6.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(t_applied.z, 7.0f, 1e-5f));

	F32x4x4 S = f32x4x4_scaling(2.0f, 3.0f, 4.0f);
	F32x4 unit = {1.0f, 1.0f, 1.0f, 1.0f};
	F32x4 s_applied = unit * S;
	TESTER_CHECK(f32_approx_equal(s_applied.x, 2.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(s_applied.y, 3.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(s_applied.z, 4.0f, 1e-5f));

	// Rotate +X by 90 deg about Y axis → -Z (right-handed).
	F32x4 x_axis = {1.0f, 0.0f, 0.0f, 0.0f};
	F32x4 rotated = x_axis * f32x4x4_rotation_y(F32_PI_OVER_2);
	TESTER_CHECK(f32_approx_equal(rotated.x, 0.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(rotated.z, -1.0f, 1e-5f));
}

TESTER_TEST("[MATH][F32x4x4]: inverse round-trip")
{
	F32x4x4 A = {
		2.0f, 5.0f, 0.0f, 8.0f,
		1.0f, 4.0f, 2.0f, 6.0f,
		7.0f, 8.0f, 8.0f, 3.0f,
		1.0f, 5.0f, 7.0f, 8.0f
	};
	TESTER_CHECK(f32x4x4_is_invertible(A));
	F32x4x4 Ainv = f32x4x4_inverse(A);
	F32x4x4 P    = A * Ainv;
	// P should be identity within 1e-4 tolerance.
	TESTER_CHECK(f32x4x4_approx_equal(P, f32x4x4_identity(), 1e-4f));
}

TESTER_TEST("[MATH][F32x4x4]: look_at + canonical convention")
{
	// Camera at (0, 0, 5) looking at origin, Y up. Target (origin) should map to -Z in view space.
	F32x4x4 view = f32x4x4_look_at(F32x3{0.0f, 0.0f, 5.0f}, F32X3_ZERO, F32X3_UP);
	F32x4   origin = {0.0f, 0.0f, 0.0f, 1.0f};
	F32x4   view_pos = origin * view;
	// Origin is 5 units along -Z in view space.
	TESTER_CHECK(f32_approx_equal(view_pos.z, -5.0f, 1e-4f));
	TESTER_CHECK(f32_approx_equal(view_pos.x,  0.0f, 1e-4f));
	TESTER_CHECK(f32_approx_equal(view_pos.y,  0.0f, 1e-4f));
}

TESTER_TEST("[MATH][F32x4x4]: perspective canonical NDC")
{
	F32x4x4 P = f32x4x4_perspective(F32_PI_OVER_2, 1.0f, 0.1f, 100.0f);

	// Point at -znear maps to clip.z / clip.w = 0 (near plane -> NDC z = 0).
	F32x4 near_pt = F32x4{0.0f, 0.0f, -0.1f, 1.0f} * P;
	TESTER_CHECK(f32_approx_equal(near_pt.z / near_pt.w, 0.0f, 1e-4f));

	// Point at -zfar maps to NDC z = 1.
	F32x4 far_pt = F32x4{0.0f, 0.0f, -100.0f, 1.0f} * P;
	TESTER_CHECK(f32_approx_equal(far_pt.z / far_pt.w, 1.0f, 1e-4f));
}

TESTER_TEST("[MATH][F32x4x4]: orthographic canonical NDC")
{
	F32x4x4 P = f32x4x4_orthographic(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);

	// Point (-1, -1, 0) (view space near-plane lower-left) -> NDC (-1, -1, 0).
	F32x4 near_ll = F32x4{-1.0f, -1.0f, 0.0f, 1.0f} * P;
	TESTER_CHECK(f32_approx_equal(near_ll.x, -1.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(near_ll.y, -1.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(near_ll.z,  0.0f, 1e-5f));

	// Point (1, 1, -1) (view space far-plane upper-right) -> NDC (1, 1, 1).
	F32x4 far_ur = F32x4{1.0f, 1.0f, -1.0f, 1.0f} * P;
	TESTER_CHECK(f32_approx_equal(far_ur.x, 1.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(far_ur.y, 1.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(far_ur.z, 1.0f, 1e-5f));
}

TESTER_TEST("[MATH][F32x4x4]: project / unproject round-trip")
{
	F32x4x4 view    = f32x4x4_look_at(F32x3{0.0f, 0.0f, 5.0f}, F32X3_ZERO, F32X3_UP);
	F32x4x4 proj    = f32x4x4_perspective(F32_PI_OVER_2, 16.0f / 9.0f, 0.1f, 100.0f);
	F32x4x4 vp      = view * proj;
	F32x4x4 vp_inv  = f32x4x4_inverse(vp);
	F32x4   viewport = {0.0f, 0.0f, 1920.0f, 1080.0f};

	F32x3 world  = {1.0f, 0.5f, 0.0f};
	F32x3 screen = f32x3_project(world, vp, viewport);
	F32x3 back   = f32x3_unproject(screen, vp_inv, viewport);

	TESTER_CHECK(f32x3_approx_equal(world, back, 1e-3f));
}

TESTER_TEST("[MATH][F64x4x4]: layout + identity + mul")
{
	TESTER_CHECK(sizeof(F64x4x4)  == 128);
	TESTER_CHECK(alignof(F64x4x4) == 32);

	F64x4x4 I = f64x4x4_identity();
	F64x4 v = {1.0, 2.0, 3.0, 4.0};
	TESTER_CHECK(v * I == v);
	TESTER_CHECK(I * I == I);

	F64x4x4 A = {
		1.0, 2.0, 3.0, 4.0,
		5.0, 6.0, 7.0, 8.0,
		9.0, 10.0, 11.0, 12.0,
		13.0, 14.0, 15.0, 16.0
	};
	TESTER_CHECK(A * f64x4x4_identity() == A);
}

TESTER_TEST("[MATH][F64x3x3]: layout + identity + inverse")
{
	TESTER_CHECK(sizeof(F64x3x3)  == 96);
	TESTER_CHECK(alignof(F64x3x3) == 32);

	F64x3x3 A = {
		1.0, 2.0, 3.0, 0.0,
		0.0, 1.0, 4.0, 0.0,
		5.0, 6.0, 0.0, 0.0
	};
	F64x3x3 Ainv = f64x3x3_inverse(A);
	F64x3x3 P    = A * Ainv;
	TESTER_CHECK(f64_approx_equal(P.m00, 1.0, 1e-10));
	TESTER_CHECK(f64_approx_equal(P.m11, 1.0, 1e-10));
	TESTER_CHECK(f64_approx_equal(P.m22, 1.0, 1e-10));
}

TESTER_TEST("[MATH][F64x2x2]: ops")
{
	F64x2x2 I = f64x2x2_identity();
	F64x2 v = {3.0, 4.0};
	TESTER_CHECK(v * I == v);
	TESTER_CHECK(f64x2x2_determinant(F64x2x2{1.0, 2.0, 3.0, 4.0}) == -2.0);
}

// ============================================================================
// Quaternion
// ============================================================================

TESTER_TEST("[MATH][Quaternion]: identity + operators")
{
	Quaternion I = quaternion_identity();
	TESTER_CHECK(I == QUATERNION_IDENTITY);
	TESTER_CHECK((I == Quaternion{1.0f, 0.0f, 0.0f, 0.0f}));

	Quaternion p = {1.0f, 2.0f, 3.0f, 4.0f};
	Quaternion q = {5.0f, 6.0f, 7.0f, 8.0f};
	TESTER_CHECK((p + q == Quaternion{6.0f, 8.0f, 10.0f, 12.0f}));
	TESTER_CHECK((-p   == Quaternion{-1.0f, -2.0f, -3.0f, -4.0f}));
	TESTER_CHECK((p * 2.0f == Quaternion{2.0f, 4.0f, 6.0f, 8.0f}));
	TESTER_CHECK(f32_approx_equal(quaternion_dot(p, q), 70.0f, 1e-5f));
}

TESTER_TEST("[MATH][Quaternion]: from_axis_angle + length")
{
	// 0-angle about any axis is identity.
	TESTER_CHECK(quaternion_approx_equal(quaternion_from_axis_angle(F32X3_UP, 0.0f), QUATERNION_IDENTITY, 1e-6f));

	// 180° about X: (0, 1, 0, 0).
	Quaternion r = quaternion_from_axis_angle(F32X3_RIGHT, F32_PI);
	TESTER_CHECK(f32_approx_equal(r.w, 0.0f, 1e-6f));
	TESTER_CHECK(f32_approx_equal(r.x, 1.0f, 1e-6f));

	// All unit-axis rotations produce unit quaternions.
	for (F32 theta : {0.5f, 1.0f, 1.5f, 2.0f, 3.0f})
	{
		Quaternion q = quaternion_from_axis_angle(F32X3_UP, theta);
		TESTER_CHECK(f32_approx_equal(quaternion_length(q), 1.0f, 1e-6f));
	}
}

TESTER_TEST("[MATH][Quaternion]: inverse round-trip")
{
	Quaternion q = quaternion_normalize(Quaternion{1.0f, 2.0f, 3.0f, 4.0f});
	Quaternion inv = quaternion_inverse(q);
	Quaternion p = q * inv;
	TESTER_CHECK(quaternion_approx_equal(p, QUATERNION_IDENTITY, 1e-5f));
}

TESTER_TEST("[MATH][Quaternion]: vector rotation by quaternion")
{
	// 90° about +Y: rotates +X to -Z (right-handed).
	Quaternion q = quaternion_from_axis_angle(F32X3_UP, F32_PI_OVER_2);
	F32x3 rotated = F32X3_RIGHT * q;
	TESTER_CHECK(f32x3_approx_equal(rotated, F32X3_FORWARD, 1e-5f));

	// Rotating +Y about +Y leaves it unchanged.
	F32x3 same = F32X3_UP * q;
	TESTER_CHECK(f32x3_approx_equal(same, F32X3_UP, 1e-5f));

	// Identity rotation leaves vectors unchanged.
	TESTER_CHECK(f32x3_approx_equal(F32X3_RIGHT * QUATERNION_IDENTITY, F32X3_RIGHT, 1e-6f));
}

TESTER_TEST("[MATH][Quaternion]: from_euler_angles is right-handed")
{
	// Euler (pitch=0, yaw=pi/2, roll=0): rotates +X to -Z (right-handed about +Y).
	// This is the test that used to fail under the old left-handed implementation.
	Quaternion q = quaternion_from_euler_angles(F32x3{0.0f, F32_PI_OVER_2, 0.0f});
	F32x3 rotated = F32X3_RIGHT * q;
	TESTER_CHECK(f32x3_approx_equal(rotated, F32X3_FORWARD, 1e-5f));

	// Round-trip through to_euler_angles.
	F32x3 angles = F32x3{0.3f, 0.5f, -0.7f};
	Quaternion q2 = quaternion_from_euler_angles(angles);
	F32x3 recovered = quaternion_to_euler_angles(q2);
	TESTER_CHECK(f32x3_approx_equal(recovered, angles, 1e-4f));
}

TESTER_TEST("[MATH][Quaternion]: slerp endpoints + midpoint")
{
	Quaternion a = quaternion_from_axis_angle(F32X3_UP, 0.0f);
	Quaternion b = quaternion_from_axis_angle(F32X3_UP, F32_PI_OVER_2);

	TESTER_CHECK(quaternion_approx_equal(quaternion_slerp(a, b, 0.0f), a, 1e-5f));
	TESTER_CHECK(quaternion_approx_equal(quaternion_slerp(a, b, 1.0f), b, 1e-5f));

	// Midpoint is 45° about +Y.
	Quaternion mid = quaternion_slerp(a, b, 0.5f);
	Quaternion expected = quaternion_from_axis_angle(F32X3_UP, F32_PI_OVER_2 * 0.5f);
	TESTER_CHECK(quaternion_approx_equal(mid, expected, 1e-5f));
}

TESTER_TEST("[MATH][Quaternion]: from_to_rotation")
{
	// +X to +Y: 90° about +Z.
	Quaternion q = quaternion_from_to_rotation(F32X3_RIGHT, F32X3_UP);
	F32x3 rotated = F32X3_RIGHT * q;
	TESTER_CHECK(f32x3_approx_equal(rotated, F32X3_UP, 1e-5f));

	// Identity case: +X to +X.
	Quaternion ident = quaternion_from_to_rotation(F32X3_RIGHT, F32X3_RIGHT);
	TESTER_CHECK(quaternion_approx_equal(ident, QUATERNION_IDENTITY, 1e-4f));

	// Anti-parallel: +X to -X. Rotates 180° about some perpendicular axis.
	Quaternion flip = quaternion_from_to_rotation(F32X3_RIGHT, F32X3_LEFT);
	F32x3 flipped = F32X3_RIGHT * flip;
	TESTER_CHECK(f32x3_approx_equal(flipped, F32X3_LEFT, 1e-4f));
}

TESTER_TEST("[MATH][Quaternion]: look_rotation")
{
	// Looking down -Z with +Y up → identity orientation.
	Quaternion q = quaternion_look_rotation(F32X3_FORWARD, F32X3_UP);
	TESTER_CHECK(quaternion_approx_equal(q, QUATERNION_IDENTITY, 1e-4f));

	// Looking down +X with +Y up → 90° about +Y.
	Quaternion q2 = quaternion_look_rotation(F32X3_RIGHT, F32X3_UP);
	F32x3 forward_of_q2 = F32X3_FORWARD * q2;
	TESTER_CHECK(f32x3_approx_equal(forward_of_q2, F32X3_RIGHT, 1e-4f));
}

TESTER_TEST("[MATH][Quaternion]: rotate_towards clamps step")
{
	Quaternion a = quaternion_identity();
	// 120° about UP — unambiguous shortest arc (180° has ambiguous direction).
	Quaternion b = quaternion_from_axis_angle(F32X3_UP, F32_PI * 2.0f / 3.0f);
	F32 full_angle = F32_PI * 2.0f / 3.0f;

	// Small max step — doesn't reach target.
	Quaternion stepped = quaternion_rotate_towards(a, b, 0.1f);
	TESTER_CHECK(!quaternion_approx_equal(stepped, b, 1e-3f));

	// Large max step (> angular distance) — snaps to target.
	Quaternion snapped = quaternion_rotate_towards(a, b, F32_PI * 2.0f);
	TESTER_CHECK(quaternion_approx_equal(snapped, b, 1e-5f));

	// Mid-range step — 60° about UP. Compare via a probe vector (robust
	// against q / -q double cover that field-wise comparison would trip on).
	Quaternion half = quaternion_rotate_towards(a, b, full_angle * 0.5f);
	F32x3 via_half     = F32X3_RIGHT * half;
	F32x3 via_expected = F32X3_RIGHT * quaternion_from_axis_angle(F32X3_UP, full_angle * 0.5f);
	TESTER_CHECK(f32x3_approx_equal(via_half, via_expected, 1e-4f));
}

TESTER_TEST("[MATH][Quaternion]: f32x4x4_from_quaternion agrees with vector rotation")
{
	Quaternion q = quaternion_from_axis_angle(F32X3_UP, F32_PI_OVER_2);
	F32x4x4 M   = f32x4x4_from_quaternion(q);

	// Rotating +X via matrix and via quaternion should match.
	F32x3 via_quat   = F32X3_RIGHT * q;
	F32x4 via_matrix = F32x4{1.0f, 0.0f, 0.0f, 0.0f} * M;

	TESTER_CHECK(f32_approx_equal(via_quat.x, via_matrix.x, 1e-5f));
	TESTER_CHECK(f32_approx_equal(via_quat.y, via_matrix.y, 1e-5f));
	TESTER_CHECK(f32_approx_equal(via_quat.z, via_matrix.z, 1e-5f));
}

TESTER_TEST("[MATH][F32x4x4]: decompose TRS round-trip")
{
	F32x3 T = {5.0f, 6.0f, 7.0f};
	Quaternion R = quaternion_from_axis_angle(f32x3_normalize(F32x3{1.0f, 1.0f, 1.0f}), 1.2f);
	F32x3 S = {2.0f, 3.0f, 4.0f};

	F32x4x4 M = f32x4x4_scaling(S) * f32x4x4_from_quaternion(R) * f32x4x4_translation(T);

	F32x3 out_t;
	Quaternion out_r;
	F32x3 out_s;
	TESTER_CHECK(f32x4x4_decompose(M, &out_t, &out_r, &out_s));

	TESTER_CHECK(f32x3_approx_equal(out_t, T, 1e-4f));
	TESTER_CHECK(f32x3_approx_equal(out_s, S, 1e-4f));
	// Quaternions ±q represent the same rotation — check by applying to a probe vector.
	F32x3 probe  = f32x3_normalize(F32x3{0.3f, -0.4f, 0.8f});
	TESTER_CHECK(f32x3_approx_equal(probe * R, probe * out_r, 1e-4f));
}

TESTER_TEST("[MATH][F32x4x4]: inverse round-trip M * inv(M) == I")
{
	// TRS-composed matrix (well-conditioned, invertible).
	F32x3 T = {1.5f, -2.3f, 4.7f};
	Quaternion R = quaternion_from_axis_angle(f32x3_normalize(F32x3{0.3f, 0.8f, -0.2f}), 0.9f);
	F32x3 S = {2.0f, 0.5f, 3.0f};
	F32x4x4 M    = f32x4x4_scaling(S) * f32x4x4_from_quaternion(R) * f32x4x4_translation(T);
	F32x4x4 Minv = f32x4x4_inverse(M);

	TESTER_CHECK(f32x4x4_is_invertible(M));

	F32x4x4 I      = f32x4x4_identity();
	F32x4x4 prod_L = M * Minv;
	F32x4x4 prod_R = Minv * M;
	for (I32 i = 0; i < 16; i++)
	{
		TESTER_CHECK(f32_approx_equal(f32x4x4_at(prod_L, i), f32x4x4_at(I, i), 1e-4f));
		TESTER_CHECK(f32_approx_equal(f32x4x4_at(prod_R, i), f32x4x4_at(I, i), 1e-4f));
	}

	// Singular matrix — determinant 0, not invertible.
	F32x4x4 singular = F32x4x4{};  // all zeros
	TESTER_CHECK(!f32x4x4_is_invertible(singular));
}

// ============================================================================
// Random (xoshiro256**)
// ============================================================================

TESTER_TEST("[MATH][Random]: seed determinism")
{
	Random a, b;
	random_seed(a, 42);
	random_seed(b, 42);
	for (int i = 0; i < 1000; ++i)
		TESTER_CHECK(random_u64(a) == random_u64(b));

	// Different seed → different stream (first 10 draws).
	Random c, d;
	random_seed(c, 1);
	random_seed(d, 2);
	bool any_diff = false;
	for (int i = 0; i < 10; ++i)
		if (random_u64(c) != random_u64(d)) { any_diff = true; break; }
	TESTER_CHECK(any_diff);
}

TESTER_TEST("[MATH][Random]: f32_random_unit in [0, 1)")
{
	Random rng;
	random_seed(rng, 12345);
	for (int i = 0; i < 10000; ++i)
	{
		F32 x = f32_random_unit(rng);
		TESTER_CHECK(x >= 0.0f);
		TESTER_CHECK(x <  1.0f);
	}
}

TESTER_TEST("[MATH][Random]: f32_random_range bounds")
{
	Random rng;
	random_seed(rng, 0xABCDEF);
	for (int i = 0; i < 10000; ++i)
	{
		F32 x = f32_random_range(rng, -5.0f, 5.0f);
		TESTER_CHECK(x >= -5.0f);
		TESTER_CHECK(x <   5.0f);
	}
}

TESTER_TEST("[MATH][Random]: i32_random_range inclusive bounds")
{
	Random rng;
	random_seed(rng, 777);
	I32 observed_min =  1000;
	I32 observed_max = -1000;
	for (int i = 0; i < 10000; ++i)
	{
		I32 v = i32_random_range(rng, -3, 5);
		TESTER_CHECK(v >= -3);
		TESTER_CHECK(v <=  5);
		if (v < observed_min) observed_min = v;
		if (v > observed_max) observed_max = v;
	}
	// With 10000 draws across 9 bins we should hit both endpoints.
	TESTER_CHECK(observed_min == -3);
	TESTER_CHECK(observed_max ==  5);
}

TESTER_TEST("[MATH][Random]: f32x3_random_in_unit_sphere / on_unit_sphere")
{
	Random rng;
	random_seed(rng, 99);

	for (int i = 0; i < 1000; ++i)
	{
		F32x3 in_sphere = f32x3_random_in_unit_sphere(rng);
		TESTER_CHECK(f32x3_length_squared(in_sphere) <= 1.0f + 1e-5f);

		F32x3 on_sphere = f32x3_random_on_unit_sphere(rng);
		TESTER_CHECK(f32_approx_equal(f32x3_length(on_sphere), 1.0f, 1e-5f));
	}
}

TESTER_TEST("[MATH][Random]: f32x2_random_in_unit_disk")
{
	Random rng;
	random_seed(rng, 0xBEEF);
	for (int i = 0; i < 1000; ++i)
	{
		F32x2 p = f32x2_random_in_unit_disk(rng);
		TESTER_CHECK(f32x2_length_squared(p) <= 1.0f + 1e-5f);
	}
}

TESTER_TEST("[MATH][Random]: quaternion_random produces unit quaternions")
{
	Random rng;
	random_seed(rng, 2026);
	for (int i = 0; i < 1000; ++i)
	{
		Quaternion q = quaternion_random(rng);
		TESTER_CHECK(f32_approx_equal(quaternion_length(q), 1.0f, 1e-5f));
	}
}

// ============================================================================
// Formatter smoke tests — make sure every math type round-trips through
// Core's formatter without compile errors.
// ============================================================================

TESTER_TEST("[MATH][format]: vectors + matrix + quaternion")
{
	Formatter f = formatter_init();
	DEFER(formatter_deinit(f));

	// F32x3 non-empty string produced.
	formatter_clear(f);
	format(f, F32x3{1.0f, 2.0f, 3.0f});
	TESTER_CHECK(f.buffer.count > 0);

	// F32x4x4 non-empty string produced.
	formatter_clear(f);
	format(f, f32x4x4_identity());
	TESTER_CHECK(f.buffer.count > 0);

	// Quaternion formatter fires.
	formatter_clear(f);
	format(f, QUATERNION_IDENTITY);
	TESTER_CHECK(f.buffer.count > 0);

	// Integer vector formatter fires.
	formatter_clear(f);
	format(f, U32x2{10u, 20u});
	TESTER_CHECK(f.buffer.count > 0);
}
