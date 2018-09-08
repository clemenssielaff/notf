# notf test runner project

add_sources(TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src
    main.cpp
    meta/typename.cpp
)
add_sources(TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include
    catch.hpp
)

add_executable(notf-tests
    ${TEST_FILES}
    ${NOTF_HEADERS}
)

target_include_directories(notf-tests PUBLIC
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_test(NAME "NotfTestRunner" COMMAND notf-tests)
