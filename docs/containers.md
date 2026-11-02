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
| `array_from(view, allocator)` | Copy a borrowed range into an owned array; remove top-level const |
| `array_copy(arr, allocator)` | Shallow copy of an array (element-wise assignment) |
| `clone(arr, allocator)` | Deep copy (recursively clones class elements) |

`array_from` copies exactly the view count. For `String_View`, it produces an `Array<char>` with no extra terminator; use the string APIs when terminated storage is required. `array_copy` takes an `Array`, not a view.

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
| `array_front(arr)` | Reference to first element |
| `array_back(arr)` | Reference to last element |

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

### Functions

| Function | Description |
|---|---|
| `stack_array_push(arr, value)` | Append element (asserts if at capacity) |
| `stack_array_pop(arr)` | Remove and return last element |
| `stack_array_clear(arr)` | Set count = 0 |

---

## View\<T\>

**Header:** `core/containers/view.h`

A **non-owning view** over a contiguous sequence. Never allocates. No `_deinit`.
Use `view_from(...)` to make the borrowed lifetime explicit.
Use `String_View` (an alias for `View<const char>`) for borrowed text. A general view does not guarantee NUL termination; individual APIs such as string interning may provide that guarantee.

```cpp
#include <core/containers/view.h>

Array<int> arr = array_init_from<int>({1, 2, 3, 4, 5});
DEFER(array_deinit(arr));

View<int>       view  = view_from(arr);           // mutable view
View<const int> cview = view_from((const Array<int>&)arr); // read-only view
```

### Construction

| Function | Returns | Description |
|---|---|---|
| `view_from(T *data, U64 count)` | `View<T>` | From pointer + count |
| `view_from(T *first, T *last)` | `View<T>` | From pointer pair |
| `view_from(T (&arr)[N])` | `View<T>` | From C array |
| `view_from({1,2,3})` | `View<const T>` | Read-only view of an initializer list |
| `view_from(Array<T> &)` | `View<T>` | Mutable view of Array |
| `view_from(const Array<T> &)` | `View<const T>` | Read-only view of Array |
| `view_from(Stack_Array<T,N> &)` | `View<T>` | Mutable view of Stack\_Array |
| `view_from(const Stack_Array<T,N> &)` | `View<const T>` | Read-only view of Stack\_Array |
| `view_from(const char *)` | `String_View` | View of a C string without the null terminator |

### Functions

| Function | Description |
|---|---|
| `view_is_empty(view)` | `count == 0` |
| `view_front(view)` | Reference to first element |
| `view_back(view)` | Reference to last element |

Supports range-based `for` via `begin()` / `end()`.

Initializer-list and temporary single-value views are valid only for the current full expression. A function that stores a `View` must copy the data it cares about.
Functions taking `View<const T>` can receive initializer lists directly, such as `process({a, b, c})`.
`view_from` from a string literal or C string stops at the first null terminator. Use `view_from(data, count)` for raw character buffers.

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

Use `string_append(s, data, count)` for a byte range, including embedded nulls or data without a terminator. The `string_append(s, view)` overload accepts `String_View` and appends its full byte range. The count excludes the terminator maintained by the destination. Null data or a zero count is a no-op. The source must reference readable bytes outside the destination allocation and remain valid for the duration of the call.

Use `string_replace(s, start, count, replacement)` to replace a byte range. A zero count inserts; an empty replacement deletes. Replacement accepts a `String_View`, a `String`, a C string, or a pointer followed by its byte count. It preserves embedded nulls and the destination terminator, reuses capacity, and moves only the suffix. The range must lie within the initialized destination, and nonempty replacement data must remain readable outside its allocation for the duration of the call.

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

### Construction

| Function | Description |
|---|---|
| `hash_table_init<K,V>(allocator)` | Empty table |
| `hash_table_init_with_capacity<K,V>(n, allocator)` | Pre-allocated capacity |
| `hash_table_init_from<K,V>({...}, allocator)` | From initializer list of `Hash_Table_Entry<K,V>` |
| `hash_table_copy(table, allocator)` | Shallow copy |
| `clone(table, allocator)` | Deep copy (recursively clones class-type keys and values) |

### Functions

| Function | Description |
|---|---|
| `hash_table_insert(table, key, value)` | Insert or update |
| `hash_table_find(table, key)` | Returns `const Hash_Table_Entry<const K, V> *` or `nullptr` |
| `hash_table_contains(table, key)` | `true` if key exists |
| `hash_table_remove(table, key)` | Swap-remove (O(1), entry order not preserved) |
| `hash_table_remove_ordered(table, key)` | Ordered remove (O(n), preserves insertion order) |
| `hash_table_reserve(table, extra)` | Reserve additional capacity |
| `hash_table_clear(table)` | Remove all entries (keep allocation) |
| `destroy(table)` | Calls `destroy()` on class-type keys/values, then deinits |

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

### Construction

| Function | Description |
|---|---|
| `hash_set_init<K>(allocator)` | Empty set |
| `hash_set_init_with_capacity<K>(n, allocator)` | Pre-allocated capacity |
| `hash_set_init_from<K>({...}, allocator)` | From initializer list |
| `hash_set_copy(set, allocator)` | Shallow copy |
| `clone(set, allocator)` | Deep copy (recursively clones class-type keys) |

### Functions

| Function | Description |
|---|---|
| `hash_set_insert(set, key)` | Insert (no-op if already present) |
| `hash_set_find(set, key)` | Returns `const K *` or `nullptr` |
| `hash_set_contains(set, key)` | `true` if key exists |
| `hash_set_remove(set, key)` | Swap-remove (O(1)) |
| `hash_set_remove_ordered(set, key)` | Ordered remove (O(n)) |
| `hash_set_reserve(set, extra)` | Reserve additional capacity |
| `hash_set_clear(set)` | Remove all entries (keep allocation) |
| `destroy(set)` | Calls `destroy()` on class-type keys, then deinits |

---

## String\_Interner

**Header:** `core/containers/string_interner.h`

Interns a byte range once and returns a borrowed `String_View`. Equal byte ranges return the same `.data` pointer. The count preserves embedded NULs and excludes the additional NUL terminator stored at `data[count]`.

```cpp
String_Interner interner = string_interner_init();
DEFER(string_interner_deinit(interner));

String_View a = string_interner_intern(interner, "hello");
String_View b = string_interner_intern(interner, String_View {"hello!", 5});
assert(a.data == b.data);
assert(a.count == 5 && a.data[5] == '\0');
```

Accepts `String_View`, `String`, C strings, pointer pairs, or pointer/count. Only the C-string overload scans for a terminator. A miss copies directly into interner-owned storage; hits and range overloads allocate no temporary copy. Empty input returns a stable non-null pointer to an empty terminated string.

Returned bytes remain valid across table growth until `string_interner_deinit`. The interner owns and frees them using its supplied allocator. Views do not own their bytes and must not be freed by callers. Concurrent mutation requires caller-owned synchronization.

---

## Ring\_Buffer\<T\>

**Header:** `core/containers/ring_buffer.h`

A heap-allocated growable double-ended circular buffer. Supports efficient push and pop from both ends. Grows automatically (×1.5) by linearising its internal layout on reallocation.

Indexed access via `rb[i]` operates in logical order (`0` = front element) regardless of the internal `head` offset.

```cpp
#include <core/containers/ring_buffer.h>

auto rb = ring_buffer_init<int>();
DEFER(ring_buffer_deinit(rb));

ring_buffer_push_back(rb, 1);
ring_buffer_push_back(rb, 2);
ring_buffer_push_back(rb, 3);

ring_buffer_pop_front(rb);          // remove from front (FIFO)

int front = ring_buffer_front(rb);  // 2
int back  = ring_buffer_back(rb);   // 3
```

> Range-based `for` is **not** supported — the data is circular and raw pointer iteration would yield incorrect results.
> Use indexed access instead: `for (u64 i = 0; i < rb.count; ++i) rb[i]`

### Construction

| Function | Description |
|---|---|
| `ring_buffer_init<T>(allocator)` | Empty, zero capacity |
| `ring_buffer_copy(rb, allocator)` | Shallow copy, linearised (`head = 0`) |
| `clone(rb, allocator)` | Deep copy (recursively clones class-type elements) |

### Modification

| Function | Description |
|---|---|
| `ring_buffer_push_back(rb, value)` | Append element at back |
| `ring_buffer_push_front(rb, value)` | Prepend element at front |
| `ring_buffer_pop_front(rb)` | Remove element from front |
| `ring_buffer_pop_back(rb)` | Remove element from back |
| `ring_buffer_reserve(rb, extra)` | Reserve additional capacity (linearises on reallocation) |
| `ring_buffer_clear(rb)` | Set count = 0, reset head (no deallocation) |

### Query

| Function | Description |
|---|---|
| `ring_buffer_front(rb)` | Reference to front element (`rb[0]`) |
| `ring_buffer_back(rb)` | Reference to back element (`rb[count-1]`) |
| `ring_buffer_is_empty(rb)` | `count == 0` |