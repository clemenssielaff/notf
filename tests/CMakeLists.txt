# notf test runner project

# add test files
add_sources(TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src
    app/test_app.cpp
    app/test_graph.cpp
    app/test_node.cpp
    app/test_property.cpp
    app/test_property_handle.cpp

    common/test_msgpack.cpp
    common/test_string.cpp
    common/test_string_view.cpp
    common/test_uuid.cpp
    common/test_vector.cpp
    common/test_version.cpp

    meta/test_assert.cpp
    meta/test_debug.cpp
    meta/test_exception.cpp
    meta/test_hash.cpp
    meta/test_log.cpp
    meta/test_numeric.cpp
    meta/test_pointer.cpp
    meta/test_real.cpp
    meta/test_stringtype.cpp
    meta/test_time.cpp
    meta/test_typename.cpp
    meta/test_types.cpp

    reactive/test_pipeline.cpp
    reactive/test_publisher.cpp
    reactive/test_reactive_generator.cpp
    reactive/test_reactive_operator.cpp
    reactive/test_reactive_registry.cpp
    reactive/test_subscriber.cpp
)

# add test utility files
add_sources(TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include
    test_app_utils.hpp
    test_reactive_utils.hpp
    test_utils.hpp
)

# declare test runner executable
add_executable(notf-tests
    main.cpp
    ${TEST_FILES}
    ${NOTF_HEADERS}
)

# test runner include paths
target_include_directories(notf-tests PUBLIC
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_SOURCE_DIR}/thirdparty/
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
)

# test runner dependencies
target_link_libraries(notf-tests PUBLIC
    notf-core
)

# register the test runner as a test
add_test(NAME "NotfTestRunner" COMMAND notf-tests)

# create coverage for tests
if(NOTF_CREATE_COVERAGE)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-instr-generate -fcoverage-mapping")
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fprofile-instr-generate")
    else()
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} --coverage")
    endif()
endif()

# automatically run all (except slow) tests as part of the build and display coverage
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    option(NOTF_RUN_TESTS "Run tests" ON)
    if(NOTF_RUN_TESTS)
        message(STATUS ">>> notf: Building tests")

        set(LLVM_COV_ARGS -instr-profile=notf-tests.profdata -Xdemangler=llvm-cxxfilt -use-color -ignore-filename-regex="thirdparty|tests")
        add_custom_command(
            TARGET notf-tests
            POST_BUILD

            # run the tests
            COMMAND notf-tests exclude:[slow] --reporter compact

            # create a coverage report and display a summary report
            COMMAND llvm-profdata merge -sparse default.profraw -o notf-tests.profdata
            COMMAND llvm-cov report ./notf-tests ${LLVM_COV_ARGS}

            # store the complete report as html
            COMMAND llvm-cov show ./notf-tests ${LLVM_COV_ARGS} -show-instantiations=false -format="html" > coverage.html
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Executing notf test runner"
        )
    endif()
endif()
