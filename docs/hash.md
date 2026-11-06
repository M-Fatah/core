# Hash

**Header:** `core/hash.h`

Generic hashing used by `Hash_Table` and `Hash_Set`.

---

## Built-in Overloads

All primitive types are covered out of the box: `bool`, `char`, `i8`–`i64`, `u8`–`u64`, `f32`, `f64`, raw pointers.

The pointer overload hashes the address without reading the pointed-to data. It mixes the address bits so aligned allocations spread across power-of-two table buckets. Mutable and const pointers to the same address produce the same hash. The single `hash(T *)` overload deduces `T` as const for a const pointer; explicitly supplied type arguments must retain that const qualification. Pointer hashes depend on the process address and are not persistent identifiers.

`String` and `String_View` have content-based hash overloads. Raw C string pointers use pointer hashing.

---

## Custom Type

Add an overload of `hash(const T &)` returning `u64`:

```cpp
struct Vec2 { float x, y; };

inline static u64
hash(const Vec2 &v)
{
    u64 h = hash(v.x);
    h ^= hash(v.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
}
```

Once defined, `Vec2` can be used as a `Hash_Table` key:

```cpp
auto table = hash_table_init<Vec2, int>();
hash_table_insert(table, Vec2{1.f, 2.f}, 42);
```

---

## Raw FNV hash

For hashing arbitrary bytes directly:

```cpp
u64 h = hash_fnv_x32(data_ptr, byte_count);
```