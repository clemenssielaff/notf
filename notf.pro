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
    QMAKE_CXXFLAGS_RELEASE += -fvisibility=hidden
    QMAKE_CXXFLAGS_RELEASE += -O3
    QMAKE_LFLAGS_RELEASE += -fvisibility=hidden
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
        test/test_core_zhierarchy.cpp

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
    src/common/real.cpp \
    src/common/system.cpp \
    src/core/render_manager.cpp \
    src/common/color.cpp \
    src/common/transform2.cpp \
    src/dynamic/layout/flexbox_layout.cpp \
    src/core/components/shape_component.cpp \
    src/core/components/canvas_component.cpp \
    src/common/size2i.cpp \
    src/common/size2r.cpp \
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
    thirdparty/nanovg/nanovg.c \
    src/python/pyvector2.cpp \
    src/python/pycomponent.cpp \
    src/python/pylayoutitem.cpp \
    src/python/pywidget.cpp \
    src/python/pypainter.cpp \
    src/python/pystacklayout.cpp \
    src/python/pylayoutroot.cpp \
    src/python/pywindow.cpp \
    src/python/pyglobal.cpp \
    src/python/pycanvascomponent.cpp \
    src/core/znode.cpp

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
    include/common/real.hpp \
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
    include/dynamic/layout/flexbox_layout.hpp \
    include/utils/smart_enabler.hpp \
    include/utils/enum_to_number.hpp \
    include/utils/unused.hpp \
    include/common/size2i.hpp \
    include/common/size2r.hpp \
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
    include/common/const.hpp \
    include/graphics/raw_image.hpp \
    thirdparty/nanovg/fontstash.h \
    thirdparty/nanovg/nanovg_gl_utils.h \
    thirdparty/nanovg/nanovg_gl.h \
    thirdparty/nanovg/nanovg.h \
    thirdparty/stb_truetype/stb_truetype.h \
    include/graphics/rendercontext.hpp \
    include/python/pyvector2.hpp \
    include/python/pycomponent.hpp \
    include/python/pylayoutitem.hpp \
    include/python/pywidget.hpp \
    include/python/pypainter.hpp \
    include/python/pystacklayout.hpp \
    include/python/pyglobal.hpp \
    include/python/pylayoutroot.hpp \
    include/python/pywindow.hpp \
    include/python/pycanvascomponent.hpp \
    include/graphics/painter.hpp \
    include/core/znode.hpp \
    include/common/memory_utils.hpp

QMAKE_CXX = ccache g++
