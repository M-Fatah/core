#pragma once

#include "core/compiler/compiler.h"

template <typename T>
requires (is_same_v<T, U32> || is_same_v<T, U64>)
struct alignas(T) Atomic
{
	T value;
};

inline static constexpr Atomic<U32>
atomic_init(U32 value)
{
	return Atomic<U32>{.value = value};
}

inline static constexpr Atomic<U64>
atomic_init(U64 value)
{
	return Atomic<U64>{.value = value};
}

inline static U32
atomic_load(const Atomic<U32> &self, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	return compiler_atomic_load_u32(&self.value, order);
}

inline static U64
atomic_load(const Atomic<U64> &self, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	return compiler_atomic_load_u64(&self.value, order);
}

inline static void
atomic_store(Atomic<U32> &self, U32 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	compiler_atomic_store_u32(&self.value, value, order);
}

inline static void
atomic_store(Atomic<U64> &self, U64 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	compiler_atomic_store_u64(&self.value, value, order);
}

inline static U32
atomic_exchange(Atomic<U32> &self, U32 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	return compiler_atomic_exchange_u32(&self.value, value, order);
}

inline static U64
atomic_exchange(Atomic<U64> &self, U64 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	return compiler_atomic_exchange_u64(&self.value, value, order);
}

inline static bool
atomic_compare_exchange(Atomic<U32> &self, U32 &expected, U32 desired, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	return compiler_atomic_compare_exchange_u32(&self.value, expected, desired, order);
}

inline static bool
atomic_compare_exchange(Atomic<U64> &self, U64 &expected, U64 desired, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	return compiler_atomic_compare_exchange_u64(&self.value, expected, desired, order);
}

inline static U32
atomic_fetch_add(Atomic<U32> &self, U32 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	return compiler_atomic_fetch_add_u32(&self.value, value, order);
}

inline static U64
atomic_fetch_add(Atomic<U64> &self, U64 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	return compiler_atomic_fetch_add_u64(&self.value, value, order);
}

inline static U32
atomic_fetch_sub(Atomic<U32> &self, U32 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	return compiler_atomic_fetch_sub_u32(&self.value, value, order);
}

inline static U64
atomic_fetch_sub(Atomic<U64> &self, U64 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	return compiler_atomic_fetch_sub_u64(&self.value, value, order);
}