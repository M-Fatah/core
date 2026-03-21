# Formatter

**Header:** `core/formatter.h`

Type-safe string formatting using `{}` placeholders. No `printf`-style format strings — the type is known at compile time.

---

## `format()`

Returns a `String` (heap-allocated by default):

```cpp
#include <core/formatter.h>

String s = format("Hello, {}! You are {} years old.", name, age);
DEFER(string_deinit(s));
```

Pass an explicit allocator as the **last** argument:

```cpp
String s = format("x = {}", x, memory::temp_allocator());
```

---

## Format Specifiers

Specifiers go inside the braces: `{specifier}` or `{specifier:options}`.

| Specifier | Meaning | Example |
|---|---|---|
| _(none)_ | Default representation | `{}` |
| `d` | Decimal integer | `{d}` |
| `x` | Hex lowercase | `{x}` → `ff` |
| `X` | Hex uppercase | `{X}` → `FF` |
| `b` | Binary | `{b}` → `1010` |
| `o` | Octal | `{o}` |
| `p` | Pointer | `{p}` |
| `c` | Character | `{c}` |

### Width & Alignment

```cpp
format("{<10}", "left");    // left-aligned in 10 chars
format("{>10}", "right");   // right-aligned
format("{^10}", "center");  // centered
format("{010d}", 42);       // zero-padded: "0000000042"
```

### Precision (floats)

```cpp
format("{.2}", 3.14159f);   // "3.14"
format("{.4}", 3.14159f);   // "3.1416"
```

---

## `Formatter` — Incremental Building

Use `Formatter` when building a string in multiple steps:

```cpp
#include <core/formatter.h>

Formatter fmt = formatter_init();
DEFER(formatter_deinit(fmt));

format_value(fmt, "Name: ");
format_value(fmt, player.name);
format_value(fmt, ", Score: ");
format_value(fmt, player.score);

print_to(stdout, "{}\n", fmt.buffer.data);
```

---

## Custom Type Formatting

Add a `format_value(Formatter &, const T &, Format_Options)` overload:

```cpp
struct Vec3 { float x, y, z; };

inline static void
format_value(Formatter &fmt, const Vec3 &v, Format_Options options = {})
{
    format_value(fmt, '(');
    format_value(fmt, v.x, options);
    format_value(fmt, ", ");
    format_value(fmt, v.y, options);
    format_value(fmt, ", ");
    format_value(fmt, v.z, options);
    format_value(fmt, ')');
}
```

Then:

```cpp
Vec3 pos = {1.0f, 2.0f, 3.0f};
String s = format("pos = {}", pos);
// "pos = (1.000000, 2.000000, 3.000000)"
```
