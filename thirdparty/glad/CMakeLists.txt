cmake_minimum_required(VERSION 3.9)

set(CURRENT_TARGET notf-glad)

# project
project(${CURRENT_TARGET}
    LANGUAGES CXX
)

# sources
add_sources(GLAD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src
    glad.cc
)

# library
add_library (${CURRENT_TARGET} STATIC
    ${GLAD_SOURCES}
)

# include paths
target_include_directories(${CURRENT_TARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
)

# global defines and options
target_compile_options(${CURRENT_TARGET} PRIVATE ${NOTF_GLOBAL_OPTIONS})
target_compile_definitions(${CURRENT_TARGET} PUBLIC ${NOTF_GLOBAL_DEFINES})
