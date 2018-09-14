# notf test runner project

# add test files
add_sources(TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include
    catch.hpp
)
add_sources(TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src
    main.cpp
    meta/typename.cpp
)

# declare test runner executable
add_executable(notf-tests
    ${TEST_FILES}
    ${NOTF_HEADERS}
)

# test runner include paths
target_include_directories(notf-tests PUBLIC
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# register the test runner as a test
add_test(NAME "NotfTestRunner" COMMAND notf-tests)

if(NOT MSVC)
    option(NOTF_CREATE_COVERAGE "Create test coverage " ON)
    if(${NOTF_CREATE_COVERAGE})
        if(${NOTF_THREAD_SANITIZATION})
            message(AUTHOR_WARNING ">>> notf: Disabling coverage to avoid interference with thread sanitization.")
        else()
            message(STATUS ">>> notf: Creating coverage.")
            if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
                set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-instr-generate -fcoverage-mapping")
                set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fprofile-instr-generate")
            else()
                set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
            endif()
        endif()
    endif()
endif()

# automatically run all (except slow) tests as part of the build and display coverage
option(NOTF_RUN_TESTS "Run tests" ON)
if(${NOTF_RUN_TESTS})
    message(STATUS ">>> notf: Building tests")
    add_custom_command(
        TARGET notf-tests
        POST_BUILD

        # run the tests
        COMMAND notf-tests exclude:[slow] --reporter compact

        # create a coverage report and display a summary report
        COMMAND llvm-profdata merge -sparse default.profraw -o notf-tests.profdata
        COMMAND llvm-cov report ./notf-tests -instr-profile=notf-tests.profdata -use-color
        COMMAND echo
        COMMAND llvm-cov show ./notf-tests -instr-profile=notf-tests.profdata -region-coverage-lt=100 -Xdemangler=llvm-cxxfilt -use-color

        # store the complete report as html
        COMMAND llvm-cov show ./notf-tests -instr-profile=notf-tests.profdata -format="html" > coverage.html
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Executing notf test runner"
    )
endif()
