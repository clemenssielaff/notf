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
option(NOTF_BUILD_TESTS "Build tests" OFF)
option(NOTF_LOG_PRETTY_FUNCTION "Log long function names" ON)
option(NOTF_THREAD_SANITIZATION "Enable thread sanitization" OFF)

# CONFIGURATION ########################################################################################################

# hide symbols by default
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# link-time optimization
include(CheckIPOSupported)
check_ipo_supported(RESULT IS_LTO_SUPPORTED OUTPUT LTO_ERROR)

# ccache
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    message(STATUS "Using ccache")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

# policies
if(COMMAND cmake_policy)
    cmake_policy(PUSH)

    if(POLICY CMP0072)
        cmake_policy(SET CMP0072 NEW)
    endif()
endif()

# thread sanitization
if(NOTF_THREAD_SANITIZATION)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options("-fsanitize=thread")
        link_libraries("-fsanitize=thread")
        message(STATUS "Building using thread sanitization")
    else()
        set(NOTF_THREAD_SANITIZATION OFF)
        message(AUTHOR_WARNING "Thread sanitization is only supported for build using Clang")
    endif()
endif()

# LIBRARIES ############################################################################################################

# glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(thirdparty/glfw-3.3.0)

# opengl
find_package(OpenGL REQUIRED)

# freetype
add_subdirectory(thirdparty/freetype-2.9.1)

# NOTF #################################################################################################################

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DNOTF_DEBUG")

if(NOTF_LOG_PRETTY_FUNCTION)
    add_definitions(-DNOTF_LOG_PRETTY_FUNCTION)
endif()

include_directories(notf
    src/
    res/
    examples/
    thirdparty/
    thirdparty/glfw-3.3.0/include
    thirdparty/freetype-2.9.1/include
)

create_module(common)
create_module(app)
create_module(graphics)
create_module(utils)

add_sources(SOURCE_FILES examples
#    framebuffer_example.cpp
#    line_example.cpp
#    wireframe_example.cpp
#    shape_example.cpp
#    text_example.cpp
#    rendermanager_example.cpp
#    renderthread_example.cpp
    smoke_example.cpp
)

add_sources(THIRDPARTY_FILES thirdparty
    tinyutf8/tinyutf8.cpp

    abseil/any.cpp
    abseil/optional.cpp

    glad/glad.c

    fmt/format.cc
    fmt/ostream.cc
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

add_library(notf
    ${common_MODULE}
    ${app_MODULE}
    ${graphics_MODULE}
    ${utils_MODULE}
    ${SOURCE_FILES}
    ${THIRDPARTY_FILES}
    ${RESOURCE_FILES}
)

target_link_libraries(notf
    glfw
    freetype
    ${PYTHON_LIBRARIES}
    ${OPENGL_LIBRARIES}
)

if(CMAKE_BUILD_TYPE MATCHES "Release")

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-fuse-ld=lld")
    endif()

    if(IS_LTO_SUPPORTED)
        message(STATUS "Building notf with LTO enabled")
        set_property(TARGET notf PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "LTO failed:\n${LTO_ERROR}")
    endif()

endif()

# APP ##################################################################################################################

if(NOTF_BUILD_APP)

    add_executable (notf-app src/main.cpp)
    target_link_libraries (notf-app notf)

endif()

# TESTS ################################################################################################################

if(NOTF_BUILD_TESTS)

    message(STATUS "Building Tests")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOTF_TEST")

    if(NOT MSVC)
        if(NOTF_THREAD_SANITIZATION)
            message(AUTHOR_WARNING "Disabling coverage to avoid interference with thread sanitization.")
        else()
            message(STATUS "Building with coverage.")
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
        endif()
    endif()

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
        test_node.hpp
        test_node.cpp
        test_scene.hpp
        test_scene.cpp
        test_scene_graph.hpp
        test_scene_graph.cpp
        test_timer.cpp
        test_path.cpp
        test_claim.cpp
        test_properties.hpp
        test_properties.cpp
        test_event_manager.hpp
#        test_signal.cpp
#        test_string_utils.cpp
#        test_utils.cpp
#        test_utils.hpp
#        test_vector2.cpp
    )

    add_executable(notf-test ${TEST_SOURCES})
    target_link_libraries(notf-test notf)

endif()


########################################################################################################################

if(COMMAND cmake_policy)
    cmake_policy(POP)
endif()
