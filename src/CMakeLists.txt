# The notf core library project
set(CURRENT_TARGET notf-core)

# add core files
add_sources(CORE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/
    app/application.cpp
    app/graph.cpp
    app/node.cpp
    app/property.cpp
    app/root_node.cpp
    app/timer_pool.cpp
    app/window.cpp

    app/event/input.cpp
    app/event/handler.cpp

    app/widget/claim.cpp

    common/mnemonic.cpp
    common/msgpack.cpp
    common/string.cpp
    common/thread_pool.cpp
    common/uuid.cpp
)

# add thirdparty files
add_sources(CORE_FILES ${CMAKE_SOURCE_DIR}/thirdparty/include/
    randutils/randutils.hpp
)

# declare core library
add_library(${CURRENT_TARGET} STATIC
    ${CORE_FILES}
    ${NOTF_HEADERS}
)

# include paths
target_include_directories(${CURRENT_TARGET} PUBLIC
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_SOURCE_DIR}/thirdparty/include/
    ${CMAKE_SOURCE_DIR}/thirdparty/lib/glfw-3.3.0/include
)

# dependencies
target_link_libraries(${CURRENT_TARGET} PUBLIC
    notf-thirdparty
    glfw
    uuid
    ${CMAKE_THREAD_LIBS_INIT}
    ${OPENGL_LIBRARIES}
)

# global defines and options
target_compile_options(${CURRENT_TARGET} PRIVATE ${NOTF_GLOBAL_OPTIONS})
target_compile_definitions(${CURRENT_TARGET} PUBLIC ${NOTF_GLOBAL_DEFINES})

# create coverage for the core library
if(NOTF_CREATE_COVERAGE)
    if (NOTF_CLANG)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-instr-generate -fcoverage-mapping")
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fprofile-instr-generate")
    else()
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
    endif()
endif()
