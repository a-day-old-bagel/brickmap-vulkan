find_package( Vulkan REQUIRED )

file(GLOB_RECURSE ENGINE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
list(FILTER ENGINE_FILES EXCLUDE REGEX "${CMAKE_CURRENT_SOURCE_DIR}/tests/.*" )

add_library( rebel_engine STATIC ${ENGINE_FILES} )

target_include_directories( rebel_engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" )

target_precompile_headers( rebel_engine PRIVATE engine.pch.h )

target_compile_definitions( rebel_engine PUBLIC TRACY_ENABLE GLM_FORCE_DEPTH_ZERO_TO_ONE )
#target_compile_definitions( rebel_engine PUBLIC TRACY_ENABLE GLM_FORCE_DEPTH_ZERO_TO_ONE DEBUG_MARKER_ENABLE )

#target_compile_options( rebel_engine PUBLIC /fsanitize=address )

target_link_libraries( rebel_engine PUBLIC 
    Vulkan::Vulkan
    glm
    vk-bootstrap
    imgui
    vma
    glfw
    spirv_reflect
    tracy
    spdlog
    json
    simplex-noise )

#if ( MODULE_TESTS )
#add_subdirectory( tests )
#endif()