# **Core**
<!-- badges: start -->
[![Build status](https://github.com/M-Fatah/core/workflows/CI/badge.svg)](https://github.com/M-Fatah/core/actions?workflow=CI)
![Lines of code](https://img.shields.io/tokei/lines/github/M-Fatah/core)
<!-- badges: end -->
---

## **Introduction:**
Core is a C-like C++ collection of utilities used as a foundation when writing programs in a data oriented fashion.

It started as a learning process that eventually evolved to be a useful container library on top of C++.

Its written in C++20.

It is still a WIP, lots of breaking changes are expected to happen.

## **Code style:**
```C++
#include <core/defines.h>
#include <core/containers/array.h>

struct Vector3
{
    f32 x, y, z;
};

struct Vertex
{
    Vector3 position;
    Vector3 normal;
};

Array<Vertex> vertices = array_init<Vertex>(memory::heap_allocator());
DEFER(array_deinit(vertices));

array_push(vertices, Vertex{{1.0f, 2.0f, 3.0f}, {0.0f, 0.0f, 1.0f}});
```

## **Platforms:**
- Windows.
- Linux.
- Mac.

## **Prerequisites:**
#### **Windows:**
- Download and install [CMake](https://cmake.org/download/) (version 3.20 atleast).
#### **Linux:**
```
sudo apt update
sudo apt-get install -y cmake libx11-dev libxkbcommon-x11-dev libx11-xcb-dev zenity
```

## **Building:**
```
cmake -B build
cmake --build build --config Debug -j
```
Output is in `build/bin/${CONFIG}/` directory.