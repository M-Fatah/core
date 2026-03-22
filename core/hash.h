#pragma once

#include "core/defines.h"

// FNV-1a 64-bit hash.
inline static u64
hash_fnv1a_64(const void *key, u64 key_length)
{
	const u64 prime = 1099511628211ULL;
	u64 hash        = 14695981039346656037ULL;

	const u8 *data = (const u8 *)key;
	for (u64 i = 0; i < key_length; ++i)
		hash = (hash ^ data[i]) * prime;

	return hash;
}

template <typename T>
inline static u64
hash(const T &)
{
	static_assert(sizeof(T) == 0, "There is no 'u64 hash(const T &)' function overload defined for this type.");
	return 0;
}

template <typename T>
inline static u64
hash(T *key)
{
	return u64(key) >> 3;
};

template <typename T>
inline static u64
hash(const T *key)
{
	return u64(key) >> 3;
};

inline static u64
hash(bool key)
{
	return u64(key);
}

inline static u64
hash(char key)
{
	return u64((unsigned char)key);
}

inline static u64
hash(i8 key)
{
	return u64((u8)key);
}

inline static u64
hash(i16 key)
{
	return u64((u16)key);
}

inline static u64
hash(i32 key)
{
	return u64((u32)key);
}

inline static u64
hash(i64 key)
{
	return u64(key);
}

inline static u64
hash(u8 key)
{
	return u64(key);
}

inline static u64
hash(u16 key)
{
	return u64(key);
}

inline static u64
hash(u32 key)
{
	return u64(key);
}

inline static u64
hash(u64 key)
{
	return u64(key);
}

inline static u64
hash(f32 key)
{
	return hash_fnv1a_64(&key, sizeof(f32));
}

inline static u64
hash(f64 key)
{
	return hash_fnv1a_64(&key, sizeof(f64));
}