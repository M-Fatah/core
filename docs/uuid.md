# UUID

**Header:** `core/uuid.h`

`Uuid` is a 16-byte value type with operating-system-backed version 4 generation, canonical string parsing and formatting, equality, and hashing.

## Generation

Initialize a UUID and check whether operating-system entropy generation succeeded:

```cpp
auto [uuid, error] = uuid_init();
if (error)
	return;
```

`uuid_init` returns an error when operating-system entropy generation fails. Check the error before using the value. Generated UUIDs use RFC 9562 version 4 and variant 10 bit patterns. `Uuid{}` represents the standardized Nil UUID.

## Parsing and Formatting

`uuid_init_from` accepts exactly the canonical 36-byte `8-4-4-4-12` representation. Hexadecimal digits may be uppercase or lowercase. Invalid input returns an error describing the invalid length, separator, or hexadecimal character.

Check the error before using the value. A valid all-zero UUID returns no error and its value equals `Uuid{}`.

```cpp
auto [uuid, error] = uuid_init_from("550e8400-e29b-41d4-a716-446655440000");
if (error)
	return;
```

Core's generic `to_string` returns a caller-owned lowercase canonical string using the supplied allocator:

```cpp
String text = to_string(uuid);
DEFER(string_deinit(text));
```

UUIDs also work directly with Core formatting and hash containers:

```cpp
log_info("asset {}", uuid);

Hash_Set<Uuid> ids = hash_set_init<Uuid>();
DEFER(hash_set_deinit(ids));
hash_set_insert(ids, uuid);
```