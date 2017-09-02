cmake_minimum_required(VERSION 3.5)

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
    set(${name}_MODULE ${${name}_SOURCE} ${${name}_HEADERS})
    message(${${name}_SOURCE})
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

# python
find_package(PythonLibs 3 REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})
add_definitions(-DNOTF_PYTHON)

# NOTF #################################################################################################################

project(notf)
set(CMAKE_CXX_STANDARD 14)
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
#create_module(ext)
create_module(graphics)
create_module(utils)

################## TMP
add_sources(ext_SOURCE src
    ext/python/interpreter.cpp
    ext/python/py_aabr.cpp
    ext/python/py_circle.cpp
    ext/python/py_claim.cpp
    ext/python/py_color.cpp
#   ext/python/py_controller.cpp
    ext/python/py_dict_utils.cpp
    ext/python/py_event.cpp
    ext/python/py_flexlayout.cpp
#   ext/python/py_font.cpp
    ext/python/py_fwd.cpp
    ext/python/py_globals.cpp
    ext/python/py_layoutroot.cpp
    ext/python/py_notf.cpp
    ext/python/py_overlayout.cpp
    ext/python/py_padding.cpp
    ext/python/py_painter.cpp
#   ext/python/py_property.cpp
#   ext/python/py_resourcemanager.cpp
#   ext/python/py_signal.cpp
    ext/python/py_size.cpp
#   ext/python/py_texture2.cpp
    ext/python/py_vector2.cpp
#   ext/python/py_widget.cpp
    ext/python/py_window.cpp
    ext/python/type_patches.cpp
)

add_sources(ext_HEADERS include
    ext/python/docstr.hpp
    ext/python/interpreter.hpp
    ext/python/py_dict_utils.hpp
    ext/python/py_fwd.hpp
    ext/python/py_notf.hpp
    ext/python/py_signal.hpp
    ext/python/type_patches.hpp
)
set(ext_MODULE ${ext_SOURCE} ${ext_HEADERS})

################## TMP

add_sources(SOURCE_FILES src
    main.cpp
    nanovg_test.cpp
    scratch.cpp
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
    ${ext_MODULE}
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
    test_claim.cpp
    test_color.cpp
    test_float.cpp
    test_main.cpp
    test_property.cpp
    test_signal.cpp
    test_string_utils.cpp
    test_utils.hpp
    test_vector2.cpp
    test_xform2.cpp
    test_xform3.cpp
    test_overlayout.cpp
    test_flexlayout.cpp
    test_utils.cpp

    catch.hpp
    test_utils.hpp
    glm_utils.hpp
)

add_executable(notf-test
    ${common_MODULE}
    ${core_MODULE}
    ${dynamic_MODULE}
    ${ext_MODULE}
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

# RENDER_ENGINE ########################################################################################################

create_module(engine)

add_sources(RENDER_SOURCES src
    render_main.cpp
)

add_executable(render_engine
    ${common_MODULE}
    ${utils_MODULE}
    ${graphics_MODULE}
    ${engine_MODULE}
    ${RENDER_SOURCES}
    ${THIRDPARTY_FILES}
)

target_link_libraries(render_engine
        ${OPENGL_gl_LIBRARY}
        glfw
        freetype
)

# CCACHE ###############################################################################################################

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)
