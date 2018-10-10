# The notf core library project

# add core files
add_sources(CORE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/
    common/random.cpp
    common/timer_pool.cpp
    common/msgpack.cpp
)

# add thirdparty files
add_sources(CORE_FILES ${CMAKE_SOURCE_DIR}/thirdparty/
    randutils/randutils.hpp
)

# declare core library
add_library(notf-core STATIC
    ${CORE_FILES}
    ${NOTF_HEADERS}
)

# core library include paths
target_include_directories(notf-core PUBLIC
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_SOURCE_DIR}/thirdparty/
)

# core library dependencies
target_link_libraries(notf-core PUBLIC
    notf-thirdparty
    ${CMAKE_THREAD_LIBS_INIT}
)

# create coverage for the core library
if(${NOTF_CREATE_COVERAGE})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-instr-generate -fcoverage-mapping")
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fprofile-instr-generate")
    else()
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
    endif()
endif()

# show all but selected warnings
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(notf-core PRIVATE
#        -Weverything
#        -Wpedantic
#        -Wno-padded
#        -Wno-weak-vtables   # used everywhere and not a problem
#        -Wno-four-char-constants      # used in system.hpp to detect system endianness
#        -Wno-c++98-compat
#        -Wno-c++98-compat-pedantic
    )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(notf-core PRIVATE
        -Wall
        -Wno-multichar
    )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    target_compile_options(notf-core PRIVATE "/W4")
endif()
