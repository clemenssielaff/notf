cmake_minimum_required(VERSION 3.5)
project(notf)

set(CMAKE_CXX_STANDARD 14)

# utility macro to add source files in a common subdirectory
macro(add_sources source_list base_path)
    foreach(file ${ARGN})
        list(APPEND ${source_list} ${base_path}/${file})
    endforeach()
endmacro()

INCLUDE_DIRECTORIES(
        include
        thirdparty
        res
        /home/clemens/code/thirdparty/freetype-2.8/include/
        test
)

add_sources(SOURCE_FILES src
    common/aabr.cpp
    common/circle.cpp
    common/color.cpp
    common/exception.cpp
    common/input.cpp
    common/line2.cpp
    common/log.cpp
    common/padding.cpp
    common/random.cpp
    common/signal.cpp
    common/size2.cpp
    common/string.cpp
    common/system.cpp
    common/time.cpp
    common/utf.cpp
    common/vector2.cpp
    common/vector4.cpp
    common/xform2.cpp
    common/xform3.cpp
    core/application.cpp
    core/claim.cpp
    core/controller.cpp
    core/item.cpp
    core/item_container.cpp
    core/layout.cpp
    core/properties.cpp
    core/property.cpp
    core/render_manager.cpp
    core/resource_manager.cpp
    core/screen_item.cpp
    core/widget.cpp
    core/window.cpp
    core/window_layout.cpp
    dynamic/controller/scroll_area.cpp
    dynamic/layout/flex_layout.cpp
    dynamic/layout/free_layout.cpp
    dynamic/layout/overlayout.cpp
    dynamic/layout/text_layout.cpp
    dynamic/widget/text_widget.cpp
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
    graphics/cell/cell.cpp
    graphics/cell/cell_canvas.cpp
    graphics/cell/paint.cpp
    graphics/cell/painter.cpp
    graphics/cell/painterpreter.cpp
    graphics/text/font.cpp
    graphics/text/font_atlas.cpp
    graphics/text/font_manager.cpp
    graphics/text/font_utils.cpp
    graphics/blend_mode.cpp
    graphics/gl_errors.cpp
    graphics/graphics_context.cpp
    graphics/raw_image.cpp
    graphics/shader.cpp
    graphics/stats.cpp
    graphics/stencil_func.cpp
    graphics/texture2.cpp
    graphics/vertex.cpp
    utils/filewatcher.cpp
    main.cpp
    nanovg_test.cpp
    scratch.cpp
)

add_sources(HEADER_FILES include
    common/aabr.hpp
    common/arithmetic.hpp
    common/circle.hpp
    common/color.hpp
    common/enum.hpp
    common/exception.hpp
    common/float.hpp
    common/hash.hpp
    common/id.hpp
    common/input.hpp
    common/integer.hpp
    common/line2.hpp
    common/log.hpp
    common/meta.hpp
    common/padding.hpp
    common/random.hpp
    common/set.hpp
    common/signal.hpp
    common/simd/simd_arithmetic4f.hpp
    common/simd/simd_vector4f.hpp
    common/simd/simd_xform3.hpp
    common/size2.hpp
    common/string.hpp
    common/system.hpp
    common/time.hpp
    common/utf.hpp
    common/vector2.hpp
    common/vector4.hpp
    common/vector.hpp
    common/warnings.hpp
    common/xform2.hpp
    common/xform3.hpp
    core/application.hpp
    core/capability.hpp
    core/claim.hpp
    core/controller.hpp
    core/events/char_event.hpp
    core/events/focus_event.hpp
    core/events/key_event.hpp
    core/events/mouse_event.hpp
    core/fwds.hpp
    core/glfw.hpp
    core/item_container.hpp
    core/item.hpp
    core/layout.hpp
    core/opengl.hpp
    core/properties.hpp
    core/property.hpp
    core/render_manager.hpp
    core/resource_manager.hpp
    core/screen_item.hpp
    core/widget.hpp
    core/window.hpp
    core/window_layout.hpp
    dynamic/controller/scroll_area.hpp
    dynamic/layout/flex_layout.hpp
    dynamic/layout/free_layout.hpp
    dynamic/layout/overlayout.hpp
    dynamic/layout/text_layout.hpp
    dynamic/widget/text_widget.hpp
    dynamic/widget/capability/text_capability.hpp
    ext/python/docstr.hpp
    ext/python/interpreter.hpp
    ext/python/py_dict_utils.hpp
    ext/python/py_fwd.hpp
    ext/python/py_notf.hpp
    ext/python/py_signal.hpp
    ext/python/type_patches.hpp
    graphics/blend_mode.hpp
    graphics/cell/cell_canvas.hpp
    graphics/cell/cell.hpp
    graphics/cell/command_buffer.hpp
    graphics/cell/commands.hpp
    graphics/cell/painter.hpp
    graphics/cell/painterpreter.hpp
    graphics/cell/paint.hpp
    graphics/gl_errors.hpp
    graphics/gl_forwards.hpp
    graphics/gl_utils.hpp
    graphics/graphics_context.hpp
    graphics/raw_image.hpp
    graphics/scissor.hpp
    graphics/shader.hpp
    graphics/stats.hpp
    graphics/stencil_func.hpp
    graphics/text/font_atlas.hpp
    graphics/text/font.hpp
    graphics/text/font_manager.hpp
    graphics/text/freetype.hpp
    graphics/text/font_utils.hpp
    graphics/texture2.hpp
    graphics/vertex.hpp
    utils/apply_tuple.hpp
    utils/binding_accessors.hpp
    utils/debug.hpp
    utils/literals.hpp
    utils/macro_overload.hpp
    utils/make_const.hpp
    utils/make_smart_enabler.hpp
    utils/print_notf.hpp
    utils/range.hpp
    utils/reverse_iterator.hpp
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
set(NOTF_FILES ${SOURCE_FILES} ${HEADER_FILES} ${THIRDPARTY_FILES} ${RESOURCE_FILES})
add_executable(notf ${NOTF_FILES})

# TESTS ################################################################################################################

include_directories(/home/clemens/code/thirdparty/glm-0.9.8.4/include/)

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

set(TEST_FILES ${TEST_SOURCES} ${NOTF_FILES})
LIST(REMOVE_ITEM TEST_FILES src/main.cpp src/scratch.cpp src/nanovg_test.cpp)

add_executable(notf-test ${TEST_FILES})

# LIBRARIES ###############################pp#############################################################################

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

# TARGETS ##############################################################################################################

target_link_libraries(${PROJECT_NAME}
        ${OPENGL_gl_LIBRARY}
        glfw
        freetype
        ${PYTHON_LIBRARIES}
)

target_link_libraries(notf-test
        ${OPENGL_gl_LIBRARY}
        glfw
        freetype
        ${PYTHON_LIBRARIES}
        )

set(CMAKE_CXX_FLAGS_DEBUG "-g -D_DEBUG")

# CCACHE ###############################################################################################################

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)
