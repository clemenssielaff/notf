cmake_minimum_required(VERSION 3.5)
project(notf)

set(CMAKE_CXX_STANDARD 14)

INCLUDE_DIRECTORIES(
        include
        thirdparty
        res
        /home/clemens/code/thirdparty/freetype-2.7/INSTALL/include/freetype2/
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
        src/common/xform3.cpp
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
    include/common/simd/simd_arithmetic4f.hpp
    include/common/simd/simd_vector4f.hpp
)

add_executable(notf ${SOURCE_FILES} ${HEADER_FILES})

# TEST SUITE ###########################################################################################################

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
)
LIST(REMOVE_ITEM TEST_FILES src/main.cpp)

add_executable(notf-test ${TEST_FILES})

# LIBRARIES ############################################################################################################

# glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(/home/clemens/code/thirdparty/glfw-3.2.1/ thirdparty/glfw)

find_package(OpenGL REQUIRED)

# freetype
add_subdirectory(/home/clemens/code/thirdparty/freetype-2.7/ thirdparty/freetype)

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
