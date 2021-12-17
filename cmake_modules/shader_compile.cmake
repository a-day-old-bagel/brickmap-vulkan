#
# Shader Compile
#

if(NOT DEFINED SHADER_TARGET)
    message( "Shader Compile: Must define SHADER_TARGET before including..." )
    return()
endif()

set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")

file(GLOB_RECURSE GLSL_INCLUDE_FILES
    "${SHADER_SOURCE_DIR}/*.glsl"  
)

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${SHADER_SOURCE_DIR}/*.frag"  
    "${SHADER_SOURCE_DIR}/*.geom"
    "${SHADER_SOURCE_DIR}/*.vert"
    "${SHADER_SOURCE_DIR}/*.comp"
    "${SHADER_SOURCE_DIR}/*.rgen"
    "${SHADER_SOURCE_DIR}/*.rmiss"
    "${SHADER_SOURCE_DIR}/*.rchit"
)

file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})
foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${SHADER_BINARY_DIR}/${FILE_NAME}.spv")
  #message(STATUS ${GLSL})
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} --target-env vulkan1.2 -V ${GLSL} -o ${SPIRV} -g 
    DEPENDS ${GLSL} ${GLSL_INCLUDE_FILES})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    ${SHADER_TARGET} 
    DEPENDS ${SPIRV_BINARY_FILES}
	  SOURCES ${GLSL_SOURCE_FILES}
)
