# The notf scratch project

# add scratch files
add_sources(SCRATCH_FILES ${CMAKE_CURRENT_SOURCE_DIR}
    scratch.cpp
    #scratch2.cpp
)

# declare scratch executable
add_executable(notf-scratch
    ${SCRATCH_FILES}
    ${NOTF_HEADERS}
)

# scratch include paths
target_include_directories(notf-scratch PUBLIC
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_SOURCE_DIR}/thirdparty/
)

# scratch dependencies
target_link_libraries(notf-scratch PUBLIC
    notf-thirdparty
    notf-core
)

# show all but selected warnings
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(notf-scratch PRIVATE
#        -Weverything
#        -Wpedantic
#        -Wno-padded
#        -Wno-weak-vtables   # used everywhere and not a problem
#        -Wno-c++98-compat
#        -Wno-c++98-compat-pedantic

# clang vectorization analysis
#        -Rpass-missed=loop-vectorized
#        -Rpass-analysis=loop-vectorize
#        -fsave-optimization-record
#        -gline-tables-only
#        -gcolumn-info
    )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(notf-scratch PRIVATE
        -Wall
        -Wno-unknown-pragmas
        -Wno-pragmas
    )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(notf-scratch PRIVATE "/W4")
endif()
