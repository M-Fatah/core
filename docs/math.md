# Math

Header-only linear-algebra + scalar math primitives. Type-prefixed free functions (`f32_sqrt`, `f32x3_dot`, `f32x4x4_look_at`); no namespaces, no templates outside the generic container types that happen to use math internally.

**Headers:** per-type, included individually (`core/math/f32.h`, `core/math/f32x3.h`, `core/math/f32x4x4.h`, `core/math/quaternion.h`, `core/math/random.h`, etc.). Pull in exactly what each translation unit uses.

---

## Canonical coordinate convention

Consistent across every platform. Documented once; enforced by the API.

| Property | Value |
|---|---|
| Handedness | Right-handed |
| World axes | `+X` right, `+Y` up, `+Z` toward the viewer |
| Axis constants | `F32X3_RIGHT = {1,0,0}`, `F32X3_UP = {0,1,0}`, `F32X3_FORWARD = {0,0,-1}` |
| Matrix storage | Row-major. `F32x4x4` layout is `[m00 m01 m02 m03 \| m10 m11 ... \| ...]` |
| Multiplication | **Row-vector**: `v * M`, not `M * v`. Translation lives in the last row. |
| Angle units | Radians unless the parameter name ends in `_degrees` |
| Clip / NDC | Y-up, Z in `[0, 1]` (Metal / D3D-native) |
| UV origin | Bottom-left |

### CPU ↔ GPU matrix layout (transpose duality)

Matrices upload to GPU buffers **as-is** — no transpose on upload, no `layout(row_major)` annotation on shader uniforms. This works because:

- CPU: row-major storage + row-vector multiply (`clip = position * mvp`).
- GPU: GLSL default is column-major storage + column-vector multiply (`clip = mvp * position`).

The "row-major vs column-major" memory reinterpretation and the "row-vector vs column-vector" multiplication order cancel exactly — same memory, same result.

**Do not** add `layout(row_major)` to shader matrix uniforms. It would re-introduce the mismatch.

---

## Type reference

All types are POD. No constructors, no destructors. Initialize via aggregate init: `F32x3{1.0f, 2.0f, 3.0f}`.

### Vectors

| Type | Lanes | Storage | Notes |
|---|---|---|---|
| `F32x2` | 2 × F32 | 8 B, scalar | |
| `F32x3` | 3 × F32 | 12 B packed | GPU-attribute interop |
| `F32x4` | 4 × F32 | 16 B SIMD-backed | `alignas(16)` |
| `F64x2` | 2 × F64 | 16 B SIMD-backed | `alignas(16)` |
| `F64x3` | 3 × F64 | 24 B packed | |
| `F64x4` | 4 × F64 | 32 B SIMD-backed | `alignas(32)`, one `__m256d` on AVX |
| `I32x2` / `I32x3` | scalar | packed | texture coords, grid indices |
| `I32x4` | 4 × I32 | 16 B SIMD-backed | SIMD masks, packed flags |
| `U32x2` / `U32x3` | scalar | packed | |
| `U32x4` | 4 × U32 | 16 B SIMD-backed | |

### Matrices

| Type | Size | Storage | Notes |
|---|---|---|---|
| `F32x2x2` | 16 B | scalar | Row-major `[m00 m01 / m10 m11]` |
| `F32x3x3` | 48 B | SIMD, 3 padded rows | Matches `std140` / MSL `matrix_float3x3` |
| `F32x4x4` | 64 B | SIMD, 4 rows | std140-compatible |
| `F64x2x2` | 32 B | scalar | |
| `F64x3x3` | 96 B | SIMD, 3 padded rows | |
| `F64x4x4` | 128 B | SIMD, 4 rows | |

### Other

| Type | Description |
|---|---|
| `Quaternion` | `{w, x, y, z}` unit-quaternion rotation |
| `Random` | Explicit-state xoshiro256** PRNG (256-bit state) |

### Constants

| Scope | Constants |
|---|---|
| Angular (F32) | `F32_PI`, `F32_TAU`, `F32_PI_OVER_2`, `F32_TO_DEGREES`, `F32_TO_RADIANS` |
| Angular (F64) | `F64_PI`, `F64_TAU`, `F64_PI_OVER_2`, `F64_TO_DEGREES`, `F64_TO_RADIANS` |
| Special values | `F32_EPSILON`, `F32_INFINITY`, `F32_NEG_INFINITY`, `F32_NAN` (+ `F64_*` mirror) |
| Limits | `F32_MIN`, `F32_MAX`, `I32_MIN`, `I32_MAX`, `U32_MAX`, ... (in `core/defines.h`) |
| Axis constants | `F32X3_RIGHT`, `F32X3_UP`, `F32X3_FORWARD`, `F32X3_LEFT`, `F32X3_DOWN`, `F32X3_BACKWARD`, `F32X3_ZERO`, `F32X3_ONE` |
| Identities | `QUATERNION_IDENTITY` |

---

## Usage

### Building a view-projection matrix and transforming a point

```cpp
#include <core/math/f32x3.h>
#include <core/math/f32x4x4.h>

F32x3 eye    = {0.0f, 0.0f, 5.0f};
F32x3 target = F32X3_ZERO;

F32x4x4 view = f32x4x4_look_at(eye, target, F32X3_UP);
F32x4x4 proj = f32x4x4_perspective(60.0f * F32_TO_RADIANS, 16.0f / 9.0f, 0.1f, 1000.0f);
F32x4x4 vp   = view * proj;

F32x4 world_pos = {1.0f, 2.0f, 3.0f, 1.0f};
F32x4 clip_pos  = world_pos * vp;          // row-vector convention
```

### Composing a TRS transform

```cpp
F32x3 translation = {5.0f, 0.0f, 0.0f};
Quaternion rotation = quaternion_from_axis_angle(F32X3_UP, 45.0f * F32_TO_RADIANS);
F32x3 scale = {2.0f, 2.0f, 2.0f};

// Apply in the order "scale first, then rotate, then translate" — reads left-to-right.
F32x4x4 trs = f32x4x4_scaling(scale)
            * f32x4x4_from_quaternion(rotation)
            * f32x4x4_translation(translation);

// Decompose back.
F32x3 out_t;
Quaternion out_r;
F32x3 out_s;
bool ok = f32x4x4_decompose(trs, &out_t, &out_r, &out_s);
```

### Rotating a vector by a quaternion

```cpp
Quaternion q = quaternion_from_axis_angle(F32X3_UP, F32_PI_OVER_2);

F32x3 rotated = F32X3_RIGHT * q;           // 90° yaw → forward direction
// rotated ≈ F32X3_FORWARD
```

### Interpolating

```cpp
// Scalar
F32 brightness = f32_lerp(0.0f, 1.0f, t);
F32 smoothed   = f32_smoothstep(0.2f, 0.8f, t);

// Vector
F32x3 pos = f32x3_lerp(start, end, t);

// Rotation — always use slerp, not lerp.
Quaternion r = quaternion_slerp(start_orientation, end_orientation, t);

// Camera follow with critical damping.
static F32 velocity = 0.0f;
camera.distance = f32_smooth_damp(camera.distance, target_distance, &velocity, 0.2f, dt);
```

### Screen-space projection / picking

```cpp
F32x4 viewport = {0.0f, 0.0f, (F32)window_w, (F32)window_h};

// World → screen.
F32x3 screen = f32x3_project(world_pos, vp, viewport);

// Mouse click → pick ray.
F32x4x4 vp_inv = f32x4x4_inverse(vp);
F32x3 near_pt = f32x3_unproject(F32x3{mouse_x, mouse_y, 0.0f}, vp_inv, viewport);
F32x3 far_pt  = f32x3_unproject(F32x3{mouse_x, mouse_y, 1.0f}, vp_inv, viewport);
F32x3 ray_dir = f32x3_normalize(far_pt - near_pt);
```

### Integer vectors

```cpp
U32x2 texture_size = {1920u, 1080u};
I32x3 cell = {grid_x, grid_y, grid_z};

U32x2 clamped = u32x2_min(texture_size, U32x2{4096u, 4096u});
```

### Random

```cpp
Random rng = random_from_seed(0xDEADBEEF);

F32 roll         = f32_random_range(rng, 0.0f, 1.0f);
I32 die          = i32_random_range(rng, 1, 6);
F32x3 in_sphere  = f32x3_random_in_unit_sphere(rng);
F32x3 on_surface = f32x3_random_on_unit_sphere(rng);
Quaternion q     = quaternion_random(rng);
```

Seeded and explicit-state — deterministic for replays, network sync, tests. No `srand()`-style hidden global.

### Formatted logging

Math types format natively via `core/formatter.h` (transitively available through `core/print.h` and `core/log.h`):

```cpp
log_info("Camera pos:  %",   camera.position);   // "{0.0, 1.5, 10.0}"
log_info("View matrix:\n %", view);              // row-by-row
log_info("Rotation:    %",   orientation);       // "{w=1.0, x=0.0, y=0.0, z=0.0}"
log_warn("Dirty texel %",    texel);             // "{10, 20}"
```

---

## SIMD

The 4-wide types (`F32x4`, `F32x3x3`, `F32x4x4`, `F64x2`, `F64x4`, `F64x3x3`, `F64x4x4`, `I32x4`, `U32x4`) are SIMD-backed. Call sites don't see the SIMD details — the storage is a union of scalar fields and the SIMD register, and ops dispatch at compile time.

| Arch | Baseline | Wrapper types |
|---|---|---|
| ARM64 (Apple Silicon) | NEON | `float32x4_t`, `float64x2_t`, `int32x4_t`, ... |
| x86_64 (Windows/Linux) | AVX (Sandy Bridge 2011+) | `__m128`, `__m128d`, `__m256d`, `__m128i` |

CMake sets `SIMD_NEON=1` or `SIMD_AVX=1` automatically. `-DCORE_SIMD_FORCE_SCALAR=ON` falls back to the scalar path for parity testing — same code, same results (within 1e-6 for non-associative ops like matmul).

Anything beyond the baseline (AVX2, AVX-512) is not in scope. The math library targets CPUs from 2011+.

---

## Field conventions

- All ops are free functions prefixed by the type: `f32x3_dot`, `f32x4x4_inverse`, `quaternion_slerp`.
- No hidden state. `f32_random_*` / `quaternion_random` take an explicit `Random &`.
- Angle parameters are radians unless named `_degrees`.
- `*_approx_equal(a, b, epsilon)` — always requires an explicit epsilon (no magic default).
- `*_length_squared` — name fixed from the older "norm" which was misleading (it returned the squared length, not the length).
