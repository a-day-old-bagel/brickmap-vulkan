file(GLOB EXE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
add_executable( ${APP_NAME} WIN32 ${EXE_FILES} )
target_include_directories( ${APP_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" )
target_compile_definitions( ${APP_NAME} PUBLIC TRACY_ENABLE )
#target_compile_options( ${APP_NAME} PUBLIC /fsanitize=address )
target_link_libraries( ${APP_NAME} PUBLIC rebel_engine tracy spdlog json vma )
target_precompile_headers( ${APP_NAME} PUBLIC ../apps.pch.h )

set_target_properties( ${APP_NAME} PROPERTIES 
    VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/apps/${APP_NAME}" 
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/apps/${APP_NAME}"
)
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/apps/${APP_NAME}" )

add_dependencies( ${APP_NAME} shaders )