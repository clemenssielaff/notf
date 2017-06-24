cmake_minimum_required(VERSION 3.5)
project(notf)

set(CMAKE_CXX_STANDARD 14)

INCLUDE_DIRECTORIES(
        include
        thirdparty
        res
        /home/clemens/code/thirdparty/freetype-2.8/include/
        test
        #/home/clemens/code/thirdparty/Python-3.6.1/INSTALL/include/python3.6dm/
)

set(SOURCE_FILES
        src/common/aabr.cpp
        src/common/circle.cpp
        src/common/color.cpp
        src/common/exception.cpp
        src/common/input.cpp
        src/common/line2.cpp
        src/common/log.cpp
        src/common/padding.cpp
        src/common/random.cpp
        src/common/signal.cpp
        src/common/size2.cpp
        src/common/string.cpp
        src/common/system.cpp
        src/common/time.cpp
        src/common/utf.cpp
        src/common/vector2.cpp
        src/common/vector4.cpp
        src/common/xform2.cpp
        src/common/xform4.cpp
        src/core/application.cpp
        src/core/claim.cpp
        src/core/controller.cpp
        src/core/item.cpp
        src/core/item_container.cpp
        src/core/layout.cpp
        src/core/properties.cpp
        src/core/property.cpp
        src/core/render_manager.cpp
        src/core/resource_manager.cpp
        src/core/screen_item.cpp
        src/core/widget.cpp
        src/core/window.cpp
        src/core/window_layout.cpp
        src/dynamic/controller/scroll_area.cpp
        src/dynamic/layout/flex_layout.cpp
        src/dynamic/layout/free_layout.cpp
        src/dynamic/layout/overlayout.cpp
#        src/ext/python/interpreter.cpp
#        src/ext/python/py_aabr.cpp
#        src/ext/python/py_circle.cpp
#        src/ext/python/py_claim.cpp
#        src/ext/python/py_color.cpp
#        src/ext/python/py_controller.cpp
#        src/ext/python/py_dict_utils.cpp
#        src/ext/python/py_event.cpp
#        src/ext/python/py_flexlayout.cpp
#        src/ext/python/py_font.cpp
#        src/ext/python/py_fwd.cpp
#        src/ext/python/py_globals.cpp
#        src/ext/python/py_layoutroot.cpp
#        src/ext/python/py_notf.cpp
#        src/ext/python/py_overlayout.cpp
#        src/ext/python/py_padding.cpp
#        src/ext/python/py_painter.cpp
#        src/ext/python/py_property.cpp
#        src/ext/python/py_resourcemanager.cpp
#        src/ext/python/py_signal.cpp
#        src/ext/python/py_size.cpp
#        src/ext/python/py_texture2.cpp
#        src/ext/python/py_vector2.cpp
#        src/ext/python/py_widget.cpp
#        src/ext/python/py_window.cpp
#        src/ext/python/type_patches.cpp
        src/graphics/cell/cell.cpp
        src/graphics/cell/cell_canvas.cpp
        src/graphics/cell/paint.cpp
        src/graphics/cell/painter.cpp
        src/graphics/cell/painterpreter.cpp
        src/graphics/text/font.cpp
        src/graphics/text/font_atlas.cpp
        src/graphics/text/font_manager.cpp
        src/graphics/blend_mode.cpp
        src/graphics/gl_errors.cpp
        src/graphics/graphics_context.cpp
        src/graphics/raw_image.cpp
        src/graphics/shader.cpp
        src/graphics/stats.cpp
        src/graphics/stencil_func.cpp
        src/graphics/texture2.cpp
        src/graphics/vertex.cpp
        src/utils/filewatcher.cpp
        src/main.cpp
        src/nanovg_test.cpp
        src/scratch.cpp
        thirdparty/double-metaphone/double_metaphone.cc
        thirdparty/tinyutf8/tinyutf8.cpp
)

set(HEADER_FILES
    include/common/aabr.hpp
    include/common/arithmetic.hpp
    include/common/circle.hpp
    include/common/color.hpp
    include/common/enum.hpp
    include/common/exception.hpp
    include/common/float.hpp
    include/common/hash.hpp
    include/common/id.hpp
    include/common/input.hpp
    include/common/integer.hpp
    include/common/line2.hpp
    include/common/log.hpp
    include/common/meta.hpp
    include/common/padding.hpp
    include/common/random.hpp
    include/common/set.hpp
    include/common/signal.hpp
    include/common/simd/simd_arithmetic4f.hpp
    include/common/simd/simd_vector4f.hpp
    include/common/simd/simd_xform4.hpp
    include/common/size2.hpp
    include/common/string.hpp
    include/common/system.hpp
    include/common/time.hpp
    include/common/utf.hpp
    include/common/vector2.hpp
    include/common/vector4.hpp
    include/common/vector.hpp
    include/common/warnings.hpp
    include/common/xform2.hpp
    include/common/xform4.hpp
    include/core/application.hpp
    include/core/claim.hpp
    include/core/controller.hpp
    include/core/events/char_event.hpp
    include/core/events/focus_event.hpp
    include/core/events/key_event.hpp
    include/core/events/mouse_event.hpp
    include/core/fwds.hpp
    include/core/glfw.hpp
    include/core/item_container.hpp
    include/core/item.hpp
    include/core/layout.hpp
    include/core/opengl.hpp
    include/core/properties.hpp
    include/core/property.hpp
    include/core/render_manager.hpp
    include/core/resource_manager.hpp
    include/core/screen_item.hpp
    include/core/widget.hpp
    include/core/window.hpp
    include/core/window_layout.hpp
    include/dynamic/controller/scroll_area.hpp
    include/dynamic/layout/flex_layout.hpp
    include/dynamic/layout/free_layout.hpp
    include/dynamic/layout/overlayout.hpp
    include/ext/python/docstr.hpp
    include/ext/python/interpreter.hpp
    include/ext/python/py_dict_utils.hpp
    include/ext/python/py_fwd.hpp
    include/ext/python/py_notf.hpp
    include/ext/python/py_signal.hpp
    include/ext/python/type_patches.hpp
    include/graphics/blend_mode.hpp
    include/graphics/cell/cell_canvas.hpp
    include/graphics/cell/cell.hpp
    include/graphics/cell/command_buffer.hpp
    include/graphics/cell/commands.hpp
    include/graphics/cell/painter.hpp
    include/graphics/cell/painterpreter.hpp
    include/graphics/cell/paint.hpp
    include/graphics/gl_errors.hpp
    include/graphics/gl_forwards.hpp
    include/graphics/gl_utils.hpp
    include/graphics/graphics_context.hpp
    include/graphics/raw_image.hpp
    include/graphics/scissor.hpp
    include/graphics/shader.hpp
    include/graphics/stats.hpp
    include/graphics/stencil_func.hpp
    include/graphics/text/font_atlas.hpp
    include/graphics/text/font.hpp
    include/graphics/text/font_manager.hpp
    include/graphics/text/freetype.hpp
    include/graphics/texture2.hpp
    include/graphics/vertex.hpp
    include/utils/apply_tuple.hpp
    include/utils/binding_accessors.hpp
    include/utils/debug.hpp
    include/utils/literals.hpp
    include/utils/macro_overload.hpp
    include/utils/make_smart_enabler.hpp
    include/utils/print_notf.hpp
    include/utils/range.hpp
    include/utils/reverse_iterator.hpp
    test/catch.hpp
    test/test_utils.hpp
    thirdparty/randutils/randutils.hpp
)

add_executable(notf ${SOURCE_FILES} ${HEADER_FILES})

# TEST SUITE ###########################################################################################################

include_directories(/home/clemens/code/thirdparty/glm-0.9.8.4/include/)

set(TEST_FILES
        ${SOURCE_FILES}
        test/test_aabr.cpp
        test/test_circle.cpp
        test/test_color.cpp
        test/test_float.cpp
        test/test_main.cpp
        test/test_property.cpp
        test/test_signal.cpp
        test/test_string_utils.cpp
        test/test_utils.hpp
        test/test_vector2.cpp
        test/test_xform2.cpp
        test/test_xform4.cpp
)
LIST(REMOVE_ITEM TEST_FILES src/main.cpp src/scratch.cpp)

add_executable(notf-test ${TEST_FILES})

# LIBRARIES ############################################################################################################

# glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(/home/clemens/code/thirdparty/glfw-3.2.1/ thirdparty/glfw)

find_package(OpenGL REQUIRED)

# freetype
add_subdirectory(/home/clemens/code/thirdparty/freetype-2.8/ thirdparty/freetype)

target_link_libraries(${PROJECT_NAME}
        ${OPENGL_gl_LIBRARY}
        glfw
        freetype
)

target_link_libraries(notf-test
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
