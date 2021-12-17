#set(FETCHCONTENT_QUIET FALSE)
#set(FETCHCONTENT_FULLY_DISCONNECTED TRUE)
#set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)

include( FetchContent )
include( ExternalProject )

message( "Updating dependencies..." )

#
# glfw
#

message( "  [glfw]" )

FetchContent_Declare(
  glfw
  GIT_REPOSITORY "https://github.com/glfw/glfw.git"
  GIT_TAG "3.3.4"
)

FetchContent_GetProperties( glfw )
if ( NOT glfw_POPULATED )
  FetchContent_Populate( glfw )
  add_subdirectory( ${glfw_SOURCE_DIR} ${glfw_BINARY_DIR} EXCLUDE_FROM_ALL )
endif()

#
# glm
#

message( "  [glm]" )

FetchContent_Declare(
  glm
  GIT_REPOSITORY "https://github.com/g-truc/glm.git"
  GIT_TAG "0.9.9.8"
)

FetchContent_GetProperties( glm )
if ( NOT glm_POPULATED )
  FetchContent_Populate( glm )
  add_library( glm INTERFACE )
  target_include_directories( glm SYSTEM INTERFACE "${glm_SOURCE_DIR}" )
endif()

#
# glslc
# Shader program compiler must be available.
#

message( "  [glslc]" )

find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

#
# imgui
#

message( "  [imgui]" )

FetchContent_Declare(
  imgui
  GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
  GIT_TAG "master"
)

FetchContent_GetProperties( imgui )
if ( NOT imgui_POPULATED )
  FetchContent_Populate( imgui )
  add_library( imgui STATIC )
  target_include_directories( imgui PUBLIC "${imgui_SOURCE_DIR}" )
  target_sources(imgui PRIVATE 
      "${imgui_SOURCE_DIR}/imgui.h"
      "${imgui_SOURCE_DIR}/imgui.cpp"
      "${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.h"
      "${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp"
      "${imgui_SOURCE_DIR}/imgui_demo.cpp"
      "${imgui_SOURCE_DIR}/imgui_draw.cpp"
      "${imgui_SOURCE_DIR}/imgui_tables.cpp"
      "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
      "${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp"
      "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
  )
  target_link_libraries( imgui PUBLIC Vulkan::Vulkan glfw )
endif()

#
# simplex-noise
#

message( "  [simplex-noise]" )

FetchContent_Declare(
  simplex-noise
  GIT_REPOSITORY "https://github.com/SRombauts/SimplexNoise.git"
  GIT_TAG "master"
)

FetchContent_GetProperties( tinyobj )
if ( NOT simplex-noise_POPULATED )
  FetchContent_Populate( simplex-noise )
  add_library(simplex-noise STATIC)
  target_sources(simplex-noise PRIVATE 
      "${simplex-noise_SOURCE_DIR}/src/SimplexNoise.h"
      "${simplex-noise_SOURCE_DIR}/src/SimplexNoise.cpp"
  )
  target_include_directories( simplex-noise SYSTEM PUBLIC "${simplex-noise_SOURCE_DIR}/src" )
endif()

#
# spdlog
#

message( "  [spdlog]" )

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
  GIT_TAG "v1.9.2"
)

FetchContent_GetProperties( spdlog )
if ( NOT spdlog_POPULATED )
  FetchContent_Populate( spdlog )
  add_subdirectory( ${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR} EXCLUDE_FROM_ALL )
endif()

#
# spirv-reflect
#

message( "  [spirv-reflect]" )

FetchContent_Declare(
  spirv-reflect
  GIT_REPOSITORY "https://github.com/KhronosGroup/SPIRV-Reflect.git"
  GIT_TAG "master"
)

FetchContent_GetProperties( spirv-reflect )
if ( NOT spirv-reflect_POPULATED )
  FetchContent_Populate( spirv-reflect )
  add_library(spirv_reflect STATIC)
  target_sources(spirv_reflect PRIVATE 
      "${spirv-reflect_SOURCE_DIR}/spirv_reflect.h"
      "${spirv-reflect_SOURCE_DIR}/spirv_reflect.c"
  )
  target_include_directories(spirv_reflect SYSTEM PUBLIC "${spirv-reflect_SOURCE_DIR}/" )
  target_include_directories(spirv_reflect SYSTEM PUBLIC "${spirv-reflect_SOURCE_DIR}/include" )
endif()

#
# tracy
#

message( "  [tracy]" )

FetchContent_Declare(
  tracy
  GIT_REPOSITORY "https://github.com/wolfpld/tracy.git"
  GIT_TAG "master"
)

FetchContent_GetProperties( tracy )
if ( NOT tracy_POPULATED )
  FetchContent_Populate( tracy )
  set( TRACY_DIR "${tracy_SOURCE_DIR}" CACHE FILEPATH "Path to Tracy" )
  add_library( tracy OBJECT "${tracy_SOURCE_DIR}/TracyClient.cpp" )
  target_include_directories( tracy PUBLIC "${tracy_SOURCE_DIR}" )
  target_compile_definitions( tracy PUBLIC TRACY_ENABLE )
endif()

#
# vk-bootstrap
#

message( "  [vk-bootstrap]" )

FetchContent_Declare(
  vk-bootstrap
  GIT_REPOSITORY "https://github.com/charles-lunarg/vk-bootstrap.git"
  GIT_TAG "master"
)

FetchContent_GetProperties( vk-bootstrap )
if ( NOT vk-bootstrap_POPULATED )
  FetchContent_Populate( vk-bootstrap )
  add_subdirectory( ${vk-bootstrap_SOURCE_DIR} ${vk-bootstrap_BINARY_DIR} EXCLUDE_FROM_ALL )
endif()

#
# vma
#

message( "  [vma]" )

FetchContent_Declare(
  vma
  GIT_REPOSITORY "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git"
  GIT_TAG "master"
)

FetchContent_GetProperties( vma )
if ( NOT vma_POPULATED )
  FetchContent_Populate( vma )
  add_library( vma INTERFACE )
  target_include_directories( vma SYSTEM INTERFACE "${vma_SOURCE_DIR}/include" )
endif()

#
# Vulkan
#

message( "  [vulkan]" )

find_package(Vulkan REQUIRED COMPONENTS glslc)



