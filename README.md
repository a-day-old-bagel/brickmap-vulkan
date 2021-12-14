# brickmap-vulkan
A high performance realtime Vulkan voxel path tracer.

This system is a Vulkan/SPIRV implementation of the [BrickMap](https://github.com/stijnherfst/BrickMap) by [stijnherfst](https://github.com/stijnherfst) which is in turn based on [this paper](https://dspace.library.uu.nl/handle/1874/315917) by Wingerden. After taking apart several voxel systems as learning exercises, I found that BrickMap
was a great starting point for building my own systems and wanted to provide a version of that code implemented in Vulkan and GLSL.

The Vulkan framework used here is pulled out of my personal tinkering engine project. The application framework is just a simple one for quickly building experiments or demos. This project uses vulkan.hpp.

## Notes

* Press ESC to toggle between mouselook and UI interaction.
* Uncomment *set(FETCHCONTENT_FULLY_DISCONNECTED TRUE)* in cmake_modules/dependencies.cmake to suppress dependency checks.
* You can build rebel_engine with *DEBUG_MARKER_ENABLE* to assign and see buffer names in [NSight](https://developer.nvidia.com/nsight-visual-studio-edition).

## Dependencies

You'll need to install [Vulkan 1.2](https://vulkan.lunarg.com/) on your system.

These dependencies will be downloaded during the first run of cmake:

* [glfw](https://github.com/glfw/glfw.git)
* [glm](https://github.com/g-truc/glm.git)
* [imgui](https://github.com/ocornut/imgui.git)
* [json](https://github.com/nlohmann/json.git)
* [spdlog](https://github.com/gabime/spdlog.git)
* [tracy](https://github.com/wolfpld/tracy.git)
* [simplex-noise](https://github.com/SRombauts/SimplexNoise.git)
* [spirv-reflect](https://github.com/KhronosGroup/SPIRV-Reflect.git)
* [vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap.git)
* [vma](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git)

## Attributions

todo