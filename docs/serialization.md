# Serialization

**Header:** `core/serialization/serializer.h`

Two concrete serializers are provided: binary and JSON. Both share the same `serialize()` interface so you can swap them freely.

---

## Interface

```cpp
template <typename S>
Error serialize(S &serializer, const char *name, T &value);
```

- `S` is `Binary_Serializer` or `Json_Serializer`.
- Returns `Error{}` on success, a descriptive `Error` on failure.

---

## Binary Serializer

**Header:** `core/serialization/binary_serializer.h`

```cpp
#include <core/serialization/binary_serializer.h>

// Write
Binary_Serializer writer = binary_serializer_init_writer("save.bin");
DEFER(binary_serializer_deinit(writer));

serialize(writer, "health", player.health);
serialize(writer, "position", player.position);
```

```cpp
// Read
Binary_Serializer reader = binary_serializer_init_reader("save.bin");
DEFER(binary_serializer_deinit(reader));

serialize(reader, "health", player.health);
serialize(reader, "position", player.position);
```

---

## JSON Serializer

**Header:** `core/serialization/json_serializer.h`

The JSON value API in `core/json.h` reads and writes every JSON root kind: objects, arrays, strings, numbers, booleans, and null. The writer escapes quotation marks, backslashes, and control characters in values and object member names. The parser decodes all JSON escapes, including Unicode surrogate pairs. Strings contain validated UTF-8; embedded nulls decoded from `\u0000` remain part of their byte counts.

`json_value_from_string(Slice<const char>, allocator)` accepts an exact byte range without requiring a terminator. The `String` overload also respects its count; the C-string overload stops at its terminator. Parsing consumes the entire input except JSON whitespace. Comments, trailing data or commas, invalid number syntax, raw control characters, malformed UTF-8, and unpaired surrogates return an error. This removes the previous permissive comment and number extensions. Parse errors include a line and byte column, and release partially constructed values.

Parsing, writing, copying, and destruction traverse nested values iteratively using Core arrays. There is no fixed nesting limit or depth parameter; traversal storage grows with the active nesting depth instead of the call stack. Available memory still bounds the input and value tree. The pretty writer retains tab indentation, so deeply nested output can be much larger than its compact input. Failed parses return an invalid value; failed writes return an empty string. Both can be passed to their normal cleanup functions.

Numbers remain `F64`. Conversion uses the C numeric locale without changing the process locale, and serialization emits 17 significant digits so finite values round-trip without the formatter's six-decimal truncation. Values outside the finite `F64` range are rejected; very small values may round to a subnormal or signed zero. Integers beyond the exact `F64` range are not preserved exactly. NaN, infinity, invalid value kinds, and invalid UTF-8 cannot be serialized.

`json_value_copy` creates an independent tree, using the supplied allocator for every nested container, key, and string. Returned trees and strings use the caller's allocator; traversal stacks and file input buffers use separate local arenas. Number conversion, destruction, and file output reclaim temporary storage with scoped arena marks, preserving earlier temporary allocations.

Object insertion copies the member name using the object's allocator and takes ownership of the supplied value. Supply an independently owned value, or use `json_value_copy` for borrowed data. Replacing a member releases its previous value. During parsing, duplicate names are compared after escape decoding and the last value wins. `json_value_object_find` returns a borrowed value; it remains valid until the owning member is replaced or destroyed.

```cpp
#include <core/serialization/json_serializer.h>

// Write
Json_Serializer writer = json_serializer_init_writer("config.json");
DEFER(json_serializer_deinit(writer));

serialize(writer, "width", config.width);
serialize(writer, "height", config.height);
serialize(writer, "fullscreen", config.fullscreen);
```

```cpp
// Read
Json_Serializer reader = json_serializer_init_reader("config.json");
DEFER(json_serializer_deinit(reader));

serialize(reader, "width", config.width);
serialize(reader, "height", config.height);
serialize(reader, "fullscreen", config.fullscreen);
```

---

## Serializing Structs

Use `serialize` with an `initializer_list` of `Serialize_Pair`:

```cpp
struct Config { int width; int height; bool fullscreen; };

template <typename S>
Error serialize(S &s, const char *name, Config &cfg)
{
    return serialize(s, {
        {"width",      cfg.width},
        {"height",     cfg.height},
        {"fullscreen", cfg.fullscreen},
    });
}
```

---

## Custom Types

Specialize `serialize` for your type:

```cpp
template <typename S>
Error serialize(S &s, const char *name, Vec3 &v)
{
    return serialize(s, {
        {"x", v.x},
        {"y", v.y},
        {"z", v.z},
    });
}
```