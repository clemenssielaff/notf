TARGET = notf
TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle qt

INCLUDEPATH *= thirdparty/ include/ test/

# glfw
LIBS *= -L/home/clemens/code/thirdparty/glfw-3.2.1/INSTALL/lib/
LIBS *= -lglfw3 -lGL -lX11 -lXxf86vm -lpthread -ldl -lXcursor -lXrandr -lXinerama

# python
LIBS *= -L/home/clemens/code/thirdparty/Python-3.5.2/
LIBS *= -lpython3.5m
INCLUDEPATH *= /home/clemens/code/thirdparty/Python-3.5.2/INSTALL/include/python3.5m/

CONFIG(release, debug|release) {
#    message("Building in Release Mode.")
    DEFINES += "SIGNAL_LOG_LEVEL=4"
    QMAKE_CXXFLAGS_RELEASE += -fvisibility=hidden -s
    QMAKE_CXXFLAGS_RELEASE += -O3
    QMAKE_LFLAGS_RELEASE += -fvisibility=hidden -s
    QMAKE_LFLAGS_RELEASE += -O3
}

# debug
CONFIG(debug, debug|release) {
#    message("Building in Debug Mode.")
    DEFINES += _DEBUG
}

# test
CONFIG(test) {
#    message("Building in Test Mode.")

    SOURCES += \
        test/test_main.cpp \
        test/test_common_string_utils.cpp \
        test/test_common_signal.cpp \
        test/test_common_color.cpp \
        test/test_common_circle.cpp \
        test/test_common_aabr.cpp

    HEADERS += \
        test/catch.hpp

    DEFINES += _DEBUG _TEST
    QMAKE_CXXFLAGS *= -fprofile-arcs -ftest-coverage
    QMAKE_LFLAGS *= -lgcov -coverage
}
else {
    SOURCES += \
        src/main.cpp \
        src/scratch.cpp \
}

SOURCES += \
    src/core/application.cpp \
    src/core/window.cpp \
    src/core/component.cpp \
    src/core/widget.cpp \
    src/common/log.cpp \
    src/common/vector2.cpp \
    src/common/aabr.cpp \
    src/core/resource_manager.cpp \
    src/common/system.cpp \
    src/core/render_manager.cpp \
    src/common/color.cpp \
    src/common/transform2.cpp \
    src/core/components/shape_component.cpp \
    src/core/components/canvas_component.cpp \
    src/common/size2i.cpp \
    src/core/layout.cpp \
    src/core/layout_root.cpp \
    src/dynamic/layout/stack_layout.cpp \
    src/core/layout_item.cpp \
    src/core/object_manager.cpp \
    src/common/claim.cpp \
    src/python/interpreter.cpp \
    src/core/object.cpp \
    src/python/pynotf.cpp \
    src/graphics/raw_image.cpp \
    src/graphics/texture2.cpp \
    thirdparty/nanovg/nanovg.c \
    src/graphics/painter.cpp \
    src/python/py_canvascomponent.cpp \
    src/python/py_layoutroot.cpp \
    src/python/py_painter.cpp \
    src/python/py_stacklayout.cpp \
    src/python/py_vector2.cpp \
    src/python/py_widget.cpp \
    src/python/py_window.cpp \
    src/python/py_color.cpp \
    src/python/py_aabr.cpp \
    src/common/circle.cpp \
    src/python/py_circle.cpp \
    src/python/py_texture2.cpp \
    src/common/size2f.cpp \
    src/python/py_size.cpp \
    src/common/time.cpp \
    src/graphics/font.cpp \
    src/python/py_resourcemanager.cpp \
    src/python/py_font.cpp \
    src/common/padding.cpp \
    src/python/py_padding.cpp \
    src/python/py_claim.cpp \
    src/python/py_layout.cpp \
    src/core/state.cpp \
    src/python/py_state.cpp \
    src/dynamic/layout/overlayout.cpp \
    src/python/py_overlayout.cpp

HEADERS += \
    include/core/application.hpp \
    include/core/window.hpp \
    include/core/component.hpp \
    include/common/signal.hpp \
    include/common/string_utils.hpp \
    include/core/glfw_wrapper.hpp \
    include/common/keyboard.hpp \
    include/core/widget.hpp \
    include/common/handle.hpp \
    include/common/vector_utils.hpp \
    include/common/log.hpp \
    include/common/debug.hpp \
    include/common/vector2.hpp \
    include/common/math.hpp \
    include/common/aabr.hpp \
    include/core/events/key_event.hpp \
    thirdparty/randutils/randutils.hpp \
    include/common/random.hpp \
    thirdparty/glfw/glfw3.h \
    thirdparty/glfw/glfw3native.h \
    include/graphics/gl_errors.hpp \
    include/common/system.hpp \
    include/graphics/gl_utils.hpp \
    include/graphics/gl_forwards.hpp \
    thirdparty/stb_image/stb_image.h \
    include/core/resource_manager.hpp \
    include/core/render_manager.hpp \
    include/core/components/shape_component.hpp \
    include/core/components/canvas_component.hpp \
    include/common/color.hpp \
    include/common/enummap.hpp \
    include/common/int_utils.hpp \
    include/common/transform2.hpp \
    include/utils/smart_enabler.hpp \
    include/utils/enum_to_number.hpp \
    include/utils/unused.hpp \
    include/common/size2i.hpp \
    include/core/layout.hpp \
    include/core/layout_root.hpp \
    include/dynamic/layout/stack_layout.hpp \
    include/common/index.hpp \
    include/core/layout_item.hpp \
    include/core/object_manager.hpp \
    include/common/claim.hpp \
    include/python/interpreter.hpp \
    include/core/object.hpp \
    include/python/pynotf.hpp \
    include/graphics/raw_image.hpp \
    include/graphics/texture2.hpp \
    thirdparty/nanovg/fontstash.h \
    thirdparty/nanovg/nanovg_gl_utils.h \
    thirdparty/nanovg/nanovg_gl.h \
    thirdparty/nanovg/nanovg.h \
    thirdparty/stb_truetype/stb_truetype.h \
    include/graphics/rendercontext.hpp \
    include/graphics/painter.hpp \
    include/common/memory_utils.hpp \
    include/common/float_utils.hpp \
    include/common/circle.hpp \
    include/common/size2f.hpp \
    include/common/hash_utils.hpp \
    include/common/time.hpp \
    include/graphics/font.hpp \
    include/common/literals.hpp \
    include/common/padding.hpp \
    include/utils/reverse_iterator.hpp \
    include/common/set_utils.hpp \
    include/core/state.hpp \
    include/common/os_utils.hpp \
    include/dynamic/layout/overlayout.hpp

QMAKE_CXX = ccache g++
