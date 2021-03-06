# brickmap-vulkan
A realtime Vulkan voxel path tracer.

This is a work in progress!

This system is a Vulkan/SPIRV implementation of the [BrickMap](https://github.com/stijnherfst/BrickMap) by [stijnherfst](https://github.com/stijnherfst) which is in turn based on [this paper](https://dspace.library.uu.nl/handle/1874/315917) by Wingerden. After taking apart several voxel systems as learning exercises, I found that BrickMap was a great starting point for building my own systems and wanted to provide a version of that code implemented in Vulkan and GLSL.

The Vulkan framework used here is pulled out of my personal tinkering engine project and is based on various tutorials and learning. https://vkguide.dev/ has been particularly helpful in learning Vulkan. This project uses vulkan.hpp.

## Notes

* Press ESC to toggle between mouselook and UI interaction.
* You can build rebel_engine with *DEBUG_MARKER_ENABLE* to assign and see buffer names in [NSight](https://developer.nvidia.com/nsight-visual-studio-edition).
* Linux doesn't build yet, but coming soon.
* Tracy can be enabled by building with TRACY_ENABLE.

## Dependencies

You'll need to install the [Vulkan SDK](https://vulkan.lunarg.com/) on your system.

These dependencies will be downloaded during the first run of cmake:

* [glfw](https://github.com/glfw/glfw.git)
* [glm](https://github.com/g-truc/glm.git)
* [imgui](https://github.com/ocornut/imgui.git)
* [spdlog](https://github.com/gabime/spdlog.git)
* [tracy](https://github.com/wolfpld/tracy.git)
* [simplex-noise](https://github.com/SRombauts/SimplexNoise.git)
* [spirv-reflect](https://github.com/KhronosGroup/SPIRV-Reflect.git)
* [vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap.git)
* [vma](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git)

Fetching content with cmake can take a while, but I find this approach to managing dependencies is easier overall.
Once you've grabbed everything, you can uncomment *set(FETCHCONTENT_FULLY_DISCONNECTED TRUE)* in cmake_modules/dependencies.cmake to suppress dependency checks.

![normal](https://user-images.githubusercontent.com/89154706/146416013-f705fdbf-cc85-49c3-a438-476377e8df2b.png)

![normals](https://user-images.githubusercontent.com/89154706/146415997-5896a2b7-df37-4ab7-8572-4f3ddfb217d4.png)

![heatmap1](https://user-images.githubusercontent.com/89154706/146415867-6ca08930-6349-46e8-ac45-2f2b6edcf43e.png)

