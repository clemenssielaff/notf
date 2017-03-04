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
    DEFINES += "SIGNAL_LOG_LEVEL=4" "NDEBUG"
    QMAKE_CXXFLAGS_RELEASE += -fvisibility=hidden -s
    QMAKE_CXXFLAGS_RELEASE += -O3 #-Os
    QMAKE_LFLAGS_RELEASE += -fvisibility=hidden -s
    QMAKE_LFLAGS_RELEASE += -O3 #-Os

    # python
    LIBS *= -L/home/clemens/code/thirdparty/Python-3.5.2/
    LIBS *= -lpython3.5m
    INCLUDEPATH *= /home/clemens/code/thirdparty/Python-3.5.2/INSTALL/include/python3.5m/
}

# debug
CONFIG(debug, debug|release) {
#    message("Building in Debug Mode.")
    DEFINES += _DEBUG

    # python
    LIBS *= -L/home/clemens/code/thirdparty/Python-3.5.2/
    LIBS *= -lpython3.5dm
    INCLUDEPATH *= /home/clemens/code/thirdparty/Python-3.5.2/INSTALL/include/python3.5dm/
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
        test/test_vector2.cpp

    HEADERS += \
        test/catch.hpp
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
    src/common/transform2.cpp \
    src/common/size2i.cpp \
    src/core/layout.cpp \
    src/core/layout_root.cpp \
    src/dynamic/layout/stack_layout.cpp \
    src/common/claim.cpp \
    src/graphics/raw_image.cpp \
    src/graphics/texture2.cpp \
    src/common/circle.cpp \
    src/common/size2f.cpp \
    src/common/time.cpp \
    src/common/padding.cpp \
    src/dynamic/layout/overlayout.cpp \
    src/core/item.cpp \
    src/core/layout_item.cpp \
    src/core/controller.cpp \
    src/common/input.cpp \
    src/core/properties.cpp \
    src/core/property.cpp \
    src/common/line2.cpp \
    src/graphics/shader.cpp \
    src/graphics/text/font.cpp \
    src/graphics/text/font_atlas.cpp \
    src/graphics/blend_mode.cpp \
    src/graphics/render_context.cpp \
    src/graphics/cell.cpp \
    src/graphics/stats.cpp \
    src/graphics/painter.cpp \
    src/common/transform3.cpp \
    src/graphics/text/font_manager.cpp \
    src/common/random.cpp \
    src/common/float_utils.cpp

HEADERS += \
    include/core/application.hpp \
    include/core/window.hpp \
    include/common/signal.hpp \
    include/common/string_utils.hpp \
    include/core/glfw_wrapper.hpp \
    include/core/widget.hpp \
    include/common/vector_utils.hpp \
    include/common/log.hpp \
    include/common/debug.hpp \
    include/common/vector2.hpp \
    include/common/vector3.hpp \
    include/common/math.hpp \
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
    include/common/enummap.hpp \
    include/common/int_utils.hpp \
    include/common/transform2.hpp \
    include/utils/enum_to_number.hpp \
    include/utils/unused.hpp \
    include/common/size2i.hpp \
    include/core/layout.hpp \
    include/core/layout_root.hpp \
    include/dynamic/layout/stack_layout.hpp \
    include/common/index.hpp \
    include/common/claim.hpp \
    include/graphics/raw_image.hpp \
    include/common/memory_utils.hpp \
    include/common/float_utils.hpp \
    include/common/circle.hpp \
    include/common/size2f.hpp \
    include/common/hash_utils.hpp \
    include/common/time.hpp \
    include/common/literals.hpp \
    include/common/padding.hpp \
    include/utils/reverse_iterator.hpp \
    include/common/set_utils.hpp \
    include/common/os_utils.hpp \
    include/dynamic/layout/overlayout.hpp \
    include/utils/make_smart_enabler.hpp \
    include/core/item.hpp \
    include/core/layout_item.hpp \
    include/core/controller.hpp \
    include/utils/binding_accessors.hpp \
    include/core/events/mouse_event.hpp \
    include/common/input.hpp \  
    include/utils/macro_overload.hpp \
    include/core/properties.hpp \
    include/utils/apply_tuple.hpp \
    include/graphics/texture2.hpp \
    include/core/property.hpp \
    include/utils/print_notf.hpp \
    include/common/line2.hpp \
    include/graphics/shader.hpp \
    include/graphics/text/font.hpp \
    include/graphics/text/font_atlas.hpp \
    include/graphics/vertex.hpp \
    include/graphics/blend_mode.hpp \
    include/graphics/gl_utils.hpp \
    include/graphics/render_context.hpp \
    include/graphics/painter.hpp \
    include/graphics/stats.hpp \
    include/graphics/cell.hpp \
    include/common/transform3.hpp \
    include/graphics/text/font_manager.hpp \
    include/graphics/text/freetype.hpp \
    include/utils/sfinae.hpp

QMAKE_CXX = ccache g++

DISTFILES += \
    app/main.py \
    res/shader/cell.frag \
    res/shader/cell.vert \
    res/shader/font.frag \
    res/shader/font.vert
