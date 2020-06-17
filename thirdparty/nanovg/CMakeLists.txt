cmake_minimum_required(VERSION 3.16)
project(nanovg)

# use plain c++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# use ccache if available
find_program(CCACHE_PROGRAM ccache)
if(NOTF_CCACHE_PROGRAM)
    message(STATUS "notf: Using ccache")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${NOTF_CCACHE_PROGRAM})
endif()

# print additional information during compilation (like timings)
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")

set(CURRENT_TARGET ${PROJECT_NAME})
add_library(${CURRENT_TARGET} STATIC
    src/fontstash.h
    src/glad.c
    src/glad.h
    src/nanovg.c
    src/nanovg_gl.h
    src/nanovg_gl_utils.h
    src/nanovg.h
    src/stb_image.h
    src/stb_truetype.h
)

set_property(TARGET ${CURRENT_TARGET} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(${CURRENT_TARGET} PUBLIC
    src
)

# In order to re-enable Freetype integration, download the latest release, put it into the nanovg thirdparty folder
# and uncomment the lines below.
# Don't forgot to enable position independent code generation to allow static linking to freetype from a shared library.
# Add the following line to the freetype CMakeLists.txt file:
#   set_property(TARGET freetype PROPERTY POSITION_INDEPENDENT_CODE ON)
#
#add_subdirectory(nanovg-thirdparty/freetype-2.10.2)
#target_link_libraries(${CURRENT_TARGET} PUBLIC
#    freetype
#)
#target_compile_definitions(${CURRENT_TARGET} PUBLIC
#    FONS_USE_FREETYPE
#)
