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
