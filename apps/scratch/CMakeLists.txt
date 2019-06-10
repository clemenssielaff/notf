# The notf scratch project
set(CURRENT_TARGET scratch)

# add scratch files
add_sources(SCRATCH_FILES ${CMAKE_CURRENT_SOURCE_DIR}
	scratch.cpp
    #tructured_buffer.cpp
    #msgpack_profile.cpp
    #ingest_test.cpp
)

# declare scratch executable
add_executable(${CURRENT_TARGET}
    ${SCRATCH_FILES}
    ${NOTF_HEADERS}
)

# scratch include paths
target_include_directories(${CURRENT_TARGET} PUBLIC
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_SOURCE_DIR}/thirdparty/benchmark_1.5/include
)

# scratch dependencies
target_link_libraries(${CURRENT_TARGET} PUBLIC
    notf-core
    benchmark
    ${CMAKE_THREAD_LIBS_INIT}
)


# global defines and options
target_compile_options(${CURRENT_TARGET} PRIVATE ${NOTF_GLOBAL_OPTIONS})
target_compile_definitions(${CURRENT_TARGET} PUBLIC ${NOTF_GLOBAL_DEFINES})

# target_link_options(${CURRENT_TARGET} PRIVATE 
#     -fsanitize=memory
#     -fno-omit-frame-pointer
#     -fno-optimize-sibling-calls
#     -O1
#     )
