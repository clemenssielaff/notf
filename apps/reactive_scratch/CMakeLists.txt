# The notf reactive scratch project

# add scratch files
add_sources(REACTIVE_SCRATCH_FILES ${CMAKE_CURRENT_SOURCE_DIR}
    reactive_scratch.cpp
)

# declare scratch executable
add_executable(notf-reactive-scratch
    ${REACTIVE_SCRATCH_FILES}
    ${NOTF_HEADERS}
)

# scratch include paths
target_include_directories(notf-reactive-scratch PUBLIC
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_SOURCE_DIR}/thirdparty/
)

# scratch dependencies
target_link_libraries(notf-reactive-scratch PUBLIC
    notf-thirdparty
    notf-core
)
