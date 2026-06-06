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

static constexpr F32x3 TEST_F32_ZERO      = { 0.0f,  0.0f,  0.0f};
static constexpr F32x3 TEST_F32_RIGHT     = { 1.0f,  0.0f,  0.0f};
static constexpr F32x3 TEST_F32_LEFT      = {-1.0f,  0.0f,  0.0f};
static constexpr F32x3 TEST_F32_UP        = { 0.0f,  1.0f,  0.0f};
static constexpr F32x3 TEST_F32_FORWARD   = { 0.0f,  0.0f, -1.0f};
static constexpr F32x3 TEST_F32_BACKWARD  = { 0.0f,  0.0f,  1.0f};
static constexpr F32x4 TEST_F32_VEC4_ZERO = {0.0f, 0.0f, 0.0f, 0.0f};
static constexpr F32x4 TEST_F32_VEC4_ONE  = {1.0f, 1.0f, 1.0f, 1.0f};

static constexpr F64x2 TEST_F64_VEC2_ZERO = {0.0, 0.0};
static constexpr F64x3 TEST_F64_ZERO      = { 0.0,  0.0,  0.0};
static constexpr F64x3 TEST_F64_RIGHT     = { 1.0,  0.0,  0.0};
static constexpr F64x3 TEST_F64_UP        = { 0.0,  1.0,  0.0};
static constexpr F64x3 TEST_F64_FORWARD   = { 0.0,  0.0, -1.0};
static constexpr F64x3 TEST_F64_BACKWARD  = { 0.0,  0.0,  1.0};
static constexpr F64x4 TEST_F64_VEC4_ZERO = {0.0, 0.0, 0.0, 0.0};

static constexpr Quaternion TEST_QUAT_IDENTITY = {1.0f, 0.0f, 0.0f, 0.0f};

// ============================================================================
// Scalar helpers — F32
// ============================================================================

TESTER_TEST("[MATH][f32]: constants")
{
	TESTER_CHECK(f32_approx_equal(F32_PI * 2.0f, F32_TAU, F32_EPSILON));
	TESTER_CHECK(f32_approx_equal(F32_PI * 0.5f, F32_PI * 0.5f, F32_EPSILON));
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

	TESTER_CHECK(f32_floor( 1.75f) ==  1.0f);
	TESTER_CHECK(f32_floor(-1.25f) == -2.0f);
	TESTER_CHECK(f32_ceil( 1.25f)  ==  2.0f);
	TESTER_CHECK(f32_round(1.5f)   ==  2.0f);
	TESTER_CHECK(f32_trunc(-1.75f) == -1.0f);
	TESTER_CHECK(f32_approx_equal(f32_fract(3.25f), 0.25f, 1e-6f));
	TESTER_CHECK(f32_saturate(-1.0f) == 0.0f);
	TESTER_CHECK(f32_saturate( 0.5f) == 0.5f);
	TESTER_CHECK(f32_saturate( 2.0f) == 1.0f);
	TESTER_CHECK(f32_approx_equal(f32_inverse_lerp(10.0f, 20.0f, 15.0f), 0.5f, 1e-6f));
	TESTER_CHECK(f32_approx_equal(f32_remap(0.0f, 10.0f, -1.0f, 1.0f, 2.5f), -0.5f, 1e-6f));
	TESTER_CHECK(f32_move_towards(0.0f, 10.0f, 3.0f) == 3.0f);
	TESTER_CHECK(f32_move_towards(0.0f,  2.0f, 3.0f) == 2.0f);
	TESTER_CHECK(f32_approx_equal_relative(1000000.0f, 1000000.5f, 1e-6f));
	TESTER_CHECK(!f32_approx_equal(1000000.0f, 1000000.5f, 1e-6f));
	TESTER_CHECK(f32_approx_equal(f32_wrap_radians(F32_PI * 3.0f), -F32_PI, 1e-6f));
	TESTER_CHECK(f32_approx_equal(f32_angle_delta(350.0f * F32_TO_RADIANS, 10.0f * F32_TO_RADIANS), 20.0f * F32_TO_RADIANS, 1e-6f));
}

TESTER_TEST("[MATH][f32]: trig + power")
{
	TESTER_CHECK(f32_approx_equal(f32_sqrt(4.0f), 2.0f, 1e-6f));
	TESTER_CHECK(f32_approx_equal(f32_sin(0.0f), 0.0f, 1e-6f));
	TESTER_CHECK(f32_approx_equal(f32_cos(0.0f), 1.0f, 1e-6f));
	TESTER_CHECK(f32_approx_equal(f32_sin(F32_PI * 0.5f), 1.0f, 1e-6f));
	TESTER_CHECK(f32_approx_equal(f32_cos(F32_PI * 0.5f), 0.0f, 1e-6f));
	TESTER_CHECK(f32_approx_equal(f32_power(2.0f, 10.0f), 1024.0f, 1e-3f));
	TESTER_CHECK(f32_approx_equal(f32_modulo(10.5f, 3.0f), 1.5f, 1e-6f));
	TESTER_CHECK(f32_approx_equal(f32_atan2(1.0f, 0.0f), F32_PI * 0.5f, 1e-6f));
}

TESTER_TEST("[MATH][f32]: special values")
{
	TESTER_CHECK(f32_is_nan(F32_NAN));
	TESTER_CHECK(!f32_is_nan(0.0f));
	TESTER_CHECK(!f32_is_nan(F32_INFINITY));

	TESTER_CHECK(f32_is_infinite(F32_INFINITY));
	TESTER_CHECK(f32_is_infinite(F32_NEGATIVE_INFINITY));
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
	TESTER_CHECK(f64_floor(-1.25) == -2.0);
	TESTER_CHECK(f64_ceil(1.25) == 2.0);
	TESTER_CHECK(f64_round(1.5) == 2.0);
	TESTER_CHECK(f64_trunc(-1.75) == -1.0);
	TESTER_CHECK(f64_approx_equal(f64_fract(3.25), 0.25, 1e-12));
	TESTER_CHECK(f64_saturate(2.0) == 1.0);
	TESTER_CHECK(f64_approx_equal(f64_inverse_lerp(10.0, 20.0, 15.0), 0.5, 1e-12));
	TESTER_CHECK(f64_approx_equal(f64_remap(0.0, 10.0, -1.0, 1.0, 2.5), -0.5, 1e-12));
	TESTER_CHECK(f64_move_towards(0.0, 10.0, 3.0) == 3.0);
	TESTER_CHECK(f64_approx_equal_relative(1000000000000.0, 1000000000000.5, 1e-12));
	TESTER_CHECK(f64_approx_equal(f64_wrap_radians(F64_PI * 3.0), -F64_PI, 1e-12));
	TESTER_CHECK(f64_approx_equal(f64_angle_delta(350.0 * F64_TO_RADIANS, 10.0 * F64_TO_RADIANS), 20.0 * F64_TO_RADIANS, 1e-12));
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
	TESTER_CHECK(i32_is_power_of_two(16));
	TESTER_CHECK(!i32_is_power_of_two(18));
	TESTER_CHECK(i32_next_power_of_two(0) == 1);
	TESTER_CHECK(i32_next_power_of_two(17) == 32);
	TESTER_CHECK(i32_previous_power_of_two(17) == 16);
	TESTER_CHECK(i32_align_up(37, 8) == 40);
	TESTER_CHECK(i32_align_down(37, 8) == 32);
	TESTER_CHECK(i32_rotate_left(0x12345678, 8) == 0x34567812);
	TESTER_CHECK(i32_rotate_right(0x12345678, 8) == 0x78123456);
	TESTER_CHECK(i32_popcount(-1) == 32);
	TESTER_CHECK(i32_leading_zero_count(1) == 31);
	TESTER_CHECK(i32_trailing_zero_count(16) == 4);
	TESTER_CHECK(i32_byte_swap(0x12345678) == 0x78563412);
}

TESTER_TEST("[MATH][i64]: basic ops")
{
	TESTER_CHECK(i64_abs(-5ll) == 5ll);
	TESTER_CHECK(i64_sign(-2ll) == -1ll);
	TESTER_CHECK(i64_min(2ll, 3ll) == 2ll);
	TESTER_CHECK(i64_max(2ll, 3ll) == 3ll);
	TESTER_CHECK(i64_clamp(15ll, 0ll, 10ll) == 10ll);
	TESTER_CHECK(i64_is_power_of_two(16ll));
	TESTER_CHECK(!i64_is_power_of_two(18ll));
	TESTER_CHECK(i64_next_power_of_two(0ll) == 1ll);
	TESTER_CHECK(i64_next_power_of_two(17ll) == 32ll);
	TESTER_CHECK(i64_previous_power_of_two(17ll) == 16ll);
	TESTER_CHECK(i64_align_up(37ll, 8ll) == 40ll);
	TESTER_CHECK(i64_align_down(37ll, 8ll) == 32ll);
	TESTER_CHECK(i64_rotate_left(0x0123456789abcdefll, 8) == 0x23456789abcdef01ll);
	TESTER_CHECK((U64)i64_rotate_right(0x0123456789abcdefll, 8) == 0xef0123456789abcdull);
	TESTER_CHECK(i64_popcount(-1ll) == 64);
	TESTER_CHECK(i64_leading_zero_count(1ll) == 63);
	TESTER_CHECK(i64_trailing_zero_count(16ll) == 4);
	TESTER_CHECK((U64)i64_byte_swap(0x0123456789abcdefll) == 0xefcdab8967452301ull);
}

TESTER_TEST("[MATH][u32]: basic ops")
{
	TESTER_CHECK(u32_min(2u, 3u) == 2u);
	TESTER_CHECK(u32_max(2u, 3u) == 3u);
	TESTER_CHECK(u32_clamp(5u, 0u, 10u) ==  5u);
	TESTER_CHECK(u32_clamp(15u, 0u, 10u) == 10u);
	TESTER_CHECK(u32_is_power_of_two(16u));
	TESTER_CHECK(!u32_is_power_of_two(18u));
	TESTER_CHECK(u32_next_power_of_two(0u) == 1u);
	TESTER_CHECK(u32_next_power_of_two(17u) == 32u);
	TESTER_CHECK(u32_previous_power_of_two(17u) == 16u);
	TESTER_CHECK(u32_align_up(37u, 8u) == 40u);
	TESTER_CHECK(u32_align_down(37u, 8u) == 32u);
	TESTER_CHECK(u32_rotate_left(0x12345678u, 8) == 0x34567812u);
	TESTER_CHECK(u32_rotate_right(0x12345678u, 8) == 0x78123456u);
	TESTER_CHECK(u32_popcount(0xf0f0f00fu) == 16u);
	TESTER_CHECK(u32_leading_zero_count(0x00100000u) == 11u);
	TESTER_CHECK(u32_trailing_zero_count(0x00100000u) == 20u);
	TESTER_CHECK(u32_byte_swap(0x12345678u) == 0x78563412u);
}

TESTER_TEST("[MATH][u64]: basic ops")
{
	TESTER_CHECK(u64_min(2ull, 3ull) == 2ull);
	TESTER_CHECK(u64_max(2ull, 3ull) == 3ull);
	TESTER_CHECK(u64_clamp(15ull, 0ull, 10ull) == 10ull);
	TESTER_CHECK(u64_is_power_of_two(16ull));
	TESTER_CHECK(!u64_is_power_of_two(18ull));
	TESTER_CHECK(u64_next_power_of_two(0ull) == 1ull);
	TESTER_CHECK(u64_next_power_of_two(17ull) == 32ull);
	TESTER_CHECK(u64_previous_power_of_two(17ull) == 16ull);
	TESTER_CHECK(u64_align_up(37ull, 8ull) == 40ull);
	TESTER_CHECK(u64_align_down(37ull, 8ull) == 32ull);
	TESTER_CHECK(u64_rotate_left(0x0123456789abcdefull, 8) == 0x23456789abcdef01ull);
	TESTER_CHECK(u64_rotate_right(0x0123456789abcdefull, 8) == 0xef0123456789abcdull);
	TESTER_CHECK(u64_popcount(0xf0f0f0f0f0f0f0f0ull) == 32u);
	TESTER_CHECK(u64_leading_zero_count(1ull << 40) == 23u);
	TESTER_CHECK(u64_trailing_zero_count(1ull << 40) == 40u);
	TESTER_CHECK(u64_byte_swap(0x0123456789abcdefull) == 0xefcdab8967452301ull);
}

// ============================================================================
// Vectors — F32x2
// ============================================================================

TESTER_TEST("[MATH][F32x2]: operators")
{
	F32x2 a = {3.0f, 4.0f};
	F32x2 b = {1.0f, 2.0f};
	F32x2 designated = {.x = 3.0f, .y = 4.0f};
	TESTER_CHECK(designated == a);
	TESTER_CHECK(a.components[0] == 3.0f);
	TESTER_CHECK(a[1] == 4.0f);
	a[1] = 9.0f;
	TESTER_CHECK(a.y == 9.0f && a.components[1] == 9.0f);
	a.y = 4.0f;

	TESTER_CHECK(a + b == F32x2{4.0f, 6.0f});
	TESTER_CHECK(a - b == F32x2{2.0f, 2.0f});
	TESTER_CHECK(-a   == F32x2{-3.0f, -4.0f});
	TESTER_CHECK(a * 2.0f == F32x2{6.0f, 8.0f});
	TESTER_CHECK(2.0f * a == F32x2{6.0f, 8.0f});
	TESTER_CHECK(a / 2.0f == F32x2{1.5f, 2.0f});
	TESTER_CHECK(f32x2_from_f32(2.5f) == F32x2{2.5f, 2.5f});

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
	TESTER_CHECK(f32_approx_equal(f32x2_distance(a, F32x2{}), 5.0f, 1e-6f));
	TESTER_CHECK(f32x2_reflect(F32x2{1.0f, -1.0f}, F32x2{0.0f, 1.0f}) == F32x2{1.0f, 1.0f});
	TESTER_CHECK(f32x2_project(a, F32x2{1.0f, 0.0f}) == F32x2{3.0f, 0.0f});
	TESTER_CHECK(f32x2_reject(a, F32x2{1.0f, 0.0f}) == F32x2{0.0f, 4.0f});

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
	F32x3 designated = {.x = 1.0f, .y = 2.0f, .z = 3.0f};
	TESTER_CHECK(designated == a);
	TESTER_CHECK(a.components[2] == 3.0f);
	TESTER_CHECK(a[1] == 2.0f);
	a[0] = 7.0f;
	TESTER_CHECK(a.x == 7.0f && a.components[0] == 7.0f);
	a.x = 1.0f;

	TESTER_CHECK(a + b == F32x3{5.0f, 7.0f, 9.0f});
	TESTER_CHECK(a - b == F32x3{-3.0f, -3.0f, -3.0f});
	TESTER_CHECK(-a   == F32x3{-1.0f, -2.0f, -3.0f});
	TESTER_CHECK(a * 2.0f == F32x3{2.0f, 4.0f, 6.0f});
	TESTER_CHECK(f32x3_from_f32(2.0f) == F32x3{2.0f, 2.0f, 2.0f});

	TESTER_CHECK(f32x3_dot(a, b) == 32.0f);
	TESTER_CHECK(f32x3_cross(TEST_F32_RIGHT, TEST_F32_UP) == TEST_F32_BACKWARD);
	TESTER_CHECK(f32x3_cross(TEST_F32_UP, TEST_F32_RIGHT) == TEST_F32_FORWARD);
}

TESTER_TEST("[MATH][F32x3]: length/normalize/min/max/lerp")
{
	F32x3 v = {2.0f, 3.0f, 6.0f};
	TESTER_CHECK(f32x3_length_squared(v) == 49.0f);
	TESTER_CHECK(f32_approx_equal(f32x3_length(v), 7.0f, 1e-6f));
	TESTER_CHECK(f32_approx_equal(f32x3_length(f32x3_normalize(v)), 1.0f, 1e-6f));
	TESTER_CHECK(f32_approx_equal(f32x3_distance(v, TEST_F32_ZERO), 7.0f, 1e-6f));
	TESTER_CHECK(f32x3_reflect(F32x3{1.0f, -1.0f, 0.0f}, TEST_F32_UP) == F32x3{1.0f, 1.0f, 0.0f});
	TESTER_CHECK(f32x3_project(v, TEST_F32_UP) == F32x3{0.0f, 3.0f, 0.0f});
	TESTER_CHECK(f32x3_reject(v, TEST_F32_UP) == F32x3{2.0f, 0.0f, 6.0f});

	F32x3 a = {3.0f, -1.0f, 10.0f};
	F32x3 b = {1.0f,  5.0f, -2.0f};
	TESTER_CHECK(f32x3_min(a, b) == F32x3{1.0f, -1.0f, -2.0f});
	TESTER_CHECK(f32x3_max(a, b) == F32x3{3.0f,  5.0f, 10.0f});
	TESTER_CHECK(f32x3_lerp(TEST_F32_ZERO, F32x3{10.0f, 20.0f, 30.0f}, 0.5f) == F32x3{5.0f, 10.0f, 15.0f});

	F32x3 clamp_v  = {-2.0f, 5.0f, 7.0f};
	F32x3 clamp_lo = { 0.0f, 0.0f, 0.0f};
	F32x3 clamp_hi = { 3.0f, 3.0f, 6.0f};
	TESTER_CHECK(f32x3_clamp(clamp_v, clamp_lo, clamp_hi) == F32x3{0.0f, 3.0f, 6.0f});
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
	F32x4 designated = {.x = 1.0f, .y = 2.0f, .z = 3.0f, .w = 4.0f};
	TESTER_CHECK(designated == a);
	TESTER_CHECK(a.components[3] == 4.0f);
	TESTER_CHECK(a[2] == 3.0f);
	a[2] = 9.0f;
	TESTER_CHECK(a.z == 9.0f && a.components[2] == 9.0f);
	a.z = 3.0f;

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
	TESTER_CHECK(f32_approx_equal(f32x4_distance(v, TEST_F32_VEC4_ZERO), 4.0f, 1e-5f));
	TESTER_CHECK(f32x4_reflect(F32x4{1.0f, -1.0f, 0.0f, 0.0f}, F32x4{0.0f, 1.0f, 0.0f, 0.0f}) == F32x4{1.0f, 1.0f, 0.0f, 0.0f});
	TESTER_CHECK(f32x4_project(v, F32x4{1.0f, 0.0f, 0.0f, 0.0f}) == F32x4{2.0f, 0.0f, 0.0f, 0.0f});
	TESTER_CHECK(f32x4_reject(v, F32x4{1.0f, 0.0f, 0.0f, 0.0f}) == F32x4{0.0f, 2.0f, 2.0f, 2.0f});
}

TESTER_TEST("[MATH][F32x4]: min / max / lerp / approx_equal / from_f32")
{
	F32x4 a = {3.0f, -1.0f, 10.0f, 0.0f};
	F32x4 b = {1.0f,  5.0f, -2.0f, 8.0f};

	TESTER_CHECK(f32x4_min(a, b) == F32x4{1.0f, -1.0f, -2.0f, 0.0f});
	TESTER_CHECK(f32x4_max(a, b) == F32x4{3.0f,  5.0f, 10.0f, 8.0f});

	TESTER_CHECK(f32x4_lerp(TEST_F32_VEC4_ZERO, TEST_F32_VEC4_ONE, 0.25f) == F32x4{0.25f, 0.25f, 0.25f, 0.25f});
	TESTER_CHECK(f32x4_approx_equal(F32x4{1.0f, 2.0f, 3.0f, 4.0f}, F32x4{1.0f, 2.0f, 3.0f, 4.0f}, 1e-6f));
	TESTER_CHECK(f32x4_from_f32(3.5f) == F32x4{3.5f, 3.5f, 3.5f, 3.5f});
}

// ============================================================================
// Vectors — F64x2 / F64x3 / F64x4
// ============================================================================

TESTER_TEST("[MATH][F64x2]: ops + dot + length")
{
	F64x2 a = {3.0, 4.0};
	F64x2 designated = {.x = 3.0, .y = 4.0};
	TESTER_CHECK(designated == a);
	TESTER_CHECK(a.components[0] == 3.0);
	TESTER_CHECK(a[1] == 4.0);
	a[1] = 9.0;
	TESTER_CHECK(a.y == 9.0 && a.components[1] == 9.0);
	a.y = 4.0;
	TESTER_CHECK(a + F64x2{1.0, 1.0} == F64x2{4.0, 5.0});
	TESTER_CHECK(-a == F64x2{-3.0, -4.0});
	TESTER_CHECK(a * 2.0 == F64x2{6.0, 8.0});
	TESTER_CHECK(f64x2_from_f64(2.5) == F64x2{2.5, 2.5});
	TESTER_CHECK(f64x2_dot(a, F64x2{1.0, 2.0}) == 11.0);
	TESTER_CHECK(f64_approx_equal(f64x2_length(a), 5.0, 1e-12));
	TESTER_CHECK(f64_approx_equal(f64x2_distance(a, TEST_F64_VEC2_ZERO), 5.0, 1e-12));
	TESTER_CHECK(f64x2_reflect(F64x2{1.0, -1.0}, F64x2{0.0, 1.0}) == F64x2{1.0, 1.0});
	TESTER_CHECK(f64x2_project(a, F64x2{1.0, 0.0}) == F64x2{3.0, 0.0});
	TESTER_CHECK(f64x2_reject(a, F64x2{1.0, 0.0}) == F64x2{0.0, 4.0});
	TESTER_CHECK(f64x2_cross(F64x2{1.0, 0.0}, F64x2{0.0, 1.0}) == 1.0);
}

TESTER_TEST("[MATH][F64x3]: ops + cross")
{
	F64x3 a = {1.0, 2.0, 3.0};
	F64x3 b = {4.0, 5.0, 6.0};
	F64x3 designated = {.x = 1.0, .y = 2.0, .z = 3.0};
	TESTER_CHECK(designated == a);
	TESTER_CHECK(a.components[2] == 3.0);
	TESTER_CHECK(a[1] == 2.0);
	a[0] = 7.0;
	TESTER_CHECK(a.x == 7.0 && a.components[0] == 7.0);
	a.x = 1.0;
	TESTER_CHECK(a + b == F64x3{5.0, 7.0, 9.0});
	TESTER_CHECK(f64x3_from_f64(2.0) == F64x3{2.0, 2.0, 2.0});
	TESTER_CHECK(f64x3_dot(a, b) == 32.0);
	TESTER_CHECK(f64x3_cross(TEST_F64_RIGHT, TEST_F64_UP) == TEST_F64_BACKWARD);
	TESTER_CHECK(TEST_F64_FORWARD == F64x3{0.0, 0.0, -1.0});
	TESTER_CHECK(f64_approx_equal(f64x3_distance(F64x3{2.0, 3.0, 6.0}, TEST_F64_ZERO), 7.0, 1e-12));
	TESTER_CHECK(f64x3_reflect(F64x3{1.0, -1.0, 0.0}, TEST_F64_UP) == F64x3{1.0, 1.0, 0.0});
	TESTER_CHECK(f64x3_project(F64x3{2.0, 3.0, 6.0}, TEST_F64_UP) == F64x3{0.0, 3.0, 0.0});
	TESTER_CHECK(f64x3_reject(F64x3{2.0, 3.0, 6.0}, TEST_F64_UP) == F64x3{2.0, 0.0, 6.0});

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
	F64x4 designated = {.x = 1.0, .y = 2.0, .z = 3.0, .w = 4.0};
	TESTER_CHECK(designated == a);
	TESTER_CHECK(a.components[3] == 4.0);
	TESTER_CHECK(a[2] == 3.0);
	a[2] = 9.0;
	TESTER_CHECK(a.z == 9.0 && a.components[2] == 9.0);
	a.z = 3.0;
	TESTER_CHECK(a + b == F64x4{6.0, 8.0, 10.0, 12.0});
	TESTER_CHECK(b - a == F64x4{4.0, 4.0,  4.0,  4.0});
	TESTER_CHECK(a * 2.0 == F64x4{2.0, 4.0, 6.0, 8.0});
	TESTER_CHECK(f64_approx_equal(f64x4_dot(a, b), 70.0, 1e-10));

	F64x4 v = {2.0, 2.0, 2.0, 2.0};
	TESTER_CHECK(f64_approx_equal(f64x4_length(v), 4.0, 1e-10));
	TESTER_CHECK(f64_approx_equal(f64x4_length(f64x4_normalize(v)), 1.0, 1e-10));
	TESTER_CHECK(f64_approx_equal(f64x4_distance(v, TEST_F64_VEC4_ZERO), 4.0, 1e-10));
	TESTER_CHECK(f64x4_reflect(F64x4{1.0, -1.0, 0.0, 0.0}, F64x4{0.0, 1.0, 0.0, 0.0}) == F64x4{1.0, 1.0, 0.0, 0.0});
	TESTER_CHECK(f64x4_project(v, F64x4{1.0, 0.0, 0.0, 0.0}) == F64x4{2.0, 0.0, 0.0, 0.0});
	TESTER_CHECK(f64x4_reject(v, F64x4{1.0, 0.0, 0.0, 0.0}) == F64x4{0.0, 2.0, 2.0, 2.0});

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
	I32x2 designated = {.x = 3, .y = -4};
	TESTER_CHECK(designated == a);
	TESTER_CHECK(a.components[0] == 3);
	TESTER_CHECK(a[1] == -4);
	a[1] = 9;
	TESTER_CHECK(a.y == 9 && a.components[1] == 9);
	a.y = -4;
	TESTER_CHECK(a + b == I32x2{4, -2});
	TESTER_CHECK(a - b == I32x2{2, -6});
	TESTER_CHECK(-a   == I32x2{-3, 4});
	TESTER_CHECK(a * 2 == I32x2{6, -8});
	TESTER_CHECK(i32x2_dot(a, b) == -5);
	TESTER_CHECK(f32_approx_equal(i32x2_length(a), 5.0f, 1e-6f));
	TESTER_CHECK(i32x2_length_squared(a) == 25);
	TESTER_CHECK(f32_approx_equal(i32x2_distance(I32x2{1, 2}, I32x2{4, 6}), 5.0f, 1e-6f));
	TESTER_CHECK(i32x2_distance_squared(I32x2{1, 2}, I32x2{4, 6}) == 25);
	TESTER_CHECK(i32x2_abs(a) == I32x2{3, 4});
	TESTER_CHECK(i32x2_min(a, b) == I32x2{1, -4});
	TESTER_CHECK(i32x2_max(a, b) == I32x2{3,  2});
	TESTER_CHECK(i32x2_clamp(I32x2{-5, 10}, I32x2{0, 0}, I32x2{4, 4}) == I32x2{0, 4});
	TESTER_CHECK(i32x2_from_i32(7) == I32x2{7, 7});
}

TESTER_TEST("[MATH][I32x3]: basic ops")
{
	I32x3 a = {1, 2, 3};
	I32x3 b = {4, 5, 6};
	I32x3 designated = {.x = 1, .y = 2, .z = 3};
	TESTER_CHECK(designated == a);
	TESTER_CHECK(a.components[2] == 3);
	TESTER_CHECK(a[1] == 2);
	a[0] = 7;
	TESTER_CHECK(a.x == 7 && a.components[0] == 7);
	a.x = 1;
	TESTER_CHECK(a + b == I32x3{5, 7, 9});
	TESTER_CHECK(i32x3_dot(a, b) == 32);
	TESTER_CHECK(f32_approx_equal(i32x3_length(I32x3{2, 3, 6}), 7.0f, 1e-6f));
	TESTER_CHECK(i32x3_length_squared(a) == 14);
	TESTER_CHECK(f32_approx_equal(i32x3_distance(I32x3{1, 1, 1}, I32x3{3, 4, 7}), 7.0f, 1e-6f));
	TESTER_CHECK(i32x3_distance_squared(I32x3{1, 1, 1}, I32x3{3, 4, 7}) == 49);
	TESTER_CHECK(i32x3_abs(I32x3{-1, -2, -3}) == I32x3{1, 2, 3});
	TESTER_CHECK(i32x3_clamp(I32x3{-1, 5, 10}, I32x3{0, 0, 0}, I32x3{3, 3, 3}) == I32x3{0, 3, 3});
	TESTER_CHECK(i32x3_from_i32(7) == I32x3{7, 7, 7});
}

TESTER_TEST("[MATH][I32x4]: SIMD ops + alignment")
{
	TESTER_CHECK(sizeof(I32x4)  == 16);
	TESTER_CHECK(alignof(I32x4) == 16);

	I32x4 a = {1, 2, 3, 4};
	I32x4 b = {5, 6, 7, 8};
	I32x4 designated = {.x = 1, .y = 2, .z = 3, .w = 4};
	TESTER_CHECK(designated == a);
	TESTER_CHECK(a.components[3] == 4);
	TESTER_CHECK(a[2] == 3);
	a[2] = 9;
	TESTER_CHECK(a.z == 9 && a.components[2] == 9);
	a.z = 3;
	TESTER_CHECK(a + b == I32x4{6, 8, 10, 12});
	TESTER_CHECK(b - a == I32x4{4, 4,  4,  4});
	TESTER_CHECK(-a    == I32x4{-1, -2, -3, -4});
	TESTER_CHECK(a * 3 == I32x4{3, 6, 9, 12});
	TESTER_CHECK(i32x4_dot(a, b) == 70);
	TESTER_CHECK(f32_approx_equal(i32x4_length(I32x4{2, 2, 4, 5}), 7.0f, 1e-6f));
	TESTER_CHECK(i32x4_length_squared(a) == 30);
	TESTER_CHECK(f32_approx_equal(i32x4_distance(I32x4{1, 1, 1, 1}, I32x4{3, 3, 5, 6}), 7.0f, 1e-6f));
	TESTER_CHECK(i32x4_distance_squared(I32x4{1, 1, 1, 1}, I32x4{3, 3, 5, 6}) == 49);
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
	U32x2 designated = {.x = 3u, .y = 4u};
	TESTER_CHECK(designated == a);
	TESTER_CHECK(a.components[0] == 3u);
	TESTER_CHECK(a[1] == 4u);
	a[1] = 9u;
	TESTER_CHECK(a.y == 9u && a.components[1] == 9u);
	a.y = 4u;
	TESTER_CHECK(a + b == U32x2{4u, 6u});
	TESTER_CHECK(a - b == U32x2{2u, 2u});
	TESTER_CHECK(a * 2u == U32x2{6u, 8u});
	TESTER_CHECK(u32x2_dot(a, b) == 11u);
	TESTER_CHECK(f32_approx_equal(u32x2_length(a), 5.0f, 1e-6f));
	TESTER_CHECK(u32x2_length_squared(a) == 25u);
	TESTER_CHECK(u32x2_min(a, b) == U32x2{1u, 2u});
	TESTER_CHECK(u32x2_max(a, b) == U32x2{3u, 4u});
	TESTER_CHECK(u32x2_clamp(U32x2{10u, 5u}, U32x2{0u, 0u}, U32x2{4u, 4u}) == U32x2{4u, 4u});
	TESTER_CHECK(u32x2_from_u32(5u) == U32x2{5u, 5u});
}

TESTER_TEST("[MATH][U32x3]: basic ops")
{
	U32x3 a = {1u, 2u, 3u};
	U32x3 b = {4u, 5u, 6u};
	U32x3 designated = {.x = 1u, .y = 2u, .z = 3u};
	TESTER_CHECK(designated == a);
	TESTER_CHECK(a.components[2] == 3u);
	TESTER_CHECK(a[1] == 2u);
	a[0] = 7u;
	TESTER_CHECK(a.x == 7u && a.components[0] == 7u);
	a.x = 1u;
	TESTER_CHECK(a + b == U32x3{5u, 7u, 9u});
	TESTER_CHECK(u32x3_dot(a, b) == 32u);
	TESTER_CHECK(f32_approx_equal(u32x3_length(U32x3{2u, 3u, 6u}), 7.0f, 1e-6f));
	TESTER_CHECK(u32x3_length_squared(a) == 14u);
	TESTER_CHECK(u32x3_clamp(U32x3{10u, 2u, 7u}, U32x3{0u, 0u, 0u}, U32x3{5u, 5u, 5u}) == U32x3{5u, 2u, 5u});
	TESTER_CHECK(u32x3_from_u32(5u) == U32x3{5u, 5u, 5u});
}

TESTER_TEST("[MATH][U32x4]: SIMD ops + alignment")
{
	TESTER_CHECK(sizeof(U32x4)  == 16);
	TESTER_CHECK(alignof(U32x4) == 16);

	U32x4 a = {1u, 2u, 3u, 4u};
	U32x4 b = {5u, 6u, 7u, 8u};
	U32x4 designated = {.x = 1u, .y = 2u, .z = 3u, .w = 4u};
	TESTER_CHECK(designated == a);
	TESTER_CHECK(a.components[3] == 4u);
	TESTER_CHECK(a[2] == 3u);
	a[2] = 9u;
	TESTER_CHECK(a.z == 9u && a.components[2] == 9u);
	a.z = 3u;
	TESTER_CHECK(a + b == U32x4{6u, 8u, 10u, 12u});
	TESTER_CHECK(b - a == U32x4{4u, 4u,  4u,  4u});
	TESTER_CHECK(a * 3u == U32x4{3u, 6u, 9u, 12u});
	TESTER_CHECK(u32x4_dot(a, b) == 70u);
	TESTER_CHECK(f32_approx_equal(u32x4_length(U32x4{2u, 2u, 4u, 5u}), 7.0f, 1e-6f));
	TESTER_CHECK(u32x4_length_squared(a) == 30u);
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
	const F32x2x2 &A_const = A;
	TESTER_CHECK(A_const[0] == F32x2{1.0f, 2.0f});
	TESTER_CHECK(A[1] == F32x2{3.0f, 4.0f});
	TESTER_CHECK(A_const[0][1] == 2.0f);
	A[1][0] = 9.0f;
	TESTER_CHECK(A.m10 == 9.0f);
	A[1] = F32x2{3.0f, 4.0f};
	TESTER_CHECK(A + B == F32x2x2{6.0f, 8.0f, 10.0f, 12.0f});
	TESTER_CHECK(A * 2.0f == F32x2x2{2.0f, 4.0f, 6.0f, 8.0f});
	TESTER_CHECK(f32x2x2_transpose(A) == F32x2x2{1.0f, 3.0f, 2.0f, 4.0f});
	TESTER_CHECK(f32x2x2_determinant(A) == -2.0f);

	F32x2x2 designated = {
		.m00 = 1.0f, .m01 = 2.0f,
		.m10 = 3.0f, .m11 = 4.0f
	};
	TESTER_CHECK(designated[1][1] == 4.0f);

	F32x2x2 Ainv = f32x2x2_inverse(A);
	F32x2x2 P    = A * Ainv;
	TESTER_CHECK(f32_approx_equal(P.m00, 1.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(P.m01, 0.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(P.m10, 0.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(P.m11, 1.0f, 1e-5f));
}

TESTER_TEST("[MATH][F32x3x3]: layout + identity + mul + inverse")
{
	TESTER_CHECK(sizeof(F32x3x3)  == 36);
	TESTER_CHECK(alignof(F32x3x3) == 4);

	F32x3x3 I = f32x3x3_identity();
	TESTER_CHECK(I.m00 == 1.0f && I.m11 == 1.0f && I.m22 == 1.0f);
	TESTER_CHECK(I.m01 == 0.0f && I.m10 == 0.0f);

	F32x3 v = {1.0f, 2.0f, 3.0f};
	TESTER_CHECK(v * I == v);

	F32x3x3 row_values = {
		1.0f, 2.0f, 3.0f,
		4.0f, 5.0f, 6.0f,
		7.0f, 8.0f, 9.0f
	};
	const F32x3x3 &row_values_const = row_values;
	TESTER_CHECK(row_values_const[0] == F32x3{1.0f, 2.0f, 3.0f});
	TESTER_CHECK(row_values[1] == F32x3{4.0f, 5.0f, 6.0f});
	TESTER_CHECK(row_values[2] == F32x3{7.0f, 8.0f, 9.0f});
	TESTER_CHECK(row_values_const[2][1] == 8.0f);
	row_values[1][2] = 42.0f;
	TESTER_CHECK(row_values.m12 == 42.0f);
	row_values[1] = F32x3{4.0f, 5.0f, 6.0f};
	TESTER_CHECK(f32x3x3_approx_equal(row_values, row_values, 0.0f));

	F32x3x3 designated = {
		.m00 = 1.0f, .m01 = 2.0f, .m02 = 3.0f,
		.m10 = 4.0f, .m11 = 5.0f, .m12 = 6.0f,
		.m20 = 7.0f, .m21 = 8.0f, .m22 = 9.0f
	};
	TESTER_CHECK(designated[2][1] == 8.0f);
	designated[0] = F32x3{10.0f, 11.0f, 12.0f};
	TESTER_CHECK(designated.m00 == 10.0f && designated.m02 == 12.0f);

	F32x3x3 A = {
		1.0f, 2.0f, 3.0f,
		0.0f, 1.0f, 4.0f,
		5.0f, 6.0f, 0.0f
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
	TESTER_CHECK(I[0] == F32x4{1.0f, 0.0f, 0.0f, 0.0f});
	TESTER_CHECK(I[3] == F32x4{0.0f, 0.0f, 0.0f, 1.0f});
	TESTER_CHECK(I[0][0] == 1.0f);
	TESTER_CHECK(I[3][3] == 1.0f);
	TESTER_CHECK(v * I == v);
	TESTER_CHECK(I * I == I);

	F32x4x4 designated = {
		.m00 = 1.0f,  .m01 = 2.0f,  .m02 = 3.0f,  .m03 = 4.0f,
		.m10 = 5.0f,  .m11 = 6.0f,  .m12 = 7.0f,  .m13 = 8.0f,
		.m20 = 9.0f,  .m21 = 10.0f, .m22 = 11.0f, .m23 = 12.0f,
		.m30 = 13.0f, .m31 = 14.0f, .m32 = 15.0f, .m33 = 16.0f
	};
	TESTER_CHECK(designated[2][1] == 10.0f);
	TESTER_CHECK(designated.rows[3][3] == 16.0f);

	F32x4x4 indexed = I;
	indexed[3][2] = 5.0f;
	TESTER_CHECK(indexed.m32 == 5.0f);
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
	F32x4 rotated = x_axis * f32x4x4_rotation_y(F32_PI * 0.5f);
	TESTER_CHECK(f32_approx_equal(rotated.x, 0.0f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(rotated.z, -1.0f, 1e-5f));

	F32x3 T3 = {5.0f, 6.0f, 7.0f};
	Quaternion R3 = quaternion_from_axis_angle(TEST_F32_UP, F32_PI * 0.5f);
	F32x3 S3 = {2.0f, 3.0f, 4.0f};
	F32x4x4 M = f32x4x4_from_trs(T3, R3, S3);
	TESTER_CHECK(f32x4x4_approx_equal(M, f32x4x4_scaling(S3) * f32x4x4_from_quaternion(R3) * f32x4x4_translation(T3), 1e-6f));
	TESTER_CHECK(f32x3_approx_equal(f32x4x4_transform_point(M, TEST_F32_RIGHT), F32x3{5.0f, 6.0f, 5.0f}, 1e-5f));
	TESTER_CHECK(f32x3_approx_equal(f32x4x4_transform_vector(M, TEST_F32_RIGHT), F32x3{0.0f, 0.0f, -2.0f}, 1e-5f));
	TESTER_CHECK(f32x3_approx_equal(f32x4x4_transform_normal(M, TEST_F32_UP), TEST_F32_UP, 1e-5f));
	TESTER_CHECK(f32x4x4_approx_equal(M * f32x4x4_affine_inverse(M), f32x4x4_identity(), 1e-4f));
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
	F32x4x4 view = f32x4x4_look_at(F32x3{0.0f, 0.0f, 5.0f}, TEST_F32_ZERO, TEST_F32_UP);
	F32x4   origin = {0.0f, 0.0f, 0.0f, 1.0f};
	F32x4   view_pos = origin * view;
	// Origin is 5 units along -Z in view space.
	TESTER_CHECK(f32_approx_equal(view_pos.z, -5.0f, 1e-4f));
	TESTER_CHECK(f32_approx_equal(view_pos.x,  0.0f, 1e-4f));
	TESTER_CHECK(f32_approx_equal(view_pos.y,  0.0f, 1e-4f));
}

TESTER_TEST("[MATH][F32x4x4]: perspective canonical NDC")
{
	F32x4x4 P = f32x4x4_perspective(F32_PI * 0.5f, 1.0f, 0.1f, 100.0f);

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
	F32x4x4 view    = f32x4x4_look_at(F32x3{0.0f, 0.0f, 5.0f}, TEST_F32_ZERO, TEST_F32_UP);
	F32x4x4 proj    = f32x4x4_perspective(F32_PI * 0.5f, 16.0f / 9.0f, 0.1f, 100.0f);
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
	TESTER_CHECK(I[0] == F64x4{1.0, 0.0, 0.0, 0.0});
	TESTER_CHECK(I[3] == F64x4{0.0, 0.0, 0.0, 1.0});
	TESTER_CHECK(I[0][0] == 1.0);
	TESTER_CHECK(I[3][3] == 1.0);
	TESTER_CHECK(v * I == v);
	TESTER_CHECK(I * I == I);

	F64x4x4 designated = {
		.m00 = 1.0,  .m01 = 2.0,  .m02 = 3.0,  .m03 = 4.0,
		.m10 = 5.0,  .m11 = 6.0,  .m12 = 7.0,  .m13 = 8.0,
		.m20 = 9.0,  .m21 = 10.0, .m22 = 11.0, .m23 = 12.0,
		.m30 = 13.0, .m31 = 14.0, .m32 = 15.0, .m33 = 16.0
	};
	TESTER_CHECK(designated[2][1] == 10.0);
	TESTER_CHECK(designated.rows[3][3] == 16.0);

	F64x4x4 indexed = I;
	indexed[3][2] = 5.0;
	TESTER_CHECK(indexed.m32 == 5.0);

	F64x4x4 A = {
		1.0, 2.0, 3.0, 4.0,
		5.0, 6.0, 7.0, 8.0,
		9.0, 10.0, 11.0, 12.0,
		13.0, 14.0, 15.0, 16.0
	};
	TESTER_CHECK(A * f64x4x4_identity() == A);

	F64x4x4 M = f64x4x4_scaling(F64x3{2.0, 3.0, 4.0}) * f64x4x4_rotation_y(F64_PI * 0.5) * f64x4x4_translation(F64x3{5.0, 6.0, 7.0});
	TESTER_CHECK(f64x3_approx_equal(f64x4x4_transform_point(M, TEST_F64_RIGHT), F64x3{5.0, 6.0, 5.0}, 1e-10));
	TESTER_CHECK(f64x3_approx_equal(f64x4x4_transform_vector(M, TEST_F64_RIGHT), F64x3{0.0, 0.0, -2.0}, 1e-10));
	TESTER_CHECK(f64x3_approx_equal(f64x4x4_transform_normal(M, TEST_F64_UP), TEST_F64_UP, 1e-10));
	TESTER_CHECK(f64x4x4_approx_equal(M * f64x4x4_affine_inverse(M), f64x4x4_identity(), 1e-10));

	F64x4x4 view = f64x4x4_look_at(F64x3{0.0, 0.0, 5.0}, TEST_F64_ZERO, TEST_F64_UP);
	F64x4 view_pos = F64x4{0.0, 0.0, 0.0, 1.0} * view;
	TESTER_CHECK(f64_approx_equal(view_pos.z, -5.0, 1e-10));
	F64x4 view_sample = F64x4{1.0, 2.0, 0.0, 1.0} * view;
	TESTER_CHECK(f64_approx_equal(view_sample.w, 1.0, 1e-10));

	F64x4x4 P = f64x4x4_perspective(F64_PI * 0.5, 1.0, 0.1, 100.0);
	F64x4 near_pt = F64x4{0.0, 0.0, -0.1, 1.0} * P;
	TESTER_CHECK(f64_approx_equal(near_pt.z / near_pt.w, 0.0, 1e-10));

	F64x4x4 O = f64x4x4_orthographic(-1.0, 1.0, -1.0, 1.0, 0.0, 1.0);
	F64x4 near_ll = F64x4{-1.0, -1.0, 0.0, 1.0} * O;
	TESTER_CHECK(f64_approx_equal(near_ll.x, -1.0, 1e-10));
	TESTER_CHECK(f64_approx_equal(near_ll.y, -1.0, 1e-10));
	TESTER_CHECK(f64_approx_equal(near_ll.z,  0.0, 1e-10));
	F64x4 far_ll = F64x4{-1.0, -1.0, -1.0, 1.0} * O;
	TESTER_CHECK(f64_approx_equal(far_ll.z,  1.0, 1e-10));
}

TESTER_TEST("[MATH][F64x3x3]: layout + identity + inverse")
{
	TESTER_CHECK(sizeof(F64x3x3)  == 72);
	TESTER_CHECK(alignof(F64x3x3) == 8);

	F64x3x3 row_values = {
		1.0, 2.0, 3.0,
		4.0, 5.0, 6.0,
		7.0, 8.0, 9.0
	};
	const F64x3x3 &row_values_const = row_values;
	TESTER_CHECK(row_values_const[0] == F64x3{1.0, 2.0, 3.0});
	TESTER_CHECK(row_values[1] == F64x3{4.0, 5.0, 6.0});
	TESTER_CHECK(row_values[2] == F64x3{7.0, 8.0, 9.0});
	TESTER_CHECK(row_values_const[2][1] == 8.0);
	row_values[1][2] = 42.0;
	TESTER_CHECK(row_values.m12 == 42.0);
	row_values[1] = F64x3{4.0, 5.0, 6.0};
	TESTER_CHECK(f64x3x3_approx_equal(row_values, row_values, 0.0));

	F64x3x3 designated = {
		.m00 = 1.0, .m01 = 2.0, .m02 = 3.0,
		.m10 = 4.0, .m11 = 5.0, .m12 = 6.0,
		.m20 = 7.0, .m21 = 8.0, .m22 = 9.0
	};
	TESTER_CHECK(designated[2][1] == 8.0);
	designated[0] = F64x3{10.0, 11.0, 12.0};
	TESTER_CHECK(designated.m00 == 10.0 && designated.m02 == 12.0);

	F64x3x3 A = {
		1.0, 2.0, 3.0,
		0.0, 1.0, 4.0,
		5.0, 6.0, 0.0
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
	TESTER_CHECK(I[0] == F64x2{1.0, 0.0});
	TESTER_CHECK(I[1] == F64x2{0.0, 1.0});
	TESTER_CHECK(I[0][0] == 1.0);
	I[1][0] = 7.0;
	TESTER_CHECK(I.m10 == 7.0);
	I[1] = F64x2{0.0, 1.0};
	TESTER_CHECK(v * I == v);
	F64x2x2 designated = {
		.m00 = 1.0, .m01 = 2.0,
		.m10 = 3.0, .m11 = 4.0
	};
	TESTER_CHECK(designated[1][1] == 4.0);
	TESTER_CHECK(f64x2x2_determinant(F64x2x2{1.0, 2.0, 3.0, 4.0}) == -2.0);
}

// ============================================================================
// Quaternion
// ============================================================================

TESTER_TEST("[MATH][Quaternion]: identity + operators")
{
	Quaternion I = quaternion_identity();
	TESTER_CHECK(I == TEST_QUAT_IDENTITY);
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
	TESTER_CHECK(quaternion_approx_equal(quaternion_from_axis_angle(TEST_F32_UP, 0.0f), TEST_QUAT_IDENTITY, 1e-6f));

	// 180° about X: (0, 1, 0, 0).
	Quaternion r = quaternion_from_axis_angle(TEST_F32_RIGHT, F32_PI);
	TESTER_CHECK(f32_approx_equal(r.w, 0.0f, 1e-6f));
	TESTER_CHECK(f32_approx_equal(r.x, 1.0f, 1e-6f));

	// All unit-axis rotations produce unit quaternions.
	for (F32 theta : {0.5f, 1.0f, 1.5f, 2.0f, 3.0f})
	{
		Quaternion q = quaternion_from_axis_angle(TEST_F32_UP, theta);
		TESTER_CHECK(f32_approx_equal(quaternion_length(q), 1.0f, 1e-6f));
	}
}

TESTER_TEST("[MATH][Quaternion]: inverse round-trip")
{
	Quaternion q = quaternion_normalize(Quaternion{1.0f, 2.0f, 3.0f, 4.0f});
	Quaternion inv = quaternion_inverse(q);
	Quaternion p = q * inv;
	TESTER_CHECK(quaternion_approx_equal(p, TEST_QUAT_IDENTITY, 1e-5f));
}

TESTER_TEST("[MATH][Quaternion]: conjugate / angle / axis-angle / nlerp")
{
	Quaternion q = quaternion_from_axis_angle(TEST_F32_UP, F32_PI * 0.5f);
	TESTER_CHECK(quaternion_approx_equal(quaternion_conjugate(q), quaternion_inverse(q), 1e-6f));
	TESTER_CHECK(f32_approx_equal(quaternion_angle(q), F32_PI * 0.5f, 1e-5f));
	TESTER_CHECK(f32_approx_equal(quaternion_angle(TEST_QUAT_IDENTITY, q), F32_PI * 0.5f, 1e-5f));

	F32x3 axis;
	F32 angle;
	quaternion_to_axis_angle(q, &axis, &angle);
	TESTER_CHECK(f32x3_approx_equal(axis, TEST_F32_UP, 1e-5f));
	TESTER_CHECK(f32_approx_equal(angle, F32_PI * 0.5f, 1e-5f));

	Quaternion mid = quaternion_nlerp(TEST_QUAT_IDENTITY, q, 0.5f);
	Quaternion expected_mid = quaternion_from_axis_angle(TEST_F32_UP, F32_PI * 0.5f * 0.5f);
	TESTER_CHECK(quaternion_rotation_approx_equal(mid, expected_mid, 1e-5f));
	TESTER_CHECK(!quaternion_approx_equal(q, -q, 1e-6f));
	TESTER_CHECK(quaternion_rotation_approx_equal(q, -q, 1e-6f));
}

TESTER_TEST("[MATH][Quaternion]: vector rotation by quaternion")
{
	// 90° about +Y: rotates +X to -Z (right-handed).
	Quaternion q = quaternion_from_axis_angle(TEST_F32_UP, F32_PI * 0.5f);
	F32x3 rotated = TEST_F32_RIGHT * q;
	TESTER_CHECK(f32x3_approx_equal(rotated, TEST_F32_FORWARD, 1e-5f));

	// Rotating +Y about +Y leaves it unchanged.
	F32x3 same = TEST_F32_UP * q;
	TESTER_CHECK(f32x3_approx_equal(same, TEST_F32_UP, 1e-5f));

	// Identity rotation leaves vectors unchanged.
	TESTER_CHECK(f32x3_approx_equal(TEST_F32_RIGHT * TEST_QUAT_IDENTITY, TEST_F32_RIGHT, 1e-6f));
}

TESTER_TEST("[MATH][Quaternion]: from_euler_angles is right-handed")
{
	// Euler (pitch=0, yaw=pi/2, roll=0): rotates +X to -Z (right-handed about +Y).
	// This is the test that used to fail under the old left-handed implementation.
	Quaternion q = quaternion_from_euler_angles(F32x3{0.0f, F32_PI * 0.5f, 0.0f});
	F32x3 rotated = TEST_F32_RIGHT * q;
	TESTER_CHECK(f32x3_approx_equal(rotated, TEST_F32_FORWARD, 1e-5f));

	// Round-trip through to_euler_angles.
	F32x3 angles = F32x3{0.3f, 0.5f, -0.7f};
	Quaternion q2 = quaternion_from_euler_angles(angles);
	F32x3 recovered = quaternion_to_euler_angles(q2);
	TESTER_CHECK(f32x3_approx_equal(recovered, angles, 1e-4f));
}

TESTER_TEST("[MATH][Quaternion]: slerp endpoints + midpoint")
{
	Quaternion a = quaternion_from_axis_angle(TEST_F32_UP, 0.0f);
	Quaternion b = quaternion_from_axis_angle(TEST_F32_UP, F32_PI * 0.5f);

	TESTER_CHECK(quaternion_approx_equal(quaternion_slerp(a, b, 0.0f), a, 1e-5f));
	TESTER_CHECK(quaternion_approx_equal(quaternion_slerp(a, b, 1.0f), b, 1e-5f));

	// Midpoint is 45° about +Y.
	Quaternion mid = quaternion_slerp(a, b, 0.5f);
	Quaternion expected = quaternion_from_axis_angle(TEST_F32_UP, F32_PI * 0.5f * 0.5f);
	TESTER_CHECK(quaternion_approx_equal(mid, expected, 1e-5f));
}

TESTER_TEST("[MATH][Quaternion]: from_to_rotation")
{
	// +X to +Y: 90° about +Z.
	Quaternion q = quaternion_from_to_rotation(TEST_F32_RIGHT, TEST_F32_UP);
	F32x3 rotated = TEST_F32_RIGHT * q;
	TESTER_CHECK(f32x3_approx_equal(rotated, TEST_F32_UP, 1e-5f));

	// Identity case: +X to +X.
	Quaternion ident = quaternion_from_to_rotation(TEST_F32_RIGHT, TEST_F32_RIGHT);
	TESTER_CHECK(quaternion_approx_equal(ident, TEST_QUAT_IDENTITY, 1e-4f));

	// Anti-parallel: +X to -X. Rotates 180° about some perpendicular axis.
	Quaternion flip = quaternion_from_to_rotation(TEST_F32_RIGHT, TEST_F32_LEFT);
	F32x3 flipped = TEST_F32_RIGHT * flip;
	TESTER_CHECK(f32x3_approx_equal(flipped, TEST_F32_LEFT, 1e-4f));
}

TESTER_TEST("[MATH][Quaternion]: look_rotation")
{
	// Looking down -Z with +Y up → identity orientation.
	Quaternion q = quaternion_look_rotation(TEST_F32_FORWARD, TEST_F32_UP);
	TESTER_CHECK(quaternion_approx_equal(q, TEST_QUAT_IDENTITY, 1e-4f));

	// Looking down +X with +Y up → 90° about +Y.
	Quaternion q2 = quaternion_look_rotation(TEST_F32_RIGHT, TEST_F32_UP);
	F32x3 forward_of_q2 = TEST_F32_FORWARD * q2;
	TESTER_CHECK(f32x3_approx_equal(forward_of_q2, TEST_F32_RIGHT, 1e-4f));
}

TESTER_TEST("[MATH][Quaternion]: rotate_towards clamps step")
{
	Quaternion a = quaternion_identity();
	// 120° about UP — unambiguous shortest arc (180° has ambiguous direction).
	Quaternion b = quaternion_from_axis_angle(TEST_F32_UP, F32_PI * 2.0f / 3.0f);
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
	F32x3 via_half     = TEST_F32_RIGHT * half;
	F32x3 via_expected = TEST_F32_RIGHT * quaternion_from_axis_angle(TEST_F32_UP, full_angle * 0.5f);
	TESTER_CHECK(f32x3_approx_equal(via_half, via_expected, 1e-4f));
}

TESTER_TEST("[MATH][Quaternion]: f32x4x4_from_quaternion agrees with vector rotation")
{
	Quaternion q = quaternion_from_axis_angle(TEST_F32_UP, F32_PI * 0.5f);
	F32x4x4 M   = f32x4x4_from_quaternion(q);

	// Rotating +X via matrix and via quaternion should match.
	F32x3 via_quat   = TEST_F32_RIGHT * q;
	F32x4 via_matrix = F32x4{1.0f, 0.0f, 0.0f, 0.0f} * M;

	TESTER_CHECK(f32_approx_equal(via_quat.x, via_matrix.x, 1e-5f));
	TESTER_CHECK(f32_approx_equal(via_quat.y, via_matrix.y, 1e-5f));
	TESTER_CHECK(f32_approx_equal(via_quat.z, via_matrix.z, 1e-5f));

	Quaternion back = quaternion_from_f32x4x4(M);
	TESTER_CHECK(quaternion_rotation_approx_equal(q, back, 1e-5f));
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
	TESTER_CHECK(f32x4x4_approx_equal(prod_L, I, 1e-4f));
	TESTER_CHECK(f32x4x4_approx_equal(prod_R, I, 1e-4f));

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

	U64 expected[] = {
		0x15780b2e0c2ec716ull,
		0x6104d9866d113a7eull,
		0xae17533239e499a1ull,
		0xecb8ad4703b360a1ull,
		0xfde6dc7fe2ec5e64ull
	};
	Random golden = random_from_seed(42);
	for (U32 i = 0; i < count_of(expected); ++i)
		TESTER_CHECK(random_u64(golden) == expected[i]);

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

TESTER_TEST("[MATH][Random]: stream determinism")
{
	Random base = random_from_seed(123);
	Random stream0 = random_from_seed_and_stream(123, 0);
	for (int i = 0; i < 16; ++i)
		TESTER_CHECK(random_u64(base) == random_u64(stream0));

	Random a = random_from_seed_and_stream(123, 7);
	Random b = random_from_seed_and_stream(123, 7);
	for (int i = 0; i < 16; ++i)
		TESTER_CHECK(random_u64(a) == random_u64(b));

	Random c = random_from_seed_and_stream(123, 7);
	Random d = random_from_seed_and_stream(123, 8);
	bool any_diff = false;
	for (int i = 0; i < 16; ++i)
		if (random_u64(c) != random_u64(d)) { any_diff = true; break; }
	TESTER_CHECK(any_diff);
}

TESTER_TEST("[MATH][Random]: random_f32_unit in [0, 1)")
{
	Random rng;
	random_seed(rng, 12345);
	for (int i = 0; i < 10000; ++i)
	{
		F32 x = random_f32_unit(rng);
		TESTER_CHECK(x >= 0.0f);
		TESTER_CHECK(x <  1.0f);
	}
}

TESTER_TEST("[MATH][Random]: random_f32_range bounds")
{
	Random rng;
	random_seed(rng, 0xABCDEF);
	for (int i = 0; i < 10000; ++i)
	{
		F32 x = random_f32_range(rng, -5.0f, 5.0f);
		TESTER_CHECK(x >= -5.0f);
		TESTER_CHECK(x <   5.0f);
	}
}

TESTER_TEST("[MATH][Random]: random_i32_range inclusive bounds")
{
	Random rng;
	random_seed(rng, 777);
	I32 observed_min =  1000;
	I32 observed_max = -1000;
	for (int i = 0; i < 10000; ++i)
	{
		I32 v = random_i32_range(rng, -3, 5);
		TESTER_CHECK(v >= -3);
		TESTER_CHECK(v <=  5);
		if (v < observed_min) observed_min = v;
		if (v > observed_max) observed_max = v;
	}
	// With 10000 draws across 9 bins we should hit both endpoints.
	TESTER_CHECK(observed_min == -3);
	TESTER_CHECK(observed_max ==  5);
}

TESTER_TEST("[MATH][Random]: u32/i64/u64 random ranges inclusive bounds")
{
	Random rng = random_from_seed(778);

	U32 observed_u32_min = 1000u;
	U32 observed_u32_max = 0u;
	for (int i = 0; i < 10000; ++i)
	{
		U32 v = random_u32_range(rng, 7u, 11u);
		TESTER_CHECK(v >= 7u);
		TESTER_CHECK(v <= 11u);
		if (v < observed_u32_min) observed_u32_min = v;
		if (v > observed_u32_max) observed_u32_max = v;
	}
	TESTER_CHECK(observed_u32_min == 7u);
	TESTER_CHECK(observed_u32_max == 11u);

	I64 observed_i64_min = 1000ll;
	I64 observed_i64_max = -1000ll;
	for (int i = 0; i < 10000; ++i)
	{
		I64 v = random_i64_range(rng, -4ll, 4ll);
		TESTER_CHECK(v >= -4ll);
		TESTER_CHECK(v <=  4ll);
		if (v < observed_i64_min) observed_i64_min = v;
		if (v > observed_i64_max) observed_i64_max = v;
	}
	TESTER_CHECK(observed_i64_min == -4ll);
	TESTER_CHECK(observed_i64_max ==  4ll);

	U64 observed_u64_min = 1000ull;
	U64 observed_u64_max = 0ull;
	for (int i = 0; i < 10000; ++i)
	{
		U64 v = random_u64_range(rng, 9ull, 13ull);
		TESTER_CHECK(v >= 9ull);
		TESTER_CHECK(v <= 13ull);
		if (v < observed_u64_min) observed_u64_min = v;
		if (v > observed_u64_max) observed_u64_max = v;
	}
	TESTER_CHECK(observed_u64_min == 9ull);
	TESTER_CHECK(observed_u64_max == 13ull);

	TESTER_CHECK(random_u64_range(rng, 42ull, 42ull) == 42ull);
	U64 any_u64 = random_u64_range(rng, 0ull, U64_MAX);
	TESTER_CHECK(any_u64 <= U64_MAX);
	I64 any_i64 = random_i64_range(rng, I64_MIN, I64_MAX);
	TESTER_CHECK(any_i64 >= I64_MIN);
	TESTER_CHECK(any_i64 <= I64_MAX);
}

TESTER_TEST("[MATH][Random]: random_f32x3_in_unit_sphere / on_unit_sphere")
{
	Random rng;
	random_seed(rng, 99);

	for (int i = 0; i < 1000; ++i)
	{
		F32x3 in_sphere = random_f32x3_in_unit_sphere(rng);
		TESTER_CHECK(f32x3_length_squared(in_sphere) <= 1.0f + 1e-5f);

		F32x3 on_sphere = random_f32x3_on_unit_sphere(rng);
		TESTER_CHECK(f32_approx_equal(f32x3_length(on_sphere), 1.0f, 1e-5f));
	}
}

TESTER_TEST("[MATH][Random]: random_f32x2_in_unit_circle / on_unit_circle")
{
	Random rng;
	random_seed(rng, 0xBEEF);
	for (int i = 0; i < 1000; ++i)
	{
		F32x2 in_circle = random_f32x2_in_unit_circle(rng);
		TESTER_CHECK(f32x2_length_squared(in_circle) <= 1.0f + 1e-5f);

		F32x2 on_circle = random_f32x2_on_unit_circle(rng);
		TESTER_CHECK(f32_approx_equal(f32x2_length(on_circle), 1.0f, 1e-5f));
	}
}

TESTER_TEST("[MATH][Random]: random_quaternion produces unit quaternions")
{
	Random rng;
	random_seed(rng, 2026);
	for (int i = 0; i < 1000; ++i)
	{
		Quaternion q = random_quaternion(rng);
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
	format(f, TEST_QUAT_IDENTITY);
	TESTER_CHECK(f.buffer.count > 0);

	// Integer vector formatter fires.
	formatter_clear(f);
	format(f, U32x2{10u, 20u});
	TESTER_CHECK(f.buffer.count > 0);
}