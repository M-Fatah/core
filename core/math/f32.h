#pragma once

#include <core/defines.h>

#include <cmath>
#include <limits>

// ============================================================================
// F32 scalar helpers + constants.
//
// All angle-accepting functions take radians. Use F32_TO_RADIANS / F32_TO_DEGREES
// for conversion.
// ============================================================================

// ---- Angular constants -----------------------------------------------------

static constexpr F32 F32_PI         = 3.14159265358979323846f;
static constexpr F32 F32_TAU        = F32_PI * 2.0f;
static constexpr F32 F32_PI_OVER_2  = F32_PI * 0.5f;
static constexpr F32 F32_TO_DEGREES = 360.0f / F32_TAU;
static constexpr F32 F32_TO_RADIANS = F32_TAU / 360.0f;

// ---- Numeric limits / special values ---------------------------------------
// F32_MIN / F32_MAX live in core/defines.h alongside the primitive aliases.

static constexpr F32 F32_EPSILON      = std::numeric_limits<F32>::epsilon();
static constexpr F32 F32_INFINITY     = std::numeric_limits<F32>::infinity();
static constexpr F32 F32_NEG_INFINITY = -F32_INFINITY;
static constexpr F32 F32_NAN          = std::numeric_limits<F32>::quiet_NaN();

// ---- Transcendental / trigonometric wrappers -------------------------------

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

// ---- Basic arithmetic helpers ----------------------------------------------

inline static F32
f32_abs(F32 x)
{
	return x < 0.0f ? -x : x;
}

inline static F32
f32_sign(F32 x)
{
	if (x > 0.0f) return  1.0f;
	if (x < 0.0f) return -1.0f;
	return 0.0f;
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
f32_clamp(F32 x, F32 lo, F32 hi)
{
	if (x < lo) return lo;
	if (x > hi) return hi;
	return x;
}

inline static F32
f32_lerp(F32 a, F32 b, F32 t)
{
	return a + t * (b - a);
}

// ---- Special-value tests ---------------------------------------------------

inline static bool
f32_is_nan(F32 x)
{
	return x != x;
}

inline static bool
f32_is_infinite(F32 x)
{
	return x == F32_INFINITY || x == F32_NEG_INFINITY;
}

inline static bool
f32_is_finite(F32 x)
{
	return !f32_is_nan(x) && !f32_is_infinite(x);
}

// Absolute-tolerance comparison. Caller specifies epsilon — no magic default,
// since the right tolerance depends on the magnitude of the values being compared.
// For "approximately equal in relative terms" use `f32_abs(a - b) <= epsilon * f32_max(f32_abs(a), f32_abs(b))`.
inline static bool
f32_approx_equal(F32 a, F32 b, F32 epsilon)
{
	return f32_abs(a - b) <= epsilon;
}

// ---- Interpolation beyond lerp ---------------------------------------------

// Hermite 3t² - 2t³ smoothing. Input is remapped to [0,1] over [edge0, edge1] then smoothed.
inline static F32
f32_smoothstep(F32 edge0, F32 edge1, F32 x)
{
	F32 t = f32_clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

// Perlin's improved smoothstep: 6t⁵ - 15t⁴ + 10t³ (C² continuous).
inline static F32
f32_smootherstep(F32 edge0, F32 edge1, F32 x)
{
	F32 t = f32_clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

// Critically-damped spring smoothing. Returns the new current value and mutates
// `velocity` in place. Standard Unity-style signature. Pass the current and target
// values each frame and a `smooth_time` measured in seconds; the function settles
// `current` toward `target` over roughly `smooth_time`.
inline static F32
f32_smooth_damp(F32 current, F32 target, F32 *velocity, F32 smooth_time, F32 dt)
{
	smooth_time = f32_max(smooth_time, 0.0001f);
	F32 omega   = 2.0f / smooth_time;
	F32 x       = omega * dt;
	F32 exp_    = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
	F32 delta   = current - target;
	F32 temp    = (*velocity + omega * delta) * dt;
	*velocity   = (*velocity - omega * temp) * exp_;
	return target + (delta + temp) * exp_;
}

// ---- Easing curves ---------------------------------------------------------
// Input t in [0, 1] (clamped). Output in [0, 1]. `ease_in_out` variants are
// symmetric (ease in over first half, ease out over second).

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
	F32 one_minus = 1.0f - t;
	return 1.0f - one_minus * one_minus * one_minus;
}

inline static F32
f32_ease_in_out_cubic(F32 t)
{
	t = f32_clamp(t, 0.0f, 1.0f);
	if (t < 0.5f)
		return 4.0f * t * t * t;
	F32 one_minus = 1.0f - t;
	return 1.0f - 4.0f * one_minus * one_minus * one_minus;
}

inline static F32
f32_ease_in_elastic(F32 t)
{
	t = f32_clamp(t, 0.0f, 1.0f);
	if (t == 0.0f) return 0.0f;
	if (t == 1.0f) return 1.0f;
	constexpr F32 c4 = F32_TAU / 3.0f;
	return -f32_power(2.0f, 10.0f * t - 10.0f) * f32_sin((t * 10.0f - 10.75f) * c4);
}

inline static F32
f32_ease_out_elastic(F32 t)
{
	t = f32_clamp(t, 0.0f, 1.0f);
	if (t == 0.0f) return 0.0f;
	if (t == 1.0f) return 1.0f;
	constexpr F32 c4 = F32_TAU / 3.0f;
	return f32_power(2.0f, -10.0f * t) * f32_sin((t * 10.0f - 0.75f) * c4) + 1.0f;
}

inline static F32
f32_ease_in_out_elastic(F32 t)
{
	t = f32_clamp(t, 0.0f, 1.0f);
	if (t == 0.0f) return 0.0f;
	if (t == 1.0f) return 1.0f;
	constexpr F32 c5 = F32_TAU / 4.5f;
	if (t < 0.5f)
		return -(f32_power(2.0f,  20.0f * t - 10.0f) * f32_sin((20.0f * t - 11.125f) * c5)) * 0.5f;
	return   (f32_power(2.0f, -20.0f * t + 10.0f) * f32_sin((20.0f * t - 11.125f) * c5)) * 0.5f + 1.0f;
}
