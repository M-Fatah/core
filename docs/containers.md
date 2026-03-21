# Containers

All containers follow the same conventions:

- **`_init`** — construct (may allocate).
- **`_deinit`** — destroy (free memory).
- **`_copy`** — shallow copy into a new container (element-wise assignment; nested pointers/allocations are not duplicated).
- **`clone()`** — deep copy; recursively invokes `clone()` on each class-type element. Requires a `clone()` overload for custom element types.
- Free functions take the container as the first argument.
- Every container that owns memory accepts a `memory::Allocator *` (defaults to `heap_allocator()`).

---

## Array\<T\>

**Header:** `core/containers/array.h`

A heap-allocated dynamic array. Owns its memory.

```cpp
#include <core/containers/array.h>
#include <core/defer.h>

auto arr = array_init<int>();
DEFER(array_deinit(arr));

array_push(arr, 10);
array_push(arr, 20);
array_push(arr, 30);

for (int x : arr)          // range-based for supported
    print_to(stdout, "{}\n", x);

arr[0] = 99;               // bounds-checked in Debug
```

### Construction

| Function | Description |
|---|---|
| `array_init<T>(allocator)` | Empty, zero capacity |
| `array_init_with_capacity<T>(n, allocator)` | Empty, pre-allocated capacity |
| `array_init_with_count<T>(n, allocator)` | Count == capacity, uninitialized |
| `array_init_from(first, last, allocator)` | Copy from pointer range |
| `array_init_from({1,2,3}, allocator)` | Copy from initializer list |
| `array_copy(arr, allocator)` | Shallow copy (element-wise assignment) |
| `clone(arr, allocator)` | Deep copy (recursively clones class elements) |

### Modification

| Function | Description |
|---|---|
| `array_push(arr, value)` | Append element |
| `array_push(arr, value, count)` | Append same value N times |
| `array_pop(arr)` | Remove and return last element |
| `array_remove(arr, index)` | Swap-remove (O(1), unordered) |
| `array_remove_if(arr, pred)` | Swap-remove matching elements |
| `array_remove_ordered(arr, index)` | Ordered remove (O(n)) |
| `array_append(arr, other)` | Append all elements of another array |
| `array_fill(arr, value)` | Set all elements to value |
| `array_clear(arr)` | Set count = 0 (no deallocation) |
| `array_resize(arr, n)` | Resize count (grows if needed) |
| `array_reserve(arr, extra)` | Reserve additional capacity |

### Query

| Function | Description |
|---|---|
| `array_is_empty(arr)` | `count == 0` |
| `array_first(arr)` | Reference to first element |
| `array_last(arr)` | Reference to last element |

---

## Stack\_Array\<T, N\>

**Header:** `core/containers/stack_array.h`

A fixed-capacity array stored entirely on the stack. No allocation, no `_deinit` needed.

```cpp
#include <core/containers/stack_array.h>

Stack_Array<int, 8> arr{};
stack_array_push(arr, 1);
stack_array_push(arr, 2);

int last = stack_array_pop(arr);   // 2
stack_array_clear(arr);
```

Supports range-based `for` via `begin()` / `end()`.

---

## Span\<T\>

**Header:** `core/containers/span.h`

A **non-owning view** over a contiguous sequence. Never allocates. No `_deinit`.

```cpp
#include <core/containers/span.h>

Array<int> arr = array_init_from<int>({1, 2, 3, 4, 5});
DEFER(array_deinit(arr));

Span<int>       view  = span_init(arr);           // mutable view
Span<const int> cview = span_init((const Array<int>&)arr); // read-only view
```

### Construction

| Function | Returns | Description |
|---|---|---|
| `span_init(T *data, u64 count)` | `Span<T>` | From pointer + count |
| `span_init(T *first, T *last)` | `Span<T>` | From pointer pair |
| `span_init(T (&arr)[N])` | `Span<T>` | From C array |
| `span_init(Array<T> &)` | `Span<T>` | Mutable view of Array |
| `span_init(const Array<T> &)` | `Span<const T>` | Read-only view of Array |
| `span_init(Stack_Array<T,N> &)` | `Span<T>` | Mutable view of Stack\_Array |
| `span_init(const char *)` | `Span<const char>` | View of a C string (no null) |
| `span_init({1,2,3})` | `Span<const T>` | From initializer\_list — **only safe as a function argument**, never store in a variable |

### Functions

| Function | Description |
|---|---|
| `span_is_empty(span)` | `count == 0` |
| `span_first(span)` | Reference to first element |
| `span_last(span)` | Reference to last element |

> **Lifetime rule for initializer\_list:** The backing array of `std::initializer_list` is a temporary. It lives only for the duration of the enclosing full-expression. Never store a `Span` constructed from `{}` in a named variable — pass it directly as a function argument.

---

## String

**Header:** `core/containers/string.h`

`String` is a typedef for `Array<char>`. It always carries a null terminator at `data[count]` (not counted in `count`).

```cpp
#include <core/containers/string.h>

String s = string_from("hello");
DEFER(string_deinit(s));

string_append(s, " world");
print_to(stdout, "{}\n", s.data);   // "hello world"
```

### Construction

| Function | Description |
|---|---|
| `string_init(allocator)` | Empty string with null terminator |
| `string_from(c_string, allocator)` | Copy from `const char *` |
| `string_from(first, last, allocator)` | Copy from pointer range |
| `string_literal(c_string)` | Non-owning view (no allocation, no `_deinit`) |
| `string_copy(str, allocator)` | Copy into a new allocation (byte-for-byte) |

### Common operations

```cpp
string_append(s, other);          // append String
string_append(s, "suffix");       // append c-string
string_push(s, 'x');              // append single char
string_clear(s);                  // reset count, keep allocation
u64 len = string_length(s);       // same as s.count
bool eq  = string_equal(s, other);
```

---

## Hash\_Table\<K, V\>

**Header:** `core/containers/hash_table.h`

An open-addressing hash table with tombstone deletion.

```cpp
#include <core/containers/hash_table.h>

auto table = hash_table_init<String, int>();
DEFER(hash_table_deinit(table));

hash_table_insert(table, string_literal("apples"), 5);
hash_table_insert(table, string_literal("bananas"), 3);

if (auto *entry = hash_table_find(table, string_literal("apples")))
    print_to(stdout, "apples: {}\n", entry->value);

hash_table_remove(table, string_literal("apples"));
```

Operator `[]` also works and inserts a default value if the key is absent:

```cpp
table[string_literal("pears")] = 7;
```

Iterate with range-based `for` — yields `Hash_Table_Entry<K, V>` references:

```cpp
for (auto &entry : table)
    print_to(stdout, "{} = {}\n", entry.key.data, entry.value);
```

Custom types need a `hash()` overload — see [Hash](hash.md).

---

## Hash\_Set\<K\>

**Header:** `core/containers/hash_set.h`

A `Hash_Table<K, Hash_Set_Value>` alias. Same API minus values.

```cpp
#include <core/containers/hash_set.h>

auto set = hash_set_init<int>();
DEFER(hash_set_deinit(set));

hash_set_insert(set, 42);
hash_set_insert(set, 99);

bool has = hash_set_contains(set, 42);  // true
hash_set_remove(set, 42);
```

---

## String\_Interner

**Header:** `core/containers/string_interner.h`

Deduplicates strings — equal strings are stored once and return the same `const char *` pointer. Pointer equality replaces string comparison.

```cpp
#include <core/containers/string_interner.h>

String_Interner interner = string_interner_init(memory::heap_allocator());
DEFER(string_interner_deinit(interner));

const char *a = string_interner_intern(interner, "hello");
const char *b = string_interner_intern(interner, "hello");

assert(a == b);  // same pointer
```

Also accepts a pointer range:

```cpp
const char *s = string_interner_intern(interner, first, last);
```
