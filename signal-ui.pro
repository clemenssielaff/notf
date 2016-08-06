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
    src/app/application.cpp \
    src/app/window.cpp \
    thirdparty/glad/glad.c \
    src/app/component.cpp \
    src/app/object.cpp \
    src/app/keyboard.cpp \
    src/app/widget.cpp \
    src/common/log.cpp \
    src/common/vector2.cpp \
    src/common/aabr.cpp

HEADERS += \
    include/app/application.hpp \
    include/app/window.hpp \
    include/common/devel.hpp \
    thirdparty/glad/glad.h \
    thirdparty/KHR/khrplatform.h \
    thirdparty/linmath.h \
    include/app/component.hpp \
    include/app/object.hpp \
    include/common/signal.hpp \
    include/common/string_utils.hpp \
    include/app/glfw_wrapper.hpp \
    include/app/keyboard.hpp \
    include/app/widget.hpp \
    include/common/handle.hpp \
    include/common/vector_utils.hpp \
    include/common/log.hpp \
    include/common/debug.hpp \
    include/common/const.hpp \
    include/common/vector2.hpp \
    include/common/math.hpp \
    include/common/aabr.hpp \
    include/common/real.hpp
