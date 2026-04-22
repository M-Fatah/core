# Print & Log

---

## Print

**Header:** `core/print.h`

`print_to` writes a formatted string to a `FILE *` or a custom callback.

```cpp
#include <core/print.h>

print_to(stdout, "Hello, {}!\n", name);
print_to(stderr, "Error: {}\n", message);

// With color
print_to(stdout, PRINT_COLOR_FG_GREEN, "OK\n");
```

### Colors

| Constant | Appearance |
|---|---|
| `PRINT_COLOR_DEFAULT` | Terminal default |
| `PRINT_COLOR_FG_RED` | Red text |
| `PRINT_COLOR_FG_GREEN` | Green text |
| `PRINT_COLOR_FG_YELLOW` | Yellow text |
| `PRINT_COLOR_FG_BLUE` | Blue text |
| `PRINT_COLOR_FG_WHITE_DIMMED` | Dimmed white |
| `PRINT_COLOR_BG_RED` | Red background |

### Custom Callback

```cpp
print_to([](PRINT_COLOR color, const char *msg) {
    my_gui_log(msg);
}, "Value: {}\n", value);
```

---

## Log

**Header:** `core/log.h`

Convenience wrappers over `print_to`. Each level has a prefix and color.

```cpp
#include <core/log.h>

log_debug("texture loaded: {}", path);    // [DEBUG]: ...  (blue, DEBUG builds only)
log_info("server started on port {}", port);  // [INFO]: ...   (dimmed)
log_warning("missing config key: {}", key);   // [WARNING]: ... (yellow, stderr)
log_error("failed to open file: {}", path);   // [ERROR]: ...   (red, stderr)
log_fatal("out of memory");                   // [FATAL]: ...   (red bg, stderr) — calls abort()
```

`log_debug` compiles to nothing in Release builds.
