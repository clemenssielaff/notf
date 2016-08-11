TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle qt

INCLUDEPATH *= thirdparty/ include/ test/

LIBS *= -L/home/clemens/code/thirdparty/glfw-3.2/INSTALL/lib/
LIBS *= -lglfw3 -lGL -ldl -lXinerama -lXrandr -lXcursor -lX11 -lXxf86vm -lpthread

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
    src/common/const.cpp \
    src/core/shapecomponent.cpp \
    src/core/texturecomponent.cpp

HEADERS += \
    include/core/application.hpp \
    include/core/window.hpp \
    include/common/devel.hpp \
    thirdparty/glad/glad.h \
    thirdparty/KHR/khrplatform.h \
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
    include/common/const.hpp \
    include/common/vector2.hpp \
    include/common/math.hpp \
    include/common/aabr.hpp \
    include/common/real.hpp \
    include/core/shapecomponent.hpp \
    include/core/texturecomponent.hpp
