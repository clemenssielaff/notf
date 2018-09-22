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
