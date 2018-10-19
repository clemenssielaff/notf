# notf benchmark  project

# add benchmark files
add_sources(BENCHMARK_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src
    common/bench_msgpack.cpp
    common/bench_string.cpp
    common/bench_uuid.cpp
)

# declare benchmark executable
add_executable(notf-benchmark
    main.cpp
    ${BENCHMARK_FILES}
    ${NOTF_HEADERS}
)

# benchmark include paths
target_include_directories(notf-benchmark PUBLIC
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_SOURCE_DIR}/thirdparty/
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# benchmark dependencies
target_link_libraries(notf-benchmark PUBLIC
    notf-core
    notf-thirdparty
)

