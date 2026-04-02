# vulkan-hello-triangle-c

A Vulkan hello triangle implemented in pure C (no C++), following the [vulkan-tutorial.com](https://vulkan-tutorial.com) guide. No STL, no RAII — just C, Vulkan, and GLFW.

![triangle](https://vulkan-tutorial.com/images/triangle.png)

---

## Features

- Vulkan instance, device, and swapchain setup in C
- Graphics pipeline with vertex and fragment shaders
- Render pass, framebuffers, and command buffers
- CPU-GPU synchronization with semaphores and fences
- Validation layers enabled in debug builds

---

## Dependencies

| Dependency | Version used | Purpose |
|---|---|---|
| [Vulkan SDK](https://vulkan.lunarg.com/) |  1.4.341 | Vulkan headers and validation layers |
| [GLFW](https://www.glfw.org/) | 3.4.0 | Window creation and surface support |
| [glslc](https://github.com/google/shaderc) | 2026.1 | GLSL to SPIR-V shader compilation (part of Vulkan SDK) |
| GCC / Clang | 15.2.1 / 22.1.2 | C compiler |

On Debian/Ubuntu:
```bash
sudo apt install libglfw3-dev vulkan-tools vulkan-validationlayers-dev spirv-tools
```

On Arch Linux:
```bash
sudo pacman -S glfw-wayland vulkan-tools vulkan-validation-layers shaderc
```

> Use `glfw-x11` instead of `glfw-wayland` if you are on X11.

---

## Project Structure

```
.
├── Makefile
├── shaders/
│   ├── shader.vert       # GLSL vertex shader
│   ├── shader.frag       # GLSL fragment shader
│   ├── vert.spv          # compiled SPIR-V (generated)
│   └── frag.spv          # compiled SPIR-V (generated)
└── src/
    └── main.c            # all Vulkan code
```

---

## Building

**Compile shaders first** (required before building):
```bash
make shader
```

**Debug build** (validation layers enabled):
```bash
make debug
```

**Release build** (validation layers disabled):
```bash
make release
```

---

## Running

```bash
make test
# or directly:
./draw_tha_Triangle
```

**Memory check:**
```bash
#uses valgrind
make memtest
```

**Clean build artifacts:**
```bash
make clean
```

---

## Notes

- On **Wayland**, the window may not appear until the first frame is presented — this is expected compositor behaviour and not a bug.
- Validation layers are enabled by default in debug builds. Run a release build to disable them.
- The project uses C99 VLAs for Vulkan property enumeration — ensure your compiler supports them.

---

## References

- [vulkan-tutorial.com](https://vulkan-tutorial.com)
- [Vulkan specification](https://docs.vulkan.org/spec/latest/)
- [Vulkan guide — swapchain semaphore reuse](https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html)
