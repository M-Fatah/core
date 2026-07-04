#pragma once

#include "core/defines.h"

enum Compiler_Atomic_Memory_Order
{
	COMPILER_ATOMIC_MEMORY_ORDER_RELAXED,
	COMPILER_ATOMIC_MEMORY_ORDER_ACQUIRE,
	COMPILER_ATOMIC_MEMORY_ORDER_RELEASE,
	COMPILER_ATOMIC_MEMORY_ORDER_ACQUIRE_RELEASE,
	COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL,
};

#if COMPILER_MSVC
	#include "core/compiler/compiler_msvc.h"
#elif COMPILER_GCC
	#include "core/compiler/compiler_gcc.h"
#elif COMPILER_CLANG
	#include "core/compiler/compiler_clang.h"
#else
	#error [COMPILER]: Unsupported compiler.
#endif