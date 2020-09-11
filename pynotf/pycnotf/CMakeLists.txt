cmake_minimum_required(VERSION 3.9)

project(
    pycnotf
    VERSION 0.5.0
)

# notf root dir
set(NOTF_ROOT_DIR "${PROJECT_SOURCE_DIR}/../../")

# include common configuration
set (CMAKE_MODULE_PATH "${NOTF_ROOT_DIR}/cmake")
include(config)

# produce your own copy of the configuration file in case this is built separately
configure_file (
    "${NOTF_ROOT_DIR}/notf/meta/config.hpp.in"
    "${CMAKE_BINARY_DIR}/notf/meta/config.hpp"
)

add_subdirectory(thirdparty/pybind11-2.5.0)
add_subdirectory(thirdparty/fmt-6.2.0)
add_subdirectory(thirdparty/nanovg)
add_subdirectory(thirdparty/utfcpp-3.1.1)

set(CURRENT_TARGET ${PROJECT_NAME})
pybind11_add_module(${CURRENT_TARGET}
    src/docstr.hpp
    src/main.cpp
    src/py_aabr.cpp
    src/py_bezier.cpp
#    src/py_circle.cpp
#    src/py_claim.cpp
    src/py_color.cpp
#    src/py_flexlayout.cpp
    src/py_matrix3.cpp
    src/py_nanovg.cpp
#    src/py_overlayout.cpp
#    src/py_padding.cpp
    src/py_polygon2.cpp
    src/py_segment.cpp
    src/py_size2.cpp
    src/py_triangle.cpp
    src/py_vector2.cpp

    ${NOTF_ROOT_DIR}/notf/common/color.cpp
)

target_compile_definitions(${CURRENT_TARGET} PUBLIC
#     NO_DOC
)

target_link_libraries(${CURRENT_TARGET} PUBLIC
    fmt::fmt-header-only
    nanovg
    utf8cpp
)

target_include_directories(${CURRENT_TARGET} PUBLIC
    /home/clemens/code/notf/
    ${CMAKE_BINARY_DIR}
)

