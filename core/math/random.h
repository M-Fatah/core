#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/math/u64.h"
#include "core/math/f32.h"
#include "core/math/f64.h"
#include "core/math/f32x2.h"
#include "core/math/f32x3.h"
#include "core/math/quaternion.h"

#include <string.h>

struct Random
{
	U64 state[4];
};

inline static Random
random_from_seed_and_stream(U64 seed, U64 stream_index)
{
	constexpr auto splitmix64_next = [](U64 &s) -> U64 {
		U64 z = (s += 0x9E3779B97F4A7C15ULL);
		z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
		z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
		return z ^ (z >> 31);
	};

	if (stream_index != 0)
	{
		U64 stream = stream_index;
		seed ^= splitmix64_next(stream);
		seed += splitmix64_next(stream);
	}

	U64 s = seed;
	return Random {
		.state = {
			splitmix64_next(s),
			splitmix64_next(s),
			splitmix64_next(s),
			splitmix64_next(s)
		}
	};
}

inline static Random
random_from_seed(U64 seed)
{
	return random_from_seed_and_stream(seed, 0);
}

inline static void
random_seed(Random &self, U64 seed)
{
	self = random_from_seed(seed);
}

inline static U64
random_u64(Random &self);

inline static U64
random_u64_range(Random &self, U64 min, U64 max);

inline static I32
random_i32_range(Random &self, I32 min, I32 max)
{
	validate(min <= max, "[MATH][Random]: random_i32_range requires min <= max.");
	U64 span = (U64)((U32)max - (U32)min) + 1;
	U64 value = random_u64_range(self, 0, span - 1);
	return (I32)((U32)min + (U32)value);
}

inline static I64
random_i64_range(Random &self, I64 min, I64 max)
{
	validate(min <= max, "[MATH][Random]: random_i64_range requires min <= max.");
	U64 span = (U64)max - (U64)min + 1;
	U64 value = random_u64_range(self, 0, span - 1);
	return (I64)((U64)min + value);
}

inline static U32
random_u32(Random &self)
{
	return (U32)(random_u64(self) >> 32);
}

inline static U32
random_u32_range(Random &self, U32 min, U32 max)
{
	validate(min <= max, "[MATH][Random]: random_u32_range requires min <= max.");
	return (U32)random_u64_range(self, (U64)min, (U64)max);
}

inline static U64
random_u64(Random &self)
{
	const U64 result = u64_rotate_left(self.state[1] * 5, 7) * 9;
	const U64 t      = self.state[1] << 17;

	self.state[2] ^= self.state[0];
	self.state[3] ^= self.state[1];
	self.state[1] ^= self.state[2];
	self.state[0] ^= self.state[3];
	self.state[2] ^= t;
	self.state[3]  = u64_rotate_left(self.state[3], 45);

	return result;
}

inline static U64
random_u64_range(Random &self, U64 min, U64 max)
{
	validate(min <= max, "[MATH][Random]: random_u64_range requires min <= max.");
	auto bounded_u64 = [](Random &self, U64 span) -> U64 {
		if (span == 0)
			return random_u64(self);
		if (span == 1)
			return 0;
		U64 threshold = (0ull - span) % span;
		for (;;)
		{
			U64 value = random_u64(self);
			if (value >= threshold)
				return value % span;
		}
	};
	U64 span = max - min + 1;
	U64 value = bounded_u64(self, span);
	return min + value;
}

inline static F32
random_f32_unit(Random &self)
{
	U32 bits = (random_u32(self) >> 8) | 0x3F800000u;
	F32 f;
	::memcpy(&f, &bits, sizeof(f));
	return f - 1.0f;
}

inline static F32
random_f32_range(Random &self, F32 min, F32 max)
{
	validate(min < max, "[MATH][Random]: random_f32_range requires min < max.");
	return min + random_f32_unit(self) * (max - min);
}

inline static F64
random_f64_unit(Random &self)
{
	U64 bits = (random_u64(self) >> 12) | 0x3FF0000000000000ULL;
	F64 d;
	::memcpy(&d, &bits, sizeof(d));
	return d - 1.0;
}

inline static F64
random_f64_range(Random &self, F64 min, F64 max)
{
	validate(min < max, "[MATH][Random]: random_f64_range requires min < max.");
	return min + random_f64_unit(self) * (max - min);
}

inline static F32x2
random_f32x2_in_unit_circle(Random &self)
{
	for (;;)
	{
		F32x2 p = F32x2 {
			.x = random_f32_range(self, -1.0f, 1.0f),
			.y = random_f32_range(self, -1.0f, 1.0f)
		};
		if (f32x2_length_squared(p) <= 1.0f)
			return p;
	}
}

inline static F32x2
random_f32x2_on_unit_circle(Random &self)
{
	F32 phi = random_f32_range(self, 0.0f, F32_TAU);
	return F32x2{.x = f32_cos(phi), .y = f32_sin(phi)};
}

inline static F32x3
random_f32x3_in_unit_sphere(Random &self)
{
	for (;;)
	{
		F32x3 p = F32x3 {
			.x = random_f32_range(self, -1.0f, 1.0f),
			.y = random_f32_range(self, -1.0f, 1.0f),
			.z = random_f32_range(self, -1.0f, 1.0f)
		};
		if (f32x3_length_squared(p) <= 1.0f)
			return p;
	}
}

inline static F32x3
random_f32x3_on_unit_sphere(Random &self)
{
	F32 z   = random_f32_range(self, -1.0f, 1.0f);
	F32 phi = random_f32_range(self,  0.0f, F32_TAU);
	F32 r   = f32_sqrt(1.0f - z * z);
	return F32x3{.x = r * f32_cos(phi), .y = r * f32_sin(phi), .z = z};
}

inline static Quaternion
random_quaternion(Random &self)
{
	F32 u1 = random_f32_unit(self);
	F32 u2 = random_f32_range(self, 0.0f, F32_TAU);
	F32 u3 = random_f32_range(self, 0.0f, F32_TAU);

	F32 a = f32_sqrt(1.0f - u1);
	F32 b = f32_sqrt(u1);

	return Quaternion {
		.w = a * f32_cos(u2),
		.x = a * f32_sin(u2),
		.y = b * f32_cos(u3),
		.z = b * f32_sin(u3)
	};
}