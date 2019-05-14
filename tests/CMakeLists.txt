# notf test runner project
set(CURRENT_TARGET notf-tests)

# add test files
add_sources(TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src
    app/test_application.cpp
    app/test_driver.cpp
#    app/test_event_handler.cpp
    app/test_graph.cpp
    app/test_input.cpp
    app/test_node.cpp # still unfinished
    app/test_property.cpp # still unfinished
#    app/test_root_node.cpp
#    app/test_slot.cpp
    app/test_timer_pool.cpp
#    app/test_widget.cpp
    app/test_window.cpp # still unfinished

    common/test_any.cpp
    common/test_arithmetic.cpp
    common/test_mutex.cpp
    common/test_polyline.cpp
    common/test_msgpack.cpp
    common/test_random.cpp
    common/test_string.cpp
    common/test_string_view.cpp
    common/test_thread.cpp
    common/test_uuid.cpp
    common/test_variant.cpp
    common/test_vector.cpp
    common/test_version.cpp

    meta/test_assert.cpp
    meta/test_debug.cpp
    meta/test_exception.cpp
    meta/test_hash.cpp
    meta/test_integer.cpp
    meta/test_log.cpp
    meta/test_numeric.cpp
    meta/test_pointer.cpp
    meta/test_real.cpp
    meta/test_stringtype.cpp
    meta/test_time.cpp
    meta/test_tuple.cpp
    meta/test_typename.cpp
    meta/test_types.cpp

    reactive/test_pipeline.cpp
    reactive/test_publisher.cpp
    reactive/test_reactive_operator.cpp
    reactive/test_reactive_registry.cpp
    reactive/test_subscriber.cpp
    reactive/test_trigger.cpp
)

# add test utility files
add_sources(TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include
    test/app.hpp
    test/reactive.hpp
    test/utils.hpp
)

# declare test runner executable
add_executable(${CURRENT_TARGET}
    main.cpp
    ${TEST_FILES}
    ${NOTF_HEADERS}
)

# test runner include paths
target_include_directories(${CURRENT_TARGET} PUBLIC
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_SOURCE_DIR}/thirdparty/
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
)

# test runner dependencies
target_link_libraries(${CURRENT_TARGET} PUBLIC
    notf-core
)

# global defines and options
target_compile_options(${CURRENT_TARGET} PRIVATE ${NOTF_GLOBAL_OPTIONS})
target_compile_definitions(${CURRENT_TARGET} PUBLIC ${NOTF_GLOBAL_DEFINES})

