# Reflect

**Header:** `core/reflect.h`

Compile-time type reflection without macros or external codegen. Supports primitives, pointers, arrays, structs, and enums.

---

## Type Names

```cpp
#include <core/reflect.h>

const char *name = name_of<int>();        // "i32"
const char *name = name_of<float>();      // "f32"
const char *name = name_of<MyStruct>();   // "MyStruct"
const char *name = name_of<MyStruct*>();  // "MyStruct*"
```

---

## Type Kinds

```cpp
Type_Info info = type_of<MyStruct>();

switch (info.kind)
{
    case TYPE_KIND_STRUCT: ...
    case TYPE_KIND_ENUM:   ...
    case TYPE_KIND_I32:    ...
    // etc.
}
```

Full list of `TYPE_KIND` values covers all primitives, pointers, C arrays, structs, and enums.

---

## Struct Fields

```cpp
struct Vertex {
    float x, y, z;
};

Type_Info info = type_of<Vertex>();
for (const Type_Field &field : info.fields)
{
    print_to(stdout, "field: {} offset: {}\n", field.name, field.offset);
}
```

Fields are discovered via structured bindings — the struct must be **aggregate-initializable**.

---

## Enums

Use the `REFLECT_ENUM` macro to register enum values:

```cpp
enum Direction { NORTH, SOUTH, EAST, WEST };
REFLECT_ENUM(Direction, NORTH, SOUTH, EAST, WEST);
```

Then iterate:

```cpp
Type_Info info = type_of<Direction>();
for (const Type_Enum_Value &v : info.enum_values)
    print_to(stdout, "{} = {}\n", v.name, v.value);
```

Supports negative values, non-contiguous values, and duplicate values.

---

## Limits

| Constant | Default |
|---|---|
| `REFLECT_MAX_NAME_LENGTH` | 128 |
| `REFLECT_MIN_ENUM_VALUE` | -32 |
| `REFLECT_MAX_ENUM_VALUE` | 64 |

These can be adjusted by modifying the header if your enums go outside this range.
