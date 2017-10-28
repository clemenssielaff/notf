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

# LIBRARIES ############################################################################################################

# glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(/home/clemens/code/thirdparty/glfw-3.2.1/ thirdparty/glfw)

find_package(OpenGL REQUIRED)

# freetype
add_subdirectory(/home/clemens/code/thirdparty/freetype-2.8/ thirdparty/freetype)

# NOTF #################################################################################################################

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "-g -D_DEBUG")

include_directories(
    include/
    thirdparty/
    res/
    /home/clemens/code/thirdparty/freetype-2.8/include/
    test/
)

create_module(common)
create_module(core)
create_module(dynamic)
create_module(graphics)
create_module(utils)

add_sources(SOURCE_FILES src
    main.cpp
)

add_sources(THIRDPARTY_FILES thirdparty
    double-metaphone/double_metaphone.cc
    tinyutf8/tinyutf8.cpp
    randutils/randutils.hpp
)

add_sources(RESOURCE_FILES res
    shaders/cell.frag
    shaders/cell.vert
)

add_executable(notf
    ${common_MODULE}
    ${core_MODULE}
    ${dynamic_MODULE}
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
    /home/clemens/code/thirdparty/glm-0.9.8.4/include/
)

add_sources(TEST_SOURCES test
    test_aabr.cpp
    test_circle.cpp
    test_color.cpp
    test_float.cpp
    test_main.cpp
    test_signal.cpp
    test_string_utils.cpp
    test_utils.hpp
    test_vector2.cpp
    test_matrix3.cpp
    test_matrix4.cpp
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

# CCACHE ###############################################################################################################

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)
