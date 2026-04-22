#pragma once

#include <core/defines.h>
#include <core/math/f32.h>
#include <core/math/f64.h>
#include <core/math/f32x2.h>
#include <core/math/f32x3.h>
#include <core/math/quaternion.h>

#include <string.h>  // For ::memcpy (bit-cast U32 → F32 in f32_random_unit).

// ============================================================================
// Random — explicit-state PRNG (xoshiro256**) + typed samplers.
//
// No hidden global state. Seeded explicitly for reproducibility (replays,
// networking, tests). Core pattern:
//
//     Random rng = random_from_seed(0xdeadbeef);
//     F32 angle  = f32_random_range(rng, 0.0f, F32_TAU);
//
// xoshiro256** has 256-bit state, passes BigCrush, is faster than Mersenne
// Twister, and has a jump primitive for parallel streams (not exposed yet —
// revisit when needed).
// ============================================================================

struct Random
{
	U64 state[4];
};

// SplitMix64 — expands a single 64-bit seed into four uncorrelated 64-bit
// words for the xoshiro state. From the xoshiro reference implementation.
inline static U64
_random_splitmix64_next(U64 &s)
{
	U64 z = (s += 0x9E3779B97F4A7C15ULL);
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
	return z ^ (z >> 31);
}

inline static U64
_random_rotate_left(U64 x, I32 k) { return (x << k) | (x >> (64 - k)); }

inline static void
random_seed(Random &rng, U64 seed)
{
	U64 s = seed;
	rng.state[0] = _random_splitmix64_next(s);
	rng.state[1] = _random_splitmix64_next(s);
	rng.state[2] = _random_splitmix64_next(s);
	rng.state[3] = _random_splitmix64_next(s);
}

// Convenience: construct and seed in one step. Useful for throwaway streams
// where you don't need to hold the `Random` across function boundaries.
inline static Random
random_from_seed(U64 seed)
{
	Random rng;
	random_seed(rng, seed);
	return rng;
}

inline static U64
random_u64(Random &rng)
{
	// xoshiro256** — reference implementation.
	const U64 result = _random_rotate_left(rng.state[1] * 5, 7) * 9;
	const U64 t      = rng.state[1] << 17;

	rng.state[2] ^= rng.state[0];
	rng.state[3] ^= rng.state[1];
	rng.state[1] ^= rng.state[2];
	rng.state[0] ^= rng.state[3];
	rng.state[2] ^= t;
	rng.state[3] = _random_rotate_left(rng.state[3], 45);

	return result;
}

inline static U32
random_u32(Random &rng)
{
	// High bits of xoshiro256** have best quality.
	return (U32)(random_u64(rng) >> 32);
}

// ---- Typed samplers (floats) -----------------------------------------------
// Map the top 24 bits of a U32 into [0, 1) by interpreting as a float in
// [1.0, 2.0) and subtracting 1 — standard trick that avoids bias from modulo.

inline static F32
f32_random_unit(Random &rng)
{
	U32 bits = (random_u32(rng) >> 8) | 0x3F800000u;
	F32 f;
	::memcpy(&f, &bits, sizeof(f));
	return f - 1.0f;
}

inline static F32
f32_random_range(Random &rng, F32 min, F32 max)
{
	return min + f32_random_unit(rng) * (max - min);
}

inline static F64
f64_random_unit(Random &rng)
{
	// Use top 52 bits.
	U64 bits = (random_u64(rng) >> 12) | 0x3FF0000000000000ULL;
	F64 d;
	::memcpy(&d, &bits, sizeof(d));
	return d - 1.0;
}

inline static F64
f64_random_range(Random &rng, F64 min, F64 max)
{
	return min + f64_random_unit(rng) * (max - min);
}

// ---- Typed samplers (integers) ---------------------------------------------

inline static I32
i32_random_range(Random &rng, I32 min, I32 max)
{
	// Inclusive: [min, max]. Uses U64 modulo which has negligible bias for
	// typical small ranges.
	U64 span  = (U64)((I64)max - (I64)min + 1);
	U64 value = random_u64(rng) % span;
	return (I32)((I64)min + (I64)value);
}

inline static U32
u32_random_range(Random &rng, U32 min, U32 max)
{
	U64 span  = (U64)max - (U64)min + 1;
	U64 value = random_u64(rng) % span;
	return (U32)(min + (U32)value);
}

// ---- Typed samplers (geometric) --------------------------------------------

// Uniform point inside the unit disk (|p| ≤ 1). Rejection sampling — cheap
// and unbiased.
inline static F32x2
f32x2_random_in_unit_disk(Random &rng)
{
	for (;;)
	{
		F32x2 p = {
			f32_random_range(rng, -1.0f, 1.0f),
			f32_random_range(rng, -1.0f, 1.0f)
		};
		if (f32x2_length_squared(p) <= 1.0f)
			return p;
	}
}

// Uniform point inside the unit sphere (|p| ≤ 1).
inline static F32x3
f32x3_random_in_unit_sphere(Random &rng)
{
	for (;;)
	{
		F32x3 p = {
			f32_random_range(rng, -1.0f, 1.0f),
			f32_random_range(rng, -1.0f, 1.0f),
			f32_random_range(rng, -1.0f, 1.0f)
		};
		if (f32x3_length_squared(p) <= 1.0f)
			return p;
	}
}

// Uniform point on the surface of the unit sphere (|p| = 1). Uses spherical
// coordinates with inverse-CDF sampling — exact, no rejection.
inline static F32x3
f32x3_random_on_unit_sphere(Random &rng)
{
	F32 z   = f32_random_range(rng, -1.0f, 1.0f);
	F32 phi = f32_random_range(rng,  0.0f, F32_TAU);
	F32 r   = f32_sqrt(1.0f - z * z);
	return F32x3{r * f32_cos(phi), r * f32_sin(phi), z};
}

// Uniform random rotation (Shoemake's method, 1992). Samples three uniform
// variables and combines them into a unit quaternion distributed uniformly on SO(3).
inline static Quaternion
quaternion_random(Random &rng)
{
	F32 u1 = f32_random_unit(rng);
	F32 u2 = f32_random_range(rng, 0.0f, F32_TAU);
	F32 u3 = f32_random_range(rng, 0.0f, F32_TAU);

	F32 a = f32_sqrt(1.0f - u1);
	F32 b = f32_sqrt(u1);

	return Quaternion{
		a * f32_cos(u2),
		a * f32_sin(u2),
		b * f32_cos(u3),
		b * f32_sin(u3)
	};
}
