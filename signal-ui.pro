TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle qt

INCLUDEPATH *= thirdparty/ include/ test/

LIBS *= -L/home/clemens/code/thirdparty/glfw-3.2/INSTALL/lib/
LIBS *= -lglfw3 -lGL -ldl -lXinerama -lXrandr -lXcursor -lX11 -lXxf86vm -lpthread

# Test sources
CONFIG(test) {
    SOURCES += \
        test/test_main.cpp \
        test/test_common_string_utils.cpp

    HEADERS += \
        test/catch.hpp
}
else {
    SOURCES += src/main.cpp
}

SOURCES += \
    src/app/application.cpp \
    src/app/window.cpp \
    thirdparty/glad/glad.c \
    src/scratch.cpp \
    src/app/component.cpp \
    src/app/object.cpp \
    src/common/debug.cpp

HEADERS += \
    include/app/application.hpp \
    include/app/window.hpp \
    include/common/vector.hpp \
    include/common/keys.hpp \
    include/common/devel.hpp \
    thirdparty/glad/glad.h \
    thirdparty/KHR/khrplatform.h \
    thirdparty/linmath.h \
    include/app/component.hpp \
    include/app/object.hpp \
    include/common/debug.hpp \
    include/common/signal.hpp \
    include/common/string_utils.hpp \
    include/app/glfw_wrapper.hpp
