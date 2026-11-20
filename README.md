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

| Dependency                                 | Version used | What it does                                           |
| ------------------------------------------ | ------------ | ------------------------------------------------------ |
| [Vulkan SDK](https://vulkan.lunarg.com/)   | 1.3.x+       | The core graphics API and validation tools             |
| [GLFW](https://www.glfw.org/)              | 3.3.x        | Creates the window and connects it to Vulkan           |
| [glslc](https://github.com/google/shaderc) | 2024.x       | Compiles shader code into a format the GPU understands |
| [cglm](https://github.com/recp/cglm)       | 0.9.x        | Math library for vectors and matrices                  |
| GCC / Clang                                | 13.x / 16.x  | C compiler                                             |

You also need a Vulkan driver for your GPU (Mesa's `radeon`, `intel` or `nouveau` drivers, or the proprietary NVIDIA driver). On macOS the driver is [MoltenVK](https://github.com/KhronosGroup/MoltenVK), which translates Vulkan to Metal (see the macOS section below).

### Installing on Arch Linux

```
sudo pacman -S glfw-wayland vulkan-tools vulkan-validation-layers shaderc cglm
```

> Use `glfw-x11` instead of `glfw-wayland` if you are on X11.

### Installing on Void Linux

```
sudo xbps-install -S base-devel pkg-config glfw-devel cglm-devel shaderc \
    vulkan-loader vulkan-loader-devel Vulkan-Headers Vulkan-Tools \
    Vulkan-ValidationLayers
```

Then install the Vulkan driver for your GPU (pick one):

```
sudo xbps-install -S mesa-vulkan-radeon    # AMD
sudo xbps-install -S mesa-vulkan-intel     # Intel
sudo xbps-install -S mesa-vulkan-nouveau   # NVIDIA (open source driver)
```

> Package names on Void are case-sensitive: the Khronos packages are capitalised (`Vulkan-Headers`, `Vulkan-Tools`, `Vulkan-ValidationLayers`), the loader is not (`vulkan-loader`). If a package is not found, check with `xbps-query -Rs <name>`.

### Installing on macOS (Apple Silicon, Homebrew)

The LunarG Vulkan SDK is **not required**. Everything below comes from Homebrew.

**1. Install the packages**

```
brew install glfw cglm shaderc glslang molten-vk vulkan-headers \
    vulkan-loader vulkan-tools vulkan-validationlayers
```

`shaderc` provides `glslc` and `glslang` provides `glslangValidator`. Either one can compile the shaders.

`molten-vk` is the Vulkan driver (it runs Vulkan on top of Metal), and `vulkan-validationlayers` provides the validation layer. The validation layer is a separate package from the loader, so installing the loader alone is not enough.

**2. Set the environment variables**

Add these lines to `~/.zshrc`, then open a new terminal (or run `source ~/.zshrc`):

```
export VK_LAYER_PATH=/opt/homebrew/share/vulkan/explicit_layer.d
export DYLD_LIBRARY_PATH=/opt/homebrew/lib:$DYLD_LIBRARY_PATH
```

- `VK_LAYER_PATH` tells the Vulkan loader where to find the layer manifest (`VkLayer_khronos_validation.json`).
- `DYLD_LIBRARY_PATH` lets the loader open the layer library itself. The manifest refers to `libVkLayer_khronos_validation.dylib` by bare name, so without this the loader finds the layer but fails to load it.
- Run the program from your own terminal. macOS removes `DYLD_*` variables for programs started through system binaries such as `/bin/sh` or `/usr/bin/env`, so some IDE run buttons and wrapper scripts will not pass it on.

If Homebrew installed the library somewhere else, find it and use that directory instead:

```
find /opt/homebrew -name "libVkLayer_khronos_validation*" 2>/dev/null
```

On an Intel Mac, replace `/opt/homebrew` with `/usr/local`.

**3. Check that the loader sees everything**

```
vulkaninfo --summary                 # should list the Apple GPU through MoltenVK
vulkaninfo --summary | grep -i validation
VK_LOADER_DEBUG=error,warn,layer ./draw_tha_Triangle 2>&1 | grep -i -E "validation|dlopen|fail"
```

**4. Portability requirements in the code**

MoltenVK is a "portability" driver, so Vulkan hides it unless the program opts in. On macOS the code must do all of the following (the project guards them with `#ifdef __APPLE__`):

- Enable the instance extensions `VK_KHR_portability_enumeration` and `VK_KHR_get_physical_device_properties2`, in addition to the extensions GLFW asks for.
- Set `VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR` in `VkInstanceCreateInfo.flags`.
- Enable the device extension `VK_KHR_portability_subset` alongside `VK_KHR_swapchain` when creating the logical device.

Without the first two, instance creation fails with `-9`. Without the device extension, device creation fails.

### Checking your installed versions

```
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
├── LICENSE
├── Makefile
├── shaders/
│   ├── shader.vert       # vertex shader source (GLSL)
│   ├── shader.frag       # fragment shader source (GLSL)
│   ├── vert.spv          # compiled vertex shader (generated)
│   └── frag.spv          # compiled fragment shader (generated)
├── src/
│   ├── main.c            # all Vulkan application code
│   ├── img_lib.c         # custom BMP image loader
│   ├── img_lib.h         # image loader header
│   ├── obj_lib.c         # custom OBJ model loader
│   └── obj_lib.h         # model loader header
└── texture/
    └── texture.bmp       # texture image (original photo, all rights reserved by author)
```

Shaders are small programs that run on the GPU. The vertex shader positions each point of the shape, and the fragment shader decides the colour of each pixel. They are written in GLSL and compiled to SPIR-V (`.spv`) before use.

`img_lib` is a lightweight BMP loader written from scratch for this project. It supports 8, 24, and 32-bit BMP files and outputs raw RGBA pixel data ready for direct upload to a Vulkan staging buffer. No third-party image libraries are used.

---

## Building and Running

**Step 1 — Compile the shaders** (only needed once, or when shaders change):

```
make shader
```

**Step 2 — Build the application:**

```
make debug      # with error checking and validation (recommended while learning)
make release    # optimised build without validation
```

**Step 3 — Run it:**

```
make test
# or directly:
./draw_tha_Triangle
```

### Building manually on macOS

If you are not using the Makefile, compile the shaders and the program by hand from the project root.

**Compile the shaders** with either tool:

```
glslangValidator -V shaders/shader.vert -o shaders/vert.spv
glslangValidator -V shaders/shader.frag -o shaders/frag.spv
```

```
glslc shaders/shader.vert -o shaders/vert.spv
glslc shaders/shader.frag -o shaders/frag.spv
```

**Compile the program** (debug build, validation layers enabled):

```
clang -g \
    -I"$(brew --prefix vulkan-headers)/include" \
    -I"$(brew --prefix glfw)/include" \
    -I"$(brew --prefix cglm)/include" \
    src/main.c \
    src/img_lib.c \
    src/obj_lib.c \
    -L"$(brew --prefix vulkan-loader)/lib" \
    -L"$(brew --prefix glfw)/lib" \
    -lglfw \
    -lvulkan \
    -lm \
    -o draw_tha_Triangle
```

For a release build without validation, add `-O2 -DNDEBUG`. Validation is on whenever `NDEBUG` is not defined.

Run it from the project root so the relative paths to the shaders, texture and model resolve, and from a terminal that has the environment variables from the macOS section set.

**Test the image loader standalone:**

```
make imgtest
```

**Check for memory leaks:**

```
make memtest
```

**Remove all build outputs:**

```
make clean
```

---

## Notes

- **Validation layers** print helpful error messages when something is used incorrectly. They are enabled automatically in debug builds and disabled in release builds.
- The texture image (`texture/texture.bmp`) is an original photo taken by the author. All rights reserved.

---

## License

The source code in this repository is released under the [MIT License](LICENSE).

The texture image (`texture/texture.bmp`) is **not** covered by the MIT License. It is an original photo by the author, all rights reserved. Replace it with your own image if you reuse this project.

---

## References

- [vulkan-tutorial.com](https://vulkan-tutorial.com) — the guide this project follows
- [Vulkan specification](https://docs.vulkan.org/spec/latest/) — the official reference
- [cglm documentation](https://cglm.readthedocs.io/) — math library used for vectors
- [Vulkan guide — swapchain semaphore reuse](https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html)
