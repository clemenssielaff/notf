# The notf core library
set(CURRENT_TARGET core)
add_library(${CURRENT_TARGET} STATIC)
add_library(notf::core ALIAS ${CURRENT_TARGET})

# add core files
add_sources(NOTF_CORE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/
    common/color.cpp
    common/random.cpp
    common/string.cpp
    common/thread.cpp

    common/geo/matrix3.cpp
    common/geo/path2.cpp
    common/geo/polygon2.cpp
)

# core sources
target_sources(${CURRENT_TARGET} PRIVATE
    ${NOTF_CORE_FILES}
    ${NOTF_HEADERS}
)

# core includes
target_include_directories(${CURRENT_TARGET} PUBLIC
    ${CMAKE_SOURCE_DIR}/
    ${CMAKE_SOURCE_DIR}/thirdparty/fmt-6.2.0/include
    ${CMAKE_SOURCE_DIR}/thirdparty/spdlog-1.5.0/include
    ${CMAKE_SOURCE_DIR}/thirdparty/pcg-cpp-0.98/include
)

# core dependencies
target_link_libraries(${CURRENT_TARGET} PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
    fmt::fmt-header-only
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
