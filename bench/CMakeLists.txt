# notf benchmark  project
set(CURRENT_TARGET notf-benchmark)

# add benchmark files
add_sources(BENCHMARK_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src
    common/bench_math.cpp
    common/bench_msgpack.cpp
    common/bench_string.cpp
    common/bench_uuid.cpp
    common/bench_stream.cpp
)

# declare benchmark executable
add_executable(${CURRENT_TARGET}
    main.cpp
    ${BENCHMARK_FILES}
    ${NOTF_HEADERS}
)

# benchmark include paths
target_include_directories(${CURRENT_TARGET} PUBLIC
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_SOURCE_DIR}/thirdparty/benchmark_1.5/
    ${CMAKE_SOURCE_DIR}/thirdparty/msgpack11_0.0.10/
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# benchmark dependencies
target_link_libraries(${CURRENT_TARGET} PUBLIC
    notf-core
    benchmark
    msgpack11
)

# global defines and options
target_compile_options(${CURRENT_TARGET} PRIVATE ${NOTF_GLOBAL_OPTIONS})
target_compile_definitions(${CURRENT_TARGET} PUBLIC ${NOTF_GLOBAL_DEFINES})
