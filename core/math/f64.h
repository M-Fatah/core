#pragma once

#include <core/defines.h>

#include <cmath>
#include <limits>

// ============================================================================
// F64 scalar helpers + constants — full prefixed mirror of F32.h.
// ============================================================================

// ---- Angular constants -----------------------------------------------------

static constexpr F64 F64_PI         = 3.14159265358979323846;
static constexpr F64 F64_TAU        = F64_PI * 2.0;
static constexpr F64 F64_PI_OVER_2  = F64_PI * 0.5;
static constexpr F64 F64_TO_DEGREES = 360.0 / F64_TAU;
static constexpr F64 F64_TO_RADIANS = F64_TAU / 360.0;

// ---- Numeric limits / special values ---------------------------------------
// F64_MIN / F64_MAX live in core/defines.h.

static constexpr F64 F64_EPSILON      = std::numeric_limits<F64>::epsilon();
static constexpr F64 F64_INFINITY     = std::numeric_limits<F64>::infinity();
static constexpr F64 F64_NEG_INFINITY = -F64_INFINITY;
static constexpr F64 F64_NAN          = std::numeric_limits<F64>::quiet_NaN();

// ---- Transcendental / trigonometric wrappers -------------------------------

inline static F64
f64_sqrt(F64 x)
{
	return ::sqrt(x);
}

inline static F64
f64_sin(F64 x)
{
	return ::sin(x);
}

inline static F64
f64_asin(F64 x)
{
	return ::asin(x);
}

inline static F64
f64_cos(F64 x)
{
	return ::cos(x);
}

inline static F64
f64_acos(F64 x)
{
	return ::acos(x);
}

inline static F64
f64_tan(F64 x)
{
	return ::tan(x);
}

inline static F64
f64_atan2(F64 y, F64 x)
{
	return ::atan2(y, x);
}

inline static F64
f64_power(F64 base, F64 exponent)
{
	return ::pow(base, exponent);
}

inline static F64
f64_modulo(F64 x, F64 divisor)
{
	return ::fmod(x, divisor);
}

// ---- Basic arithmetic helpers ----------------------------------------------

inline static F64
f64_abs(F64 x)
{
	return x < 0.0 ? -x : x;
}

inline static F64
f64_sign(F64 x)
{
	if (x > 0.0) return  1.0;
	if (x < 0.0) return -1.0;
	return 0.0;
}

inline static F64
f64_min(F64 a, F64 b)
{
	return a < b ? a : b;
}

inline static F64
f64_max(F64 a, F64 b)
{
	return a > b ? a : b;
}

inline static F64
f64_clamp(F64 x, F64 lo, F64 hi)
{
	if (x < lo) return lo;
	if (x > hi) return hi;
	return x;
}

inline static F64
f64_lerp(F64 a, F64 b, F64 t)
{
	return a + t * (b - a);
}

// ---- Special-value tests ---------------------------------------------------

inline static bool
f64_is_nan(F64 x)
{
	return x != x;
}

inline static bool
f64_is_infinite(F64 x)
{
	return x == F64_INFINITY || x == F64_NEG_INFINITY;
}

inline static bool
f64_is_finite(F64 x)
{
	return !f64_is_nan(x) && !f64_is_infinite(x);
}

inline static bool
f64_approx_equal(F64 a, F64 b, F64 epsilon)
{
	return f64_abs(a - b) <= epsilon;
}

// ---- Interpolation beyond lerp ---------------------------------------------

inline static F64
f64_smoothstep(F64 edge0, F64 edge1, F64 x)
{
	F64 t = f64_clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
	return t * t * (3.0 - 2.0 * t);
}

inline static F64
f64_smootherstep(F64 edge0, F64 edge1, F64 x)
{
	F64 t = f64_clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
	return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

inline static F64
f64_smooth_damp(F64 current, F64 target, F64 *velocity, F64 smooth_time, F64 dt)
{
	smooth_time = f64_max(smooth_time, 0.0001);
	F64 omega   = 2.0 / smooth_time;
	F64 x       = omega * dt;
	F64 exp_    = 1.0 / (1.0 + x + 0.48 * x * x + 0.235 * x * x * x);
	F64 delta   = current - target;
	F64 temp    = (*velocity + omega * delta) * dt;
	*velocity   = (*velocity - omega * temp) * exp_;
	return target + (delta + temp) * exp_;
}

// ---- Easing curves ---------------------------------------------------------

inline static F64
f64_ease_in_quad(F64 t)
{
	t = f64_clamp(t, 0.0, 1.0);
	return t * t;
}

inline static F64
f64_ease_out_quad(F64 t)
{
	t = f64_clamp(t, 0.0, 1.0);
	return 1.0 - (1.0 - t) * (1.0 - t);
}

inline static F64
f64_ease_in_out_quad(F64 t)
{
	t = f64_clamp(t, 0.0, 1.0);
	return t < 0.5 ? 2.0 * t * t : 1.0 - 2.0 * (1.0 - t) * (1.0 - t);
}

inline static F64
f64_ease_in_cubic(F64 t)
{
	t = f64_clamp(t, 0.0, 1.0);
	return t * t * t;
}

inline static F64
f64_ease_out_cubic(F64 t)
{
	t = f64_clamp(t, 0.0, 1.0);
	F64 one_minus = 1.0 - t;
	return 1.0 - one_minus * one_minus * one_minus;
}

inline static F64
f64_ease_in_out_cubic(F64 t)
{
	t = f64_clamp(t, 0.0, 1.0);
	if (t < 0.5)
		return 4.0 * t * t * t;
	F64 one_minus = 1.0 - t;
	return 1.0 - 4.0 * one_minus * one_minus * one_minus;
}

inline static F64
f64_ease_in_elastic(F64 t)
{
	t = f64_clamp(t, 0.0, 1.0);
	if (t == 0.0) return 0.0;
	if (t == 1.0) return 1.0;
	constexpr F64 c4 = F64_TAU / 3.0;
	return -f64_power(2.0, 10.0 * t - 10.0) * f64_sin((t * 10.0 - 10.75) * c4);
}

inline static F64
f64_ease_out_elastic(F64 t)
{
	t = f64_clamp(t, 0.0, 1.0);
	if (t == 0.0) return 0.0;
	if (t == 1.0) return 1.0;
	constexpr F64 c4 = F64_TAU / 3.0;
	return f64_power(2.0, -10.0 * t) * f64_sin((t * 10.0 - 0.75) * c4) + 1.0;
}

inline static F64
f64_ease_in_out_elastic(F64 t)
{
	t = f64_clamp(t, 0.0, 1.0);
	if (t == 0.0) return 0.0;
	if (t == 1.0) return 1.0;
	constexpr F64 c5 = F64_TAU / 4.5;
	if (t < 0.5)
		return -(f64_power(2.0,  20.0 * t - 10.0) * f64_sin((20.0 * t - 11.125) * c5)) * 0.5;
	return   (f64_power(2.0, -20.0 * t + 10.0) * f64_sin((20.0 * t - 11.125) * c5)) * 0.5 + 1.0;
}
