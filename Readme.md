# vulkan-c

A learning project that explores the Vulkan graphics API, written entirely in C. It follows the [vulkan-tutorial.com](https://vulkan-tutorial.com) guide, but avoids C++ — no classes, no standard library containers, just plain C.

---

## What is this?

**Vulkan** is a low-level graphics API made by the Khronos Group. Unlike older APIs like OpenGL, Vulkan gives the programmer direct control over the GPU — which makes it faster and more predictable, but also more work to set up. This project is a from-scratch implementation of the classic "hello world" of graphics programming.

**Why C instead of C++?** The official tutorial uses C++, but Vulkan itself is a C API. This project proves you don't need C++ to use it.

---

## What it does

- Opens a window and renders a textured quad using the GPU
- Sets up the full Vulkan rendering pipeline from scratch — instance, device, swapchain, shaders, framebuffers, command buffers, and synchronization
- Loads and applies a BMP texture using a custom image library (`img_lib`) written from scratch — no third-party image loaders
- Handles window resizing gracefully
- Uses a staging buffer to upload geometry and texture data to fast GPU memory
- Runs validation layers in debug mode to catch mistakes early

---

## Dependencies

These are the tools and libraries needed to build and run the project.

| Dependency | Version used | What it does |
|---|---|---|
| [Vulkan SDK](https://vulkan.lunarg.com/) | 1.3.x | The core graphics API and validation tools |
| [GLFW](https://www.glfw.org/) | 3.3.x | Creates the window and connects it to Vulkan |
| [glslc](https://github.com/google/shaderc) | 2024.x | Compiles shader code into a format the GPU understands |
| [cglm](https://github.com/recp/cglm) | 0.9.x | Math library for vectors and matrices |
| GCC / Clang | 13.x / 16.x | C compiler |

### Installing on Arch Linux

```bash
sudo pacman -S glfw-wayland vulkan-tools vulkan-validation-layers shaderc cglm
```

> Use `glfw-x11` instead of `glfw-wayland` if you are on X11.

### Checking your installed versions

```bash
glslc --version                              # shader compiler
vulkaninfo | grep "Vulkan Instance Version"  # Vulkan runtime
pkg-config --modversion glfw3               # GLFW
gcc --version                                # C compiler
valgrind --version                           # memory checker
```

---

## Project Structure

```
.
├── Makefile
├── shaders/
│   ├── shader.vert       # vertex shader source (GLSL)
│   ├── shader.frag       # fragment shader source (GLSL)
│   ├── vert.spv          # compiled vertex shader (generated)
│   └── frag.spv          # compiled fragment shader (generated)
├── src/
│   ├── main.c            # all Vulkan application code
│   ├── img_lib.c         # custom BMP image loader
│   └── img_lib.h         # image loader header
└── texture/
    └── texture.bmp       # texture image (original photo, all rights reserved by author)
```

Shaders are small programs that run on the GPU. The vertex shader positions each point of the shape, and the fragment shader decides the colour of each pixel. They are written in GLSL and compiled to SPIR-V (`.spv`) before use.

`img_lib` is a lightweight BMP loader written from scratch for this project. It supports 8, 24, and 32-bit BMP files and outputs raw RGBA pixel data ready for direct upload to a Vulkan staging buffer. No third-party image libraries are used.

---

## Building and Running

**Step 1 — Compile the shaders** (only needed once, or when shaders change):
```bash
make shader
```

**Step 2 — Build the application:**
```bash
make debug      # with error checking and validation (recommended while learning)
make release    # optimised build without validation
```

**Step 3 — Run it:**
```bash
make test
# or directly:
./draw_tha_Triangle
```

**Test the image loader standalone:**
```bash
make imgtest
```

**Check for memory leaks:**
```bash
make memtest
```

**Remove all build outputs:**
```bash
make clean
```

---

## Notes

- **Validation layers** print helpful error messages when something is used incorrectly. They are enabled automatically in debug builds and disabled in release builds.
- The texture image (`texture/texture.bmp`) is an original photo taken by the author. All rights reserved.

---

## References

- [vulkan-tutorial.com](https://vulkan-tutorial.com) — the guide this project follows
- [Vulkan specification](https://docs.vulkan.org/spec/latest/) — the official reference
- [cglm documentation](https://cglm.readthedocs.io/) — math library used for vectors
- [Vulkan guide — swapchain semaphore reuse](https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html)
