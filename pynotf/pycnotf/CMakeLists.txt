cmake_minimum_required(VERSION 3.16)
project(pycnotf)

# use plain c++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# use ccache if available
find_program(CCACHE_PROGRAM ccache)
if(NOTF_CCACHE_PROGRAM)
    message(STATUS "notf: Using ccache")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${NOTF_CCACHE_PROGRAM})
endif()

# print additional information during compilation (like timings)
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")

add_subdirectory(thirdparty/pybind11-2.5.0)
add_subdirectory(thirdparty/fmt-6.2.0)

set(CURRENT_TARGET ${PROJECT_NAME})
pybind11_add_module(${CURRENT_TARGET}
    src/main.cpp
    src/py_aabr.cpp
    src/py_bezier.cpp
    src/py_size2.cpp
    src/py_vector2.cpp
)

target_compile_definitions(${CURRENT_TARGET} PUBLIC
    # NO_DOC
)

target_link_libraries(${CURRENT_TARGET} PUBLIC
    fmt::fmt-header-only
)

target_include_directories(${CURRENT_TARGET} PUBLIC
    /home/clemens/code/notf/
    /home/clemens/code/notf/BUILD/notf/clang10/debug/config
)
