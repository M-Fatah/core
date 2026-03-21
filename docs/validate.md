# Validate

**Header:** `core/validate.h`

`validate` is a runtime assertion that includes the source file, line, and function name automatically.

---

## Usage

```cpp
#include <core/validate.h>

validate(index < count);
validate(ptr != nullptr, "pointer must not be null");
validate(size > 0, "size must be positive");
```

On failure it prints the condition, the message, and the source location, then aborts. In Debug builds this fires immediately. In Release builds the behaviour is configurable (currently also aborts).

---

## vs. `assert`

| | `assert` | `validate` |
|---|---|---|
| Source location | Condition string + line | File, line, function |
| Custom message | No | Yes |
| Release behavior | Strips out | Configurable |
| Used by containers | No | Yes (bounds checks) |

All container bounds checks (`array[i]`, `span[i]`, etc.) use `validate` internally.
