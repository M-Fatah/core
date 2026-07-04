#pragma once

inline static int
_compiler_atomic_memory_order(Compiler_Atomic_Memory_Order order)
{
	switch (order)
	{
		case COMPILER_ATOMIC_MEMORY_ORDER_RELAXED:         return __ATOMIC_RELAXED;
		case COMPILER_ATOMIC_MEMORY_ORDER_ACQUIRE:         return __ATOMIC_ACQUIRE;
		case COMPILER_ATOMIC_MEMORY_ORDER_RELEASE:         return __ATOMIC_RELEASE;
		case COMPILER_ATOMIC_MEMORY_ORDER_ACQUIRE_RELEASE: return __ATOMIC_ACQ_REL;
		case COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL:      return __ATOMIC_SEQ_CST;
	}
	return __ATOMIC_SEQ_CST;
}

inline static int
_compiler_atomic_compare_exchange_failure_memory_order(Compiler_Atomic_Memory_Order order)
{
	switch (order)
	{
		case COMPILER_ATOMIC_MEMORY_ORDER_RELEASE:         return __ATOMIC_RELAXED;
		case COMPILER_ATOMIC_MEMORY_ORDER_ACQUIRE_RELEASE: return __ATOMIC_ACQUIRE;
		default:                                           return _compiler_atomic_memory_order(order);
	}
}

inline static int
_compiler_atomic_load_memory_order(Compiler_Atomic_Memory_Order order)
{
	switch (order)
	{
		case COMPILER_ATOMIC_MEMORY_ORDER_RELAXED:    return __ATOMIC_RELAXED;
		case COMPILER_ATOMIC_MEMORY_ORDER_ACQUIRE:    return __ATOMIC_ACQUIRE;
		case COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL: return __ATOMIC_SEQ_CST;
		default:                                      return __ATOMIC_SEQ_CST;
	}
}

inline static int
_compiler_atomic_store_memory_order(Compiler_Atomic_Memory_Order order)
{
	switch (order)
	{
		case COMPILER_ATOMIC_MEMORY_ORDER_RELAXED:    return __ATOMIC_RELAXED;
		case COMPILER_ATOMIC_MEMORY_ORDER_RELEASE:    return __ATOMIC_RELEASE;
		case COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL: return __ATOMIC_SEQ_CST;
		default:                                      return __ATOMIC_SEQ_CST;
	}
}

inline static U32
compiler_atomic_load_u32(const U32 *target, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	return __atomic_load_n(target, _compiler_atomic_load_memory_order(order));
}

inline static U64
compiler_atomic_load_u64(const U64 *target, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	return __atomic_load_n(target, _compiler_atomic_load_memory_order(order));
}

inline static void
compiler_atomic_store_u32(U32 *target, U32 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	__atomic_store_n(target, value, _compiler_atomic_store_memory_order(order));
}

inline static void
compiler_atomic_store_u64(U64 *target, U64 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	__atomic_store_n(target, value, _compiler_atomic_store_memory_order(order));
}

inline static U32
compiler_atomic_exchange_u32(U32 *target, U32 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	return __atomic_exchange_n(target, value, _compiler_atomic_memory_order(order));
}

inline static U64
compiler_atomic_exchange_u64(U64 *target, U64 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	return __atomic_exchange_n(target, value, _compiler_atomic_memory_order(order));
}

inline static bool
compiler_atomic_compare_exchange_u32(U32 *target, U32 &expected, U32 desired, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	return __atomic_compare_exchange_n(target, &expected, desired, false, _compiler_atomic_memory_order(order), _compiler_atomic_compare_exchange_failure_memory_order(order));
}

inline static bool
compiler_atomic_compare_exchange_u64(U64 *target, U64 &expected, U64 desired, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	return __atomic_compare_exchange_n(target, &expected, desired, false, _compiler_atomic_memory_order(order), _compiler_atomic_compare_exchange_failure_memory_order(order));
}

inline static U32
compiler_atomic_fetch_add_u32(U32 *target, U32 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	return __atomic_fetch_add(target, value, _compiler_atomic_memory_order(order));
}

inline static U64
compiler_atomic_fetch_add_u64(U64 *target, U64 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	return __atomic_fetch_add(target, value, _compiler_atomic_memory_order(order));
}

inline static U32
compiler_atomic_fetch_sub_u32(U32 *target, U32 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	return __atomic_fetch_sub(target, value, _compiler_atomic_memory_order(order));
}

inline static U64
compiler_atomic_fetch_sub_u64(U64 *target, U64 value, Compiler_Atomic_Memory_Order order = COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL)
{
	return __atomic_fetch_sub(target, value, _compiler_atomic_memory_order(order));
}