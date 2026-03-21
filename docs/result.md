# Result & Error

**Header:** `core/result.h`

`Error` is a simple value type carrying an optional formatted message. Functions that can fail return `Error` instead of throwing exceptions.

---

## Error

```cpp
#include <core/result.h>

// Return success
return Error{};

// Return failure
return Error{"failed to open file '{}'", path};
```

Check at call site:

```cpp
if (Error err = load_file("data.bin"))
{
    log_error("{}", err.message.data);
    return err;
}
```

`Error` converts to `bool`: `true` means failure (message is non-empty).

---

## Result\<T\>

For functions that return a value **or** an error:

```cpp
Result<Texture> load_texture(const char *path);

auto [texture, err] = load_texture("albedo.png");
if (err)
{
    log_error("texture load failed: {}", err.message.data);
    return err;
}
// use texture
```

---

## Pattern: propagate with early return

```cpp
Error init_renderer()
{
    if (Error err = create_device())   return err;
    if (Error err = create_swapchain()) return err;
    if (Error err = load_shaders())    return err;
    return Error{};
}
```

This is the idiomatic way to chain fallible operations — no exception overhead, no hidden control flow.
