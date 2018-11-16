# The notf fiber project
set(CURRENT_TARGET notf-fiber)

# add source files
add_sources(SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}
    fiber_main.cpp
)
# declare executable
add_executable(${CURRENT_TARGET}
    ${SOURCE_FILES}
    ${NOTF_HEADERS}
)

# scratch include paths
target_include_directories(${CURRENT_TARGET} PUBLIC
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_SOURCE_DIR}/thirdparty/include/
)

# scratch dependencies
target_link_libraries(${CURRENT_TARGET} PUBLIC
    notf-thirdparty
    notf-core
    ${CMAKE_THREAD_LIBS_INIT}
)

# global defines and options
target_compile_options(${CURRENT_TARGET} PRIVATE ${NOTF_GLOBAL_OPTIONS})
target_compile_definitions(${CURRENT_TARGET} PUBLIC ${NOTF_GLOBAL_DEFINES})
