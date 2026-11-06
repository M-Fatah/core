#pragma once

#include "core/defines.h"

// 32 bit Fowler-Noll-Vo hash.
inline static U64
hash_fnv_x32(const void *key, U64 key_length)
{
	const U32 p = 16777619U;
	U32 hash    = 2166136261U;

	const U8 *data = (const U8 *)key;
	for (U64 i = 0; i < key_length; ++i)
		hash = (hash ^ data[i]) * p;

	hash += hash << 13;
	hash ^= hash >> 7;
	hash += hash << 3;
	hash ^= hash >> 17;
	hash += hash << 5;
	return hash;
}

template <typename T>
inline static U64
hash(const T &)
{
	static_assert(sizeof(T) == 0, "There is no 'u64 hash(const T &)' function overload defined for this type.");
	return 0;
}

template <typename T>
inline static U64
hash(T *key)
{
	//
	// Mix upper address bits into the low bits used by power-of-two hash tables.
	// Aligned arena allocations otherwise cluster in the same few buckets.
	//
	U64 value = U64(key);
	value ^= value >> 30;
	value *= 0xbf58476d1ce4e5b9ULL;
	value ^= value >> 27;
	value *= 0x94d049bb133111ebULL;
	value ^= value >> 31;
	return value;
}

inline static U64
hash(bool key)
{
	return U64(key);
}

inline static U64
hash(char key)
{
	return U64(key);
}

inline static U64
hash(I8 key)
{
	return U64(key);
}

inline static U64
hash(I16 key)
{
	return U64(key);
}

inline static U64
hash(I32 key)
{
	return U64(key);
}

inline static U64
hash(I64 key)
{
	return U64(key);
}

inline static U64
hash(U8 key)
{
	return U64(key);
}

inline static U64
hash(U16 key)
{
	return U64(key);
}

inline static U64
hash(U32 key)
{
	return U64(key);
}

inline static U64
hash(U64 key)
{
	return U64(key);
}

inline static U64
hash(F32 key)
{
	return hash_fnv_x32(&key, sizeof(F32));
}

inline static U64
hash(F64 key)
{
	return hash_fnv_x32(&key, sizeof(F64));
}