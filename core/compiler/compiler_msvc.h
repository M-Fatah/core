#pragma once

#include <intrin.h>

inline static U32
compiler_atomic_load_u32(const U32 *target, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	// TODO: Map order to MSVC ARM/ARM64 _acq/_rel/_nf intrinsics after CMake exposes architecture capability macros.
	unused(order);
	return (U32)_InterlockedCompareExchange((volatile long *)target, 0, 0);
}

inline static U64
compiler_atomic_load_u64(const U64 *target, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	unused(order);
	return (U64)_InterlockedCompareExchange64((volatile __int64 *)target, 0, 0);
}

inline static void
compiler_atomic_store_u32(U32 *target, U32 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	unused(order);
	_InterlockedExchange((volatile long *)target, (long)value);
}

inline static void
compiler_atomic_store_u64(U64 *target, U64 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	unused(order);
	_InterlockedExchange64((volatile __int64 *)target, (__int64)value);
}

inline static U32
compiler_atomic_exchange_u32(U32 *target, U32 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	unused(order);
	return (U32)_InterlockedExchange((volatile long *)target, (long)value);
}

inline static U64
compiler_atomic_exchange_u64(U64 *target, U64 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	unused(order);
	return (U64)_InterlockedExchange64((volatile __int64 *)target, (__int64)value);
}

inline static bool
compiler_atomic_compare_exchange_u32(U32 *target, U32 &expected, U32 desired, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	unused(order);
	long original = _InterlockedCompareExchange((volatile long *)target, (long)desired, (long)expected);
	if ((U32)original == expected)
		return true;
	expected = (U32)original;
	return false;
}

inline static bool
compiler_atomic_compare_exchange_u64(U64 *target, U64 &expected, U64 desired, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	unused(order);
	__int64 original = _InterlockedCompareExchange64((volatile __int64 *)target, (__int64)desired, (__int64)expected);
	if ((U64)original == expected)
		return true;
	expected = (U64)original;
	return false;
}

inline static U32
compiler_atomic_fetch_add_u32(U32 *target, U32 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	U32 expected = compiler_atomic_load_u32(target, order);
	while (true)
	{
		U32 desired = expected + value;
		if (compiler_atomic_compare_exchange_u32(target, expected, desired, order))
			return expected;
	}
}

inline static U64
compiler_atomic_fetch_add_u64(U64 *target, U64 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	U64 expected = compiler_atomic_load_u64(target, order);
	while (true)
	{
		U64 desired = expected + value;
		if (compiler_atomic_compare_exchange_u64(target, expected, desired, order))
			return expected;
	}
}

inline static U32
compiler_atomic_fetch_sub_u32(U32 *target, U32 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	U32 expected = compiler_atomic_load_u32(target, order);
	while (true)
	{
		U32 desired = expected - value;
		if (compiler_atomic_compare_exchange_u32(target, expected, desired, order))
			return expected;
	}
}

inline static U64
compiler_atomic_fetch_sub_u64(U64 *target, U64 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	U64 expected = compiler_atomic_load_u64(target, order);
	while (true)
	{
		U64 desired = expected - value;
		if (compiler_atomic_compare_exchange_u64(target, expected, desired, order))
			return expected;
	}
}