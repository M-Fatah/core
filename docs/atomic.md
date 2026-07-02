**Header:** `core/atomic.h`

Small typed atomic wrapper for Core integer counters and ids.

Compiler-specific intrinsic details live in `core/compiler/compiler.h`; this header keeps the typed public API small.

---

## Supported Types

```cpp
Atomic<U32> state = atomic_init((U32)0);
Atomic<U64> counter = atomic_init((U64)100);
```

`Atomic<T>` currently accepts `U32` and `U64`.

---

## Operations

```cpp
U64 old_value = atomic_fetch_add(counter, (U64)1, COMPILER_ATOMIC_MEMORY_ORDER_RELAXED);
U64 value = atomic_load(counter);

atomic_store(counter, (U64)0);
old_value = atomic_exchange(counter, (U64)10);
```

All operations default to `COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL`, the strongest ordering. Pass a weaker order when the code only needs atomicity, for example relaxed counters or unique id generation.

Available orders:

```cpp
COMPILER_ATOMIC_MEMORY_ORDER_RELAXED
COMPILER_ATOMIC_MEMORY_ORDER_ACQUIRE
COMPILER_ATOMIC_MEMORY_ORDER_RELEASE
COMPILER_ATOMIC_MEMORY_ORDER_ACQUIRE_RELEASE
COMPILER_ATOMIC_MEMORY_ORDER_SEQUENTIAL
```

---

## Compare Exchange

```cpp
U32 expected = 0;
if (atomic_compare_exchange(state, expected, 1))
{
	// state changed from 0 to 1.
}
else
{
	// expected now contains the value observed in state.
}
```

`atomic_compare_exchange` takes `expected` by reference. On success, the atomic value is replaced with `desired`; on failure, `expected` is overwritten with the value that was actually observed.