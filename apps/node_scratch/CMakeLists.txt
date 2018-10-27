# The notf node scratch project

# add scratch files
add_sources(NODE_SCRATCH_FILES ${CMAKE_CURRENT_SOURCE_DIR}
    node_scratch.cpp
)

# declare scratch executable
add_executable(notf-node-scratch
    ${NODE_SCRATCH_FILES}
    ${NOTF_HEADERS}
)

# scratch include paths
target_include_directories(notf-node-scratch PUBLIC
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_SOURCE_DIR}/thirdparty/
)

# scratch dependencies
target_link_libraries(notf-node-scratch PUBLIC
    notf-thirdparty
    notf-core
)

# show all but selected warnings
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(notf-node-scratch PRIVATE
#        -Weverything
#        -Wpedantic
#        -Wno-padded
#        -Wno-weak-vtables   # used everywhere and not a problem
#        -Wno-c++98-compat
#        -Wno-c++98-compat-pedantic
    )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(notf-node-scratch PRIVATE
        -Wall
        -Wno-unknown-pragmas
        -Wno-pragmas
    )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(notf-node-scratch PRIVATE "/W4")
endif()
