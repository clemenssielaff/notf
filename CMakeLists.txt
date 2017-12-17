cmake_minimum_required(VERSION 3.10)
project(notf)

# MACROS ###############################################################################################################

# utility macro to add source files in a common subdirectory
macro(add_sources source_list base_path)
    foreach(file ${ARGN})
        list(APPEND ${source_list} ${base_path}/${file})
    endforeach()
endmacro()

# utility macro to add source files in a common subdirectory
macro(create_module name)
    file(GLOB_RECURSE ${name}_SOURCE RELATIVE ${CMAKE_SOURCE_DIR}
        "src/${name}/*.cpp"
    )
    file(GLOB_RECURSE ${name}_HEADERS RELATIVE ${CMAKE_SOURCE_DIR}
        "include/${name}/*.hpp"
    )
    file(GLOB_RECURSE ${name}_MARKDOWN RELATIVE ${CMAKE_SOURCE_DIR}
        "include/${name}/*.md"
    )
    set(${name}_MODULE ${${name}_SOURCE} ${${name}_HEADERS} ${${name}_MARKDOWN})
endmacro()

# OPTIONS ##############################################################################################################

option(USE_LIBCXX "Use LLVM's libc++ instead of libstdc++" OFF)
option(USE_OPENMP "Use OpenMP" ON)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)

# LIBRARIES ############################################################################################################

# libcxx
if(USE_LIBCXX)
    set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-fuse-ld=lld")
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-stdlib=libc++")
endif()

# glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(/home/clemens/code/thirdparty/glfw-3.2.1/ thirdparty/glfw)

# opengl
find_package(OpenGL REQUIRED)

# freetype
add_subdirectory(/home/clemens/code/thirdparty/freetype-2.8.1/ thirdparty/freetype)

# benchmark
if(BUILD_BENCHMARKS)
    add_subdirectory(/home/clemens/code/thirdparty/benchmark/ thirdparty/benchmark)
endif()

# OpenMP
if(USE_OPENMP)
    find_package(OpenMP)
    if(OPENMP_FOUND)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    endif()
endif()

# NOTF #################################################################################################################

set(CMAKE_CXX_FLAGS_DEBUG "-g -DNOTF_DEBUG")
set(CMAKE_CXX_STANDARD 14)

include_directories(
    include/
    thirdparty/
    res/
    /home/clemens/code/thirdparty/freetype-2.8/include/
    test/
    examples/
)

create_module(common)
create_module(app)
create_module(graphics)
create_module(utils)

add_sources(SOURCE_FILES src
    main.cpp
)

add_sources(SOURCE_FILES examples
    framebuffer_example.cpp
    line_example.cpp
    wireframe_example.cpp
    shape_example.cpp
)

add_sources(THIRDPARTY_FILES thirdparty
    double-metaphone/double_metaphone.cc
    tinyutf8/tinyutf8.cpp
    randutils/randutils.hpp
)

add_sources(RESOURCE_FILES res
    shaders/cell.frag
    shaders/cell.vert
    shaders/blinn_phong.vert
    shaders/blinn_phong.frag
    shaders/wireframe.vert
    shaders/wireframe.tess
    shaders/wireframe.eval
    shaders/wireframe.geo
    shaders/wireframe.frag
    shaders/line.vert
    shaders/line.tess
    shaders/line.eval
    shaders/line.frag
    shaders/trivial.vert
    shaders/trivial.frag
)

add_executable(notf
    ${common_MODULE}
    ${app_MODULE}
    ${graphics_MODULE}
    ${utils_MODULE}
    ${SOURCE_FILES}
    ${THIRDPARTY_FILES}
    ${RESOURCE_FILES}
)

target_link_libraries(${PROJECT_NAME}
    ${OPENGL_gl_LIBRARY}
    glfw
    freetype
    ${PYTHON_LIBRARIES}
)

# TESTS ################################################################################################################

include_directories(
    /home/clemens/code/thirdparty/glm-0.9.8.5/include/
)

add_sources(TEST_SOURCES test
    test_aabr.cpp
    test_circle.cpp
    test_color.cpp
    test_float.cpp
    test_main.cpp
    test_half.cpp
#    test_signal.cpp
    test_string_utils.cpp
#    test_vector2.cpp
#    test_matrix3.cpp
#    test_matrix4.cpp
    test_utils.cpp

    catch.hpp
    test_utils.hpp
    glm_utils.hpp
)

add_executable(notf-test
    ${common_MODULE}
    ${core_MODULE}
    ${dynamic_MODULE}
    ${graphics_MODULE}
    ${utils_MODULE}
    ${TEST_SOURCES}
    ${THIRDPARTY_FILES}
    ${RESOURCE_FILES}
)

target_link_libraries(notf-test
    ${OPENGL_gl_LIBRARY}
    glfw
    freetype
    ${PYTHON_LIBRARIES}
)

# BENCHMARK ############################################################################################################

if(BUILD_BENCHMARKS)

    include_directories(
        /home/clemens/code/thirdparty/benchmark/INSTALL/include/
    )

    add_sources(BENCHMARK_SOURCES benchmark
        bench_vector4.cpp
        benchmark.cpp
    )

    add_executable(notf-benchmark
        ${common_MODULE}
        ${utils_MODULE}
        ${BENCHMARK_SOURCES}
        ${THIRDPARTY_FILES}
    )

    target_link_libraries(notf-benchmark
        glfw
        benchmark
        pthread
    )

endif()

# CCACHE ###############################################################################################################

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)
