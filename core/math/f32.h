#pragma once

#include "core/defines.h"
#include "core/validate.h"

#include <float.h>
#include <math.h>

inline static constexpr F32 F32_PI                = 3.14159265358979323846f;
inline static constexpr F32 F32_TAU               = F32_PI * 2.0f;
inline static constexpr F32 F32_TO_DEGREES        = 360.0f / F32_TAU;
inline static constexpr F32 F32_TO_RADIANS        = F32_TAU / 360.0f;

inline static constexpr F32 F32_EPSILON           = FLT_EPSILON;
inline static constexpr F32 F32_INFINITY          = INFINITY;
inline static constexpr F32 F32_NEGATIVE_INFINITY = -F32_INFINITY;
inline static constexpr F32 F32_NAN               = NAN;

inline static F32
f32_sqrt(F32 x)
{
	return ::sqrtf(x);
}

inline static F32
f32_sin(F32 x)
{
	return ::sinf(x);
}

inline static F32
f32_asin(F32 x)
{
	return ::asinf(x);
}

inline static F32
f32_cos(F32 x)
{
	return ::cosf(x);
}

inline static F32
f32_acos(F32 x)
{
	return ::acosf(x);
}

inline static F32
f32_tan(F32 x)
{
	return ::tanf(x);
}

inline static F32
f32_atan2(F32 y, F32 x)
{
	return ::atan2f(y, x);
}

inline static F32
f32_power(F32 base, F32 exponent)
{
	return ::powf(base, exponent);
}

inline static F32
f32_modulo(F32 x, F32 divisor)
{
	return ::fmodf(x, divisor);
}

inline static F32
f32_floor(F32 x)
{
	return ::floorf(x);
}

inline static F32
f32_ceil(F32 x)
{
	return ::ceilf(x);
}

inline static F32
f32_round(F32 x)
{
	return ::roundf(x);
}

inline static F32
f32_trunc(F32 x)
{
	return ::truncf(x);
}

inline static F32
f32_fract(F32 x)
{
	return x - f32_floor(x);
}

inline static F32
f32_min(F32 a, F32 b)
{
	return a < b ? a : b;
}

inline static F32
f32_max(F32 a, F32 b)
{
	return a > b ? a : b;
}

inline static F32
f32_clamp(F32 x, F32 a, F32 b)
{
	if (x < a)
		return a;
	if (x > b)
		return b;
	return x;
}

inline static F32
f32_saturate(F32 x)
{
	return f32_clamp(x, 0.0f, 1.0f);
}

inline static F32
f32_abs(F32 x)
{
	return x < 0.0f ? -x : x;
}

inline static F32
f32_sign(F32 x)
{
	if (x > 0.0f)
		return  1.0f;
	if (x < 0.0f)
		return -1.0f;
	return 0.0f;
}

inline static bool
f32_is_nan(F32 x)
{
	return x != x;
}

inline static bool
f32_is_infinite(F32 x)
{
	return x == F32_INFINITY || x == F32_NEGATIVE_INFINITY;
}

inline static bool
f32_is_finite(F32 x)
{
	return !f32_is_nan(x) && !f32_is_infinite(x);
}

inline static bool
f32_approx_equal(F32 a, F32 b, F32 epsilon)
{
	validate(epsilon >= 0.0f, "[MATH][F32]: approx_equal epsilon must be non-negative.");
	return f32_abs(a - b) <= epsilon;
}

inline static bool
f32_approx_equal_relative(F32 a, F32 b, F32 epsilon)
{
	validate(epsilon >= 0.0f, "[MATH][F32]: approx_equal_relative epsilon must be non-negative.");
	F32 scale = f32_max(1.0f, f32_max(f32_abs(a), f32_abs(b)));
	return f32_abs(a - b) <= epsilon * scale;
}

inline static F32
f32_wrap_radians(F32 angle)
{
	F32 wrapped = f32_modulo(angle + F32_PI, F32_TAU);
	if (wrapped < 0.0f)
		wrapped += F32_TAU;
	return wrapped - F32_PI;
}

inline static F32
f32_angle_delta(F32 from, F32 to)
{
	return f32_wrap_radians(to - from);
}

inline static F32
f32_lerp(F32 a, F32 b, F32 t)
{
	return a + t * (b - a);
}

inline static F32
f32_inverse_lerp(F32 a, F32 b, F32 x)
{
	validate(a != b, "[MATH][F32]: inverse_lerp range must not be empty.");
	return (x - a) / (b - a);
}

inline static F32
f32_remap(F32 in_min, F32 in_max, F32 out_min, F32 out_max, F32 x)
{
	return f32_lerp(out_min, out_max, f32_inverse_lerp(in_min, in_max, x));
}

inline static F32
f32_move_towards(F32 current, F32 target, F32 max_delta)
{
	validate(max_delta >= 0.0f, "[MATH][F32]: move_towards max_delta must be non-negative.");
	F32 delta = target - current;
	if (f32_abs(delta) <= max_delta)
		return target;
	return current + f32_sign(delta) * max_delta;
}

inline static F32
f32_smoothstep(F32 edge0, F32 edge1, F32 x)
{
	validate(edge0 != edge1, "[MATH][F32]: smoothstep edges must not be equal.");
	F32 t = f32_clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

inline static F32
f32_smootherstep(F32 edge0, F32 edge1, F32 x)
{
	validate(edge0 != edge1, "[MATH][F32]: smootherstep edges must not be equal.");
	F32 t = f32_clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

inline static F32
f32_smooth_damp(F32 current, F32 target, F32 *velocity, F32 smooth_time, F32 dt)
{
	validate(velocity != nullptr, "[MATH][F32]: smooth_damp velocity must not be null.");
	validate(smooth_time > 0.0f, "[MATH][F32]: smooth_damp smooth_time must be positive.");
	validate(dt >= 0.0f, "[MATH][F32]: smooth_damp dt must be non-negative.");
	F32 omega = 2.0f / smooth_time;
	F32 x     = omega * dt;
	F32 exp   = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
	F32 delta = current - target;
	F32 temp  = (*velocity + omega * delta) * dt;
	*velocity = (*velocity - omega * temp) * exp;
	return target + (delta + temp) * exp;
}

inline static F32
f32_ease_in_quad(F32 t)
{
	t = f32_clamp(t, 0.0f, 1.0f);
	return t * t;
}

inline static F32
f32_ease_out_quad(F32 t)
{
	t = f32_clamp(t, 0.0f, 1.0f);
	return 1.0f - (1.0f - t) * (1.0f - t);
}

inline static F32
f32_ease_in_out_quad(F32 t)
{
	t = f32_clamp(t, 0.0f, 1.0f);
	return t < 0.5f ? 2.0f * t * t : 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
}

inline static F32
f32_ease_in_cubic(F32 t)
{
	t = f32_clamp(t, 0.0f, 1.0f);
	return t * t * t;
}

inline static F32
f32_ease_out_cubic(F32 t)
{
	t = f32_clamp(t, 0.0f, 1.0f);
	F32 one_minus_t = 1.0f - t;
	return 1.0f - one_minus_t * one_minus_t * one_minus_t;
}

inline static F32
f32_ease_in_out_cubic(F32 t)
{
	t = f32_clamp(t, 0.0f, 1.0f);
	if (t < 0.5f)
		return 4.0f * t * t * t;
	F32 one_minus_t = 1.0f - t;
	return 1.0f - 4.0f * one_minus_t * one_minus_t * one_minus_t;
}

inline static F32
f32_ease_in_elastic(F32 t)
{
	constexpr F32 C4 = F32_TAU / 3.0f;
	t = f32_clamp(t, 0.0f, 1.0f);
	if (t == 0.0f)
		return 0.0f;
	if (t == 1.0f)
		return 1.0f;
	return -f32_power(2.0f, 10.0f * t - 10.0f) * f32_sin((t * 10.0f - 10.75f) * C4);
}

inline static F32
f32_ease_out_elastic(F32 t)
{
	constexpr F32 C4 = F32_TAU / 3.0f;
	t = f32_clamp(t, 0.0f, 1.0f);
	if (t == 0.0f)
		return 0.0f;
	if (t == 1.0f)
		return 1.0f;
	return f32_power(2.0f, -10.0f * t) * f32_sin((t * 10.0f - 0.75f) * C4) + 1.0f;
}

inline static F32
f32_ease_in_out_elastic(F32 t)
{
	constexpr F32 C5 = F32_TAU / 4.5f;
	t = f32_clamp(t, 0.0f, 1.0f);
	if (t == 0.0f)
		return 0.0f;
	if (t == 1.0f)
		return 1.0f;
	if (t < 0.5f)
		return -(f32_power(2.0f,  20.0f * t - 10.0f) * f32_sin((20.0f * t - 11.125f) * C5)) * 0.5f;
	return (f32_power(2.0f, -20.0f * t + 10.0f) * f32_sin((20.0f * t - 11.125f) * C5)) * 0.5f + 1.0f;
}