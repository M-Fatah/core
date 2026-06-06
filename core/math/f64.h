#pragma once

#include "core/defines.h"
#include "core/validate.h"

#include <float.h>
#include <math.h>

inline static constexpr F64 F64_PI                = 3.14159265358979323846;
inline static constexpr F64 F64_TAU               = F64_PI * 2.0;
inline static constexpr F64 F64_TO_DEGREES        = 360.0 / F64_TAU;
inline static constexpr F64 F64_TO_RADIANS        = F64_TAU / 360.0;

inline static constexpr F64 F64_EPSILON           = DBL_EPSILON;
inline static constexpr F64 F64_INFINITY          = INFINITY;
inline static constexpr F64 F64_NEGATIVE_INFINITY = -F64_INFINITY;
inline static constexpr F64 F64_NAN               = NAN;

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

inline static F64
f64_floor(F64 x)
{
	return ::floor(x);
}

inline static F64
f64_ceil(F64 x)
{
	return ::ceil(x);
}

inline static F64
f64_round(F64 x)
{
	return ::round(x);
}

inline static F64
f64_trunc(F64 x)
{
	return ::trunc(x);
}

inline static F64
f64_fract(F64 x)
{
	return x - f64_floor(x);
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
f64_clamp(F64 x, F64 a, F64 b)
{
	if (x < a)
		return a;
	if (x > b)
		return b;
	return x;
}

inline static F64
f64_saturate(F64 x)
{
	return f64_clamp(x, 0.0, 1.0);
}

inline static F64
f64_abs(F64 x)
{
	return x < 0.0 ? -x : x;
}

inline static F64
f64_sign(F64 x)
{
	if (x > 0.0)
		return  1.0;
	if (x < 0.0)
		return -1.0;
	return 0.0;
}

inline static bool
f64_is_nan(F64 x)
{
	return x != x;
}

inline static bool
f64_is_infinite(F64 x)
{
	return x == F64_INFINITY || x == F64_NEGATIVE_INFINITY;
}

inline static bool
f64_is_finite(F64 x)
{
	return !f64_is_nan(x) && !f64_is_infinite(x);
}

inline static bool
f64_approx_equal(F64 a, F64 b, F64 epsilon)
{
	validate(epsilon >= 0.0, "[MATH][F64]: approx_equal epsilon must be non-negative.");
	return f64_abs(a - b) <= epsilon;
}

inline static bool
f64_approx_equal_relative(F64 a, F64 b, F64 epsilon)
{
	validate(epsilon >= 0.0, "[MATH][F64]: approx_equal_relative epsilon must be non-negative.");
	F64 scale = f64_max(1.0, f64_max(f64_abs(a), f64_abs(b)));
	return f64_abs(a - b) <= epsilon * scale;
}

inline static F64
f64_wrap_radians(F64 angle)
{
	F64 wrapped = f64_modulo(angle + F64_PI, F64_TAU);
	if (wrapped < 0.0)
		wrapped += F64_TAU;
	return wrapped - F64_PI;
}

inline static F64
f64_angle_delta(F64 from, F64 to)
{
	return f64_wrap_radians(to - from);
}

inline static F64
f64_lerp(F64 a, F64 b, F64 t)
{
	return a + t * (b - a);
}

inline static F64
f64_inverse_lerp(F64 a, F64 b, F64 x)
{
	validate(a != b, "[MATH][F64]: inverse_lerp range must not be empty.");
	return (x - a) / (b - a);
}

inline static F64
f64_remap(F64 in_min, F64 in_max, F64 out_min, F64 out_max, F64 x)
{
	return f64_lerp(out_min, out_max, f64_inverse_lerp(in_min, in_max, x));
}

inline static F64
f64_move_towards(F64 current, F64 target, F64 max_delta)
{
	validate(max_delta >= 0.0, "[MATH][F64]: move_towards max_delta must be non-negative.");
	F64 delta = target - current;
	if (f64_abs(delta) <= max_delta)
		return target;
	return current + f64_sign(delta) * max_delta;
}

inline static F64
f64_smoothstep(F64 edge_0, F64 edge_1, F64 x)
{
	validate(edge_0 != edge_1, "[MATH][F64]: smoothstep edges must not be equal.");
	F64 t = f64_clamp((x - edge_0) / (edge_1 - edge_0), 0.0, 1.0);
	return t * t * (3.0 - 2.0 * t);
}

inline static F64
f64_smootherstep(F64 edge_0, F64 edge_1, F64 x)
{
	validate(edge_0 != edge_1, "[MATH][F64]: smootherstep edges must not be equal.");
	F64 t = f64_clamp((x - edge_0) / (edge_1 - edge_0), 0.0, 1.0);
	return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

inline static F64
f64_smooth_damp(F64 current, F64 target, F64 *velocity, F64 smooth_time, F64 dt)
{
	validate(velocity != nullptr, "[MATH][F64]: smooth_damp velocity must not be null.");
	validate(smooth_time > 0.0, "[MATH][F64]: smooth_damp smooth_time must be positive.");
	validate(dt >= 0.0, "[MATH][F64]: smooth_damp dt must be non-negative.");
	F64 omega = 2.0 / smooth_time;
	F64 x     = omega * dt;
	F64 exp   = 1.0 / (1.0 + x + 0.48 * x * x + 0.235 * x * x * x);
	F64 delta = current - target;
	F64 temp  = (*velocity + omega * delta) * dt;
	*velocity = (*velocity - omega * temp) * exp;
	return target + (delta + temp) * exp;
}

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
	F64 one_minus_t = 1.0 - t;
	return 1.0 - one_minus_t * one_minus_t * one_minus_t;
}

inline static F64
f64_ease_in_out_cubic(F64 t)
{
	t = f64_clamp(t, 0.0, 1.0);
	if (t < 0.5)
		return 4.0 * t * t * t;
	F64 one_minus_t = 1.0 - t;
	return 1.0 - 4.0 * one_minus_t * one_minus_t * one_minus_t;
}

inline static F64
f64_ease_in_elastic(F64 t)
{
	constexpr F64 C4 = F64_TAU / 3.0;
	t = f64_clamp(t, 0.0, 1.0);
	if (t == 0.0)
		return 0.0;
	if (t == 1.0)
		return 1.0;
	return -f64_power(2.0, 10.0 * t - 10.0) * f64_sin((t * 10.0 - 10.75) * C4);
}

inline static F64
f64_ease_out_elastic(F64 t)
{
	constexpr F64 C4 = F64_TAU / 3.0;
	t = f64_clamp(t, 0.0, 1.0);
	if (t == 0.0)
		return 0.0;
	if (t == 1.0)
		return 1.0;
	return f64_power(2.0, -10.0 * t) * f64_sin((t * 10.0 - 0.75) * C4) + 1.0;
}

inline static F64
f64_ease_in_out_elastic(F64 t)
{
	constexpr F64 C5 = F64_TAU / 4.5;
	t = f64_clamp(t, 0.0, 1.0);
	if (t == 0.0)
		return 0.0;
	if (t == 1.0)
		return 1.0;
	if (t < 0.5)
		return -(f64_power(2.0,  20.0 * t - 10.0) * f64_sin((20.0 * t - 11.125) * C5)) * 0.5;
	return (f64_power(2.0, -20.0 * t + 10.0) * f64_sin((20.0 * t - 11.125) * C5)) * 0.5 + 1.0;
}