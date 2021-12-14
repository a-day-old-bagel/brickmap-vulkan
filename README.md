# brickmap-vulkan
A high performance realtime Vulkan voxel path tracer.

This system is a Vulkan/SPIRV implementation of the [BrickMap](https://github.com/stijnherfst/BrickMap) by [stijnherfst](https://github.com/stijnherfst) based on 
[this paper](https://dspace.library.uu.nl/handle/1874/315917) by Wingerden. After taking apart several voxel systems as learning exercises, I found that BrickMap
was a great starting point for building my own systems and wanted to provide a version of that code implemented in Vulkan and GLSL.

## Usage

Press ESC to toggle between mouselook and UI interaction.

## CMake

* Uncomment set(FETCHCONTENT_FULLY_DISCONNECTED TRUE) in cmake_modules/dependencies.cmake to suppress dependency checks.

## Vulkan

todo

## Dependencies

You'll need to install Vulkan 1.2 on your system.

These projects will be downloaded during the first run of cmake:

* glfw
* glm
* spdlog
* json
* tracy
* imgui
* spirv-reflect
* vk-bootstrap
* [vma](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git)
* [simplex-noise](https://github.com/SRombauts/SimplexNoise.git)

## Attributions

todo