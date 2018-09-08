# The notf scratch project

add_sources(SCRATCH_FILES ${CMAKE_CURRENT_SOURCE_DIR}
    node_scratch.cpp
)

add_executable(notf-scratch
    ${SCRATCH_FILES}
    ${NOTF_HEADERS}
)

target_include_directories(notf-scratch PUBLIC
    ${CMAKE_SOURCE_DIR}/include/
)

target_link_libraries(notf-scratch PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
)
