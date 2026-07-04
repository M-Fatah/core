**Header:** `core/compiler/compiler.h`

Small compiler-specific primitive layer.

This header is where Core hides compiler intrinsic differences behind one naming style. Higher-level headers can include this instead of carrying `COMPILER_MSVC`, `COMPILER_CLANG`, or `COMPILER_GCC` branches directly.

`core/compiler/compiler.h` is the facade. It selects one backend header:

```text
core/compiler/compiler_msvc.h
core/compiler/compiler_clang.h
core/compiler/compiler_gcc.h
```

---

## Atomics

`compiler.h` provides low-level atomic primitives for `U32` and `U64`:

```cpp
U64 value = 0;
U64 old_value = compiler_atomic_fetch_add_u64(&value, 1, COMPILER_ATOMIC_MEMORY_ORDER_RELAXED);
```

Most user code should prefer `core/atomic.h`:

```cpp
Atomic<U64> counter = atomic_init((U64)0);
atomic_fetch_add(counter, 1, COMPILER_ATOMIC_MEMORY_ORDER_RELAXED);
```

Use the compiler primitives when implementing Core utilities that need direct access to the storage-level operation.

---

## Memory Order

`Compiler_Atomic_Memory_Order` is defined here because the compiler layer maps it to the actual compiler intrinsic memory model.

```cpp
COMPILER_ATOMIC_MEMORY_ORDER_RELAXED
COMPILER_ATOMIC_MEMORY_ORDER_ACQUIRE
COMPILER_ATOMIC_MEMORY_ORDER_RELEASE
COMPILER_ATOMIC_MEMORY_ORDER_ACQUIRE_RELEASE
COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL
```

The default is `COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL`.