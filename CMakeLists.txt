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
        "src/${name}/*.hpp"
    )
    file(GLOB_RECURSE ${name}_MARKDOWN RELATIVE ${CMAKE_SOURCE_DIR}
        "src/${name}/*.md"
    )
    set(${name}_MODULE ${${name}_SOURCE} ${${name}_HEADERS} ${${name}_MARKDOWN})
endmacro()

# OPTIONS ##############################################################################################################

option(NOTF_BUILD_APP "Build application" ON)
option(NOTF_USE_LIBCXX "Use LLVM's libc++ instead of libstdc++" OFF)
option(NOTF_USE_OPENMP "Use OpenMP" ON)
option(NOTF_BUILD_TESTS "Build tests" OFF)
option(NOTF_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(NOTF_LOG_PRETTY_FUNCTION "Log long function names" ON)

# LIBRARIES ############################################################################################################

# libcxx
if(NOTF_USE_LIBCXX)
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
if(NOTF_BUILD_BENCHMARKS)
    add_subdirectory(/home/clemens/code/thirdparty/benchmark/ thirdparty/benchmark)
endif()

# OpenMP
if(NOTF_USE_OPENMP)
    find_package(OpenMP)
    if(OPENMP_FOUND)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    endif()
endif()

# NOTF #################################################################################################################

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "-g -DNOTF_DEBUG")

if(NOTF_LOG_PRETTY_FUNCTION)
    add_definitions(-DNOTF_LOG_PRETTY_FUNCTION)
endif()

include_directories(
    ./
    src/
    thirdparty/
    res/
    /home/clemens/code/thirdparty/freetype-2.8.1/include
    examples/
)

create_module(common)
#create_module(app)
create_module(graphics)
create_module(utils)


add_sources(SOURCE_FILES src
    app/application.cpp
    app/application.hpp
    app/ids.hpp
    app/window.cpp
    app/window.hpp
    app/property_graph.hpp
    app/property_graph.cpp
    app/resource_manager.hpp
    app/resource_manager.cpp
    app/event_manager.hpp
    app/event_manager.cpp
    app/render_target.hpp
    app/render_target.cpp
    app/render_manager.hpp
    app/render_manager.cpp
    app/scene.hpp
    app/scene.cpp
    app/layer.hpp
    app/layer.cpp
    app/scene_manager.hpp
    app/scene_manager.cpp
    app/renderer.hpp
    app/renderer.cpp
    app/forwards.hpp
    app/glfw.hpp
    app/io/event.hpp
    app/io/event.cpp
    app/io/char_event.hpp
    app/io/char_event.cpp
    app/io/key_event.hpp
    app/io/key_event.cpp
    app/io/keyboard.hpp
    app/io/keyboard.cpp
    app/io/mouse_event.hpp
    app/io/mouse_event.cpp
    app/io/time.hpp
    app/io/time.cpp
    app/io/window_event.hpp
    app/io/window_event.cpp
    app/README.md
)

add_sources(SOURCE_FILES examples
#    framebuffer_example.cpp
#    line_example.cpp
#    wireframe_example.cpp
#    shape_example.cpp
#    text_example.cpp
#    rendermanager_example.cpp
#    renderthread_example.cpp
    properties_example.cpp
)

add_sources(THIRDPARTY_FILES thirdparty
    tinyutf8/tinyutf8.cpp
    randutils/randutils.hpp
    abseil/abseil.hpp
    abseil/any.hpp
    abseil/any.cpp
    abseil/optional.hpp
    abseil/optional.cpp
    mpark/variant.hpp
    robin-map/robin_map.h
    robin-map/robin_set.h
    fmt/format.h
    fmt/format.cc
)

#add_sources(RESOURCE_FILES res
#    shaders/cell.frag
#    shaders/cell.vert
#    shaders/blinn_phong.vert
#    shaders/blinn_phong.frag
#    shaders/fullscreen.vert
#    shaders/wireframe.vert
#    shaders/wireframe.tess
#    shaders/wireframe.eval
#    shaders/wireframe.geo
#    shaders/wireframe.frag
#    shaders/plotter.vert
#    shaders/plotter.tess
#    shaders/plotter.eval
#    shaders/plotter.frag
#    shaders/trivial.vert
#    shaders/trivial.frag
#    shaders/clouds.frag
#)

if(NOTF_BUILD_APP)

    add_sources(SOURCE_FILES src main.cpp)

    add_executable(notf
        ${common_MODULE}
    #    ${app_MODULE}
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

endif()

# TESTS ################################################################################################################

if(NOTF_BUILD_TESTS)

    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-DNOTF_TEST")

    include_directories(
        /home/clemens/code/thirdparty/glm-0.9.8.5/include/
    )

    add_sources(TEST_SOURCES test
        catch.hpp
        main.cpp
#        glm_utils.hpp
        testenv.hpp
#        test_aabr.cpp
#        test_circle.cpp
#        test_color.cpp
#        test_dag.cpp
#        test_float.cpp
#        test_half.cpp
#        test_matrix3.cpp
#        test_matrix4.cpp
        test_scene.cpp
        test_properties.cpp
#        test_signal.cpp
#        test_string_utils.cpp
#        test_utils.cpp
#        test_utils.hpp
#        test_vector2.cpp
    )

    add_executable(notf-test
        ${common_MODULE}
    #    ${app_MODULE}
        ${graphics_MODULE}
        ${utils_MODULE}
        ${SOURCE_FILES}
        ${THIRDPARTY_FILES}
        ${RESOURCE_FILES}
        ${TEST_SOURCES}
    )

    target_link_libraries(notf-test
        ${OPENGL_gl_LIBRARY}
        glfw
        freetype
        ${PYTHON_LIBRARIES}
    )

endif()

# BENCHMARK ############################################################################################################

if(NOTF_BUILD_BENCHMARKS)

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
