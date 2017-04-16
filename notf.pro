TARGET = notf
TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle qt

INCLUDEPATH *= thirdparty/ include/ test/ res/

# glfw
LIBS *= -L/home/clemens/code/thirdparty/glfw-3.2.1/INSTALL/lib/
LIBS *= -lglfw3 -lGL -lX11 -lXxf86vm -lpthread -ldl -lXcursor -lXrandr -lXinerama

# freetype
LIBS *= -L/home/clemens/code/thirdparty/freetype-2.7/INSTALL/lib/
LIBS *= -lfreetype
INCLUDEPATH *= /home/clemens/code/thirdparty/freetype-2.7/INSTALL/include/freetype2/

CONFIG(release, debug|release) {
#    message("Building in Release Mode.")
    DEFINES += "NDEBUG" #"NOTF_LOG_LEVEL=3"
    QMAKE_CXXFLAGS_RELEASE += -fvisibility=hidden -s
    QMAKE_CXXFLAGS_RELEASE += -O3 -flto #-Os
    QMAKE_LFLAGS_RELEASE += -fvisibility=hidden -s
    QMAKE_LFLAGS_RELEASE += -O3 -flto #-Os

    # python
    LIBS *= -L/home/clemens/code/thirdparty/Python-3.6.1/
    LIBS *= -lpython3.6m
    INCLUDEPATH *= /home/clemens/code/thirdparty/Python-3.6.1/INSTALL/include/python3.6m/
}

# debug
CONFIG(debug, debug|release) {
#    message("Building in Debug Mode.")
    DEFINES += _DEBUG

    # python
    LIBS *= -L/home/clemens/code/thirdparty/Python-3.6.1/
    LIBS *= -lpython3.6dm
    INCLUDEPATH *= /home/clemens/code/thirdparty/Python-3.6.1/INSTALL/include/python3.6dm/
}

# test
CONFIG(test) {
#    message("Building in Test Mode.")

    DEFINES += _DEBUG _TEST
    QMAKE_CXXFLAGS *= -fprofile-arcs -ftest-coverage
    QMAKE_LFLAGS *= -lgcov -coverage

    SOURCES += \
        test/test_main.cpp \
        test/test_string_utils.cpp \
        test/test_signal.cpp \
        test/test_color.cpp \
        test/test_circle.cpp \
        test/test_aabr.cpp \
        test/test_property.cpp \
        test/test_vector2.cpp \
        test/test_float.cpp

    HEADERS += \
        test/catch.hpp \
        test/test_utils.hpp
}
else {
    SOURCES += \
        src/main.cpp \
        src/scratch.cpp \
}

# python
CONFIG(python) {
    message("Building with Python support enabled.")

    DEFINES += NOTF_PYTHON

    SOURCES += \
        src/python/interpreter.cpp \
        src/python/py_layoutroot.cpp \
        src/python/py_painter.cpp \
        src/python/py_stacklayout.cpp \
        src/python/py_vector2.cpp \
        src/python/py_widget.cpp \
        src/python/py_window.cpp \
        src/python/py_color.cpp \
        src/python/py_aabr.cpp \
        src/python/py_circle.cpp \
        src/python/py_texture2.cpp \
        src/python/py_size.cpp \
        src/python/py_resourcemanager.cpp \
        src/python/py_font.cpp \
        src/python/py_padding.cpp \
        src/python/py_claim.cpp \
        src/python/py_overlayout.cpp \
        src/python/py_notf.cpp \
        src/python/py_controller.cpp \
        src/python/type_patches.cpp \
        src/python/py_fwd.cpp \
        src/python/py_globals.cpp \
        src/python/py_event.cpp \
        src/python/py_signal.cpp \
        src/python/py_dict_utils.cpp \
        src/python/py_property.cpp

    HEADERS += \
        include/python/interpreter.hpp \
        include/python/py_notf.hpp \
        include/python/docstr.hpp \
        include/python/type_patches.hpp \
        include/python/py_fwd.hpp \
        include/python/py_signal.hpp \
        include/python/py_dict_utils.hpp
}

SOURCES += \
    src/core/application.cpp \
    src/core/window.cpp \
    src/core/widget.cpp \
    src/common/log.cpp \
    src/common/vector2.cpp \
    src/common/vector3.cpp \
    src/common/aabr.cpp \
    src/core/resource_manager.cpp \
    src/common/system.cpp \
    src/core/render_manager.cpp \
    src/common/color.cpp \
    src/core/layout.cpp \
    src/dynamic/layout/stack_layout.cpp \
    src/common/claim.cpp \
    src/graphics/raw_image.cpp \
    src/graphics/texture2.cpp \
    src/common/circle.cpp \
    src/common/time.cpp \
    src/common/padding.cpp \
    src/dynamic/layout/overlayout.cpp \
    src/core/item.cpp \
    src/core/screen_item.cpp \
    src/common/input.cpp \
    src/core/properties.cpp \
    src/core/property.cpp \
    src/core/controller.cpp \
    src/common/line2.cpp \
    src/graphics/shader.cpp \
    src/graphics/text/font.cpp \
    src/graphics/text/font_atlas.cpp \
    src/graphics/blend_mode.cpp \
    src/graphics/stencil_func.cpp \
    src/graphics/stats.cpp \
    src/graphics/cell/paint.cpp \
    src/graphics/text/font_manager.cpp \
    src/common/random.cpp \
    src/common/size2.cpp \
    src/common/xform2.cpp \
    src/common/xform3.cpp \
    src/common/exception.cpp \
    src/common/string.cpp \
    src/core/window_layout.cpp \
    src/graphics/gl_errors.cpp \
    src/graphics/vertex.cpp \
    src/graphics/cell/painter.cpp \
    src/graphics/cell/painterpreter.cpp \
    src/graphics/cell/cell.cpp \
    src/graphics/graphics_context.cpp \
    src/graphics/cell/cell_canvas.cpp

HEADERS += \
    include/core/application.hpp \
    include/core/window.hpp \
    include/common/signal.hpp \
    include/core/widget.hpp \
    include/common/log.hpp \
    include/utils/debug.hpp \
    include/common/vector2.hpp \
    include/common/vector3.hpp \
    include/common/aabr.hpp \
    include/core/events/key_event.hpp \
    thirdparty/randutils/randutils.hpp \
    include/common/random.hpp \
    thirdparty/glfw/glfw3.h \
    thirdparty/glfw/glfw3native.h \
    include/graphics/gl_errors.hpp \
    include/common/system.hpp \
    include/graphics/gl_forwards.hpp \
    thirdparty/stb_image/stb_image.h \
    include/core/resource_manager.hpp \
    include/core/render_manager.hpp \
    include/common/color.hpp \
    include/utils/unused.hpp \
    include/core/layout.hpp \
    include/dynamic/layout/stack_layout.hpp \
    include/common/claim.hpp \
    include/graphics/raw_image.hpp \
    include/common/circle.hpp \
    include/common/time.hpp \
    include/utils/literals.hpp \
    include/common/padding.hpp \
    include/utils/reverse_iterator.hpp \
    include/dynamic/layout/overlayout.hpp \
    include/core/item.hpp \
    include/core/screen_item.hpp \
    include/utils/binding_accessors.hpp \
    include/core/events/mouse_event.hpp \
    include/common/input.hpp \  
    include/utils/macro_overload.hpp \
    include/core/properties.hpp \
    include/utils/apply_tuple.hpp \
    include/graphics/texture2.hpp \
    include/core/property.hpp \
    include/core/controller.hpp \
    include/utils/print_notf.hpp \
    include/common/line2.hpp \
    include/graphics/shader.hpp \
    include/graphics/text/font.hpp \
    include/graphics/text/font_atlas.hpp \
    include/graphics/vertex.hpp \
    include/graphics/blend_mode.hpp \
    include/graphics/stencil_func.hpp \
    include/graphics/gl_utils.hpp \
    include/graphics/cell/paint.hpp \
    include/graphics/stats.hpp \
    include/graphics/text/font_manager.hpp \
    include/graphics/text/freetype.hpp \
    include/utils/sfinae.hpp \
    include/common/hash.hpp \
    include/common/integer.hpp \
    include/common/set.hpp \
    include/common/string.hpp \
    include/common/vector.hpp \
    include/common/float.hpp \
    include/common/size2.hpp \
    include/common/xform2.hpp \
    include/common/xform3.hpp \
    include/common/exception.hpp \
    include/core/opengl.hpp \
    include/core/glfw.hpp \
    include/core/window_layout.hpp \
    include/utils/range.hpp \
    include/graphics/cell/cell.hpp \
    include/graphics/cell/painter.hpp \
    include/graphics/cell/painterpreter.hpp \
    include/graphics/scissor.hpp \
    include/graphics/cell/commands.hpp \
    include/graphics/cell/command_buffer.hpp \
    include/common/identifier.hpp \
    include/graphics/graphics_context.hpp \
    include/graphics/cell/cell_canvas.hpp \
    include/common/enum.hpp

QMAKE_CXX = ccache g++

DISTFILES += \
    app/main.py \
    res/shader/cell.frag \
    res/shader/cell.vert \
    res/shader/font.frag \
    res/shader/font.vert
