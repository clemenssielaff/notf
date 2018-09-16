# The notf scratch project

# add scratch files
add_sources(SCRATCH_FILES ${CMAKE_CURRENT_SOURCE_DIR}
    node_scratch.cpp
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
    ${CMAKE_THREAD_LIBS_INIT}
)
