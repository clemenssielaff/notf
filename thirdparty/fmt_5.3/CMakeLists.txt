cmake_minimum_required(VERSION 3.9)

set(CURRENT_TARGET notf-fmt)

# project
project(${CURRENT_TARGET}
    VERSION 5.3
    DESCRIPTION "A modern formatting library"
    LANGUAGES CXX
)

# sources
add_sources(FMT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src
    format.cc
    posix.cc
)

# library
add_library (${CURRENT_TARGET} STATIC
    ${FMT_SOURCES}
)

# include paths
target_include_directories(${CURRENT_TARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
)

# global defines and options
target_compile_options(${CURRENT_TARGET} PRIVATE ${NOTF_GLOBAL_OPTIONS})
target_compile_definitions(${CURRENT_TARGET} PUBLIC ${NOTF_GLOBAL_DEFINES})