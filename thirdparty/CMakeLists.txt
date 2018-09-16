# The notf thirdparty library project

# discover all thirdparty header files
file(GLOB_RECURSE THIRDPARTY_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# add thirdparty source files
add_sources(THIRDPARTY_FILES ${CMAKE_CURRENT_SOURCE_DIR}/
    fmt/format.cc
    fmt/posix.cc
)

# declare thirdparty library
add_library(notf-thirdparty STATIC
    ${THIRDPARTY_FILES}
)

# thirdparty library include paths
target_include_directories(notf-thirdparty PUBLIC
    ${CMAKE_SOURCE_DIR}/thirdparty/
)

# CMake cannot not determine the linker language from the files, so we'll tell it
set_target_properties(notf-thirdparty PROPERTIES LINKER_LANGUAGE CXX)
