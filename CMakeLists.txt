cmake_minimum_required( VERSION 3.21 )
project( rebel_road VERSION 0.1.0 LANGUAGES C CXX )

set( CMAKE_C_STANDARD 17 )
set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_modules )

#set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/out/${PROJECT_NAME}" )

set( CPACK_PROJECT_NAME ${PROJECT_NAME} )
set( CPACK_PROJECT_VERSION ${PROJECT_VERSION} )
include( CPack )

include ( dependencies )

add_subdirectory(code)


