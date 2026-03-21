# ECS (Entity Component System)

**Header:** `core/ecs.h`

A minimal, type-safe ECS built on `Hash_Table` and `Pool_Allocator`.

---

## Entities

```cpp
#include <core/ecs.h>

ecs::Entity e = ecs::entity_new();

if (e)   // Entity is valid
    ...

e.id     // underlying u64 — U64_MAX means invalid
```

---

## Components

Define any struct as a component — no base class or macro required:

```cpp
struct Transform { float x, y, z; };
struct Health    { int current, max; };
```

---

## World

```cpp
ecs::World world = ecs::world_init();
DEFER(ecs::world_deinit(world));
```

---

## Adding & Accessing Components

```cpp
ecs::Entity player = ecs::entity_new();

ecs::world_add_component(world, player, Transform{0.f, 0.f, 0.f});
ecs::world_add_component(world, player, Health{100, 100});

// Write
Transform *t = ecs::world_get_component<Transform>(world, player);
t->x += 1.f;

// Read-only
const Health *h = ecs::world_get_component<const Health>(world, player);
```

Returns `nullptr` if the entity doesn't have that component.

---

## Removing Components

```cpp
ecs::world_remove_component<Transform>(world, player);
```

---

## Iterating

```cpp
ecs::world_for_each<Transform, Health>(world, [](ecs::Entity e, Transform &t, Health &h) {
    t.y -= 9.8f;
    h.current -= 1;
});
```

Only entities that have **all** listed component types are visited.
