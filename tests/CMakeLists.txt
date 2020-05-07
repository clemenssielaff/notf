# The notf test runner executable
set(CURRENT_TARGET tests)
add_executable(${CURRENT_TARGET})

# add test files
add_sources(TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}
#    common/test_any.cpp
#    common/test_mutex.cpp
    common/test_random.cpp
    common/test_string.cpp
#    common/test_string_view.cpp
#    common/test_thread.cpp
#    common/test_variant.cpp
    common/test_vector.cpp
    common/test_version.cpp

    common/geo/test_aabr.cpp
    common/geo/test_circle.cpp
    common/geo/test_color.cpp
#    common/geo/test_matrix3.cpp
#    common/geo/test_matrix4.cpp
    common/geo/test_polyline.cpp
    common/geo/test_vector2.cpp

    meta/test_assert.cpp
    meta/test_config.cpp
    meta/test_debug.cpp
    meta/test_exception.cpp
    meta/test_hash.cpp
    meta/test_integer.cpp
    meta/test_log.cpp
    meta/test_numeric.cpp
    meta/test_pointer.cpp
    meta/test_real.cpp
    meta/test_stringtype.cpp
    meta/test_system.cpp
    meta/test_time.cpp
#    meta/test_tuple.cpp
    meta/test_typename.cpp
    meta/test_types.cpp
)

# test runner sources
target_sources(${CURRENT_TARGET} PRIVATE
    main.cpp
    ${TEST_FILES}
    ${NOTF_HEADERS}
)

# test runner include paths
target_include_directories(${CURRENT_TARGET} PUBLIC
    ${CMAKE_SOURCE_DIR}/
    ${CMAKE_CURRENT_SOURCE_DIR}/
    ${CMAKE_SOURCE_DIR}/thirdparty/glm-0.9.9.8/
)

# test runner dependencies
target_link_libraries(${CURRENT_TARGET} PUBLIC
    notf::core
)

# global defines and options
target_compile_options(${CURRENT_TARGET} PRIVATE ${NOTF_GLOBAL_OPTIONS})
target_compile_definitions(${CURRENT_TARGET} PUBLIC ${NOTF_GLOBAL_DEFINES})
