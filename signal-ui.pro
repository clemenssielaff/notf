TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle qt

INCLUDEPATH *= thirdparty/ include/ test/

LIBS *= -L/home/clemens/code/thirdparty/glfw-3.2.1/INSTALL/lib/
LIBS *= -lglfw3 -lGL -ldl -lpthread -lX11 -lXrandr -lXcursor -lXinerama -lXxf86vm #-lXi

# debug
CONFIG(debug, debug|release) {
    DEFINES += _DEBUG
}

# test
CONFIG(test) {
    SOURCES += \
        test/test_main.cpp \
        test/test_common_string_utils.cpp \
        test/test_common_signal.cpp

    HEADERS += \
        test/catch.hpp \
        test/catch_utils.hpp

    DEFINES += _DEBUG
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
    thirdparty/glad/glad.c \
    src/core/component.cpp \
    src/core/keyboard.cpp \
    src/core/widget.cpp \
    src/common/log.cpp \
    src/common/vector2.cpp \
    src/common/aabr.cpp \
    thirdparty/polypartition/polypartition.cpp \
    thirdparty/stb_image/stb_image.cpp \
    src/breakout.cpp \
    src/breakout/game.cpp \
    src/breakout/gameobject.cpp \
    src/breakout/ballobject.cpp \
    src/breakout/gamelevel.cpp \
    src/graphics/shader.cpp \
    src/graphics/texture2.cpp \
    src/core/resource_manager.cpp \
    src/common/real.cpp \
    src/common/system.cpp \
    src/breakout/spriterenderer.cpp \
    src/core/render_manager.cpp \
    src/common/color.cpp \
    src/core/components/render_component.cpp \
    src/core/components/texture_component.cpp \
    src/common/size2.cpp \
    src/dynamic/render/sprite.cpp \
    src/dynamic/color/singlecolor.cpp \
    src/dynamic/shape/aabr_shape.cpp \
    src/common/transform2.cpp \
    src/dynamic/layout/flexbox_layout.cpp \
    src/core/components/shape_component.cpp \
    src/core/layout_item.cpp \
    src/core/item_manager.cpp \
    src/dynamic/layout/fill_layout.cpp \
    src/core/abstract_item.cpp \
    src/core/abstract_layout_item.cpp \
    src/core/root_layout_item.cpp

HEADERS += \
    include/core/application.hpp \
    include/core/window.hpp \
    thirdparty/glad/glad.h \
    thirdparty/khr/khrplatform.h \
    thirdparty/linmath.h \
    include/core/component.hpp \
    include/common/signal.hpp \
    include/common/string_utils.hpp \
    include/core/glfw_wrapper.hpp \
    include/core/keyboard.hpp \
    include/core/widget.hpp \
    include/common/handle.hpp \
    include/common/vector_utils.hpp \
    include/common/log.hpp \
    include/common/debug.hpp \
    include/common/vector2.hpp \
    include/common/math.hpp \
    include/common/aabr.hpp \
    include/common/real.hpp \
    include/core/key_event.hpp \
    thirdparty/randutils/randutils.hpp \
    include/common/random.hpp \
    thirdparty/glfw/glfw3.h \
    thirdparty/glfw/glfw3native.h \
    include/graphics/gl_errors.hpp \
    include/common/system.hpp \
    include/graphics/gl_utils.hpp \
    include/graphics/gl_forwards.hpp \
    thirdparty/polypartition/polypartition.h \
    thirdparty/stb_image/std_image.h \
    include/breakout/game.hpp \
    include/breakout/gameobject.hpp \
    include/breakout/ballobject.hpp \
    include/breakout/gamelevel.hpp \
    include/graphics/shader.hpp \
    include/graphics/texture2.hpp \
    include/core/resource_manager.hpp \
    include/breakout/spriterenderer.hpp \
    include/core/render_manager.hpp \
    include/core/components/render_component.hpp \
    include/core/components/shape_component.hpp \
    include/core/components/color_component.hpp \
    include/common/color.hpp \
    include/common/enummap.hpp \
    include/core/components/texture_component.hpp \
    include/common/size2.hpp \
    include/common/int_utils.hpp \
    include/dynamic/render/sprite.hpp \
    include/dynamic/shape/aabr_shape.hpp \
    include/dynamic/color/singlecolor.hpp \
    include/common/transform2.hpp \
    include/dynamic/layout/flexbox_layout.hpp \
    include/common/size_range.hpp \
    include/utils/smart_enabler.hpp \
    include/utils/enum_to_number.hpp \
    include/utils/unused.hpp \
    include/core/layout_item.hpp \
    include/core/item_manager.hpp \
    include/dynamic/layout/fill_layout.hpp \
    include/core/abstract_item.hpp \
    include/core/abstract_layout_item.hpp \
    include/core/root_layout_item.hpp

DISTFILES += \
    res/shaders/test01.vert \
    res/shaders/test01.frag \
    res/shaders/sprite.vert \
    res/shaders/sprite.frag
