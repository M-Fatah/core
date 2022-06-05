#pragma once

#include "core/defines.h"

// 32 bit Fowler-Noll-Vo hash.
inline static u64
hash_fnv_x32(const void *key, u64 key_length)
{
	const u32 p = 16777619U;
	u32 hash    = 2166136261U;

	const u8 *data = (const u8 *)key;
	for (i32 i = 0; i < key_length; ++i)
		hash = (hash ^ data[i]) * p;

	hash += hash << 13;
	hash ^= hash >> 7;
	hash += hash << 3;
	hash ^= hash >> 17;
	hash += hash << 5;
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
hash(const T *key)
{
	return u64(key);
};

inline static u64
hash(bool key)
{
	return u64(key);
}

inline static u64
hash(char key)
{
	return u64(key);
}

inline static u64
hash(i8 key)
{
	return u64(key);
}

inline static u64
hash(i16 key)
{
	return u64(key);
}

inline static u64
hash(i32 key)
{
	return u64(key);
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
	return hash_fnv_x32(&key, sizeof(f32));
}

inline static u64
hash(f64 key)
{
	return hash_fnv_x32(&key, sizeof(f64));
}