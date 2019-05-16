cmake_minimum_required(VERSION 3.9)

set(CURRENT_TARGET notf-boost)

# project
project(${CURRENT_TARGET}
    VERSION 1.68
    DESCRIPTION "Subset of the Boost libraries used by notf"
    LANGUAGES CXX
)

# ASM MAGIC FOR BOOST::CONTEXT #########################################################################################
# from https://github.com/boost-cmake/boost/blob/master/libs/context/CMakeLists.txt

set(abi
    aapcs
    eabi
    ms
    n32
    n64
    o32
    o64
    sysv
)

set(binary_format
    elf
    macho
    pe
)

set(arch
    arm
    arm64
    mips32
    ppc32
    ppc64
    i386
    x86_64
)

if(WIN32)
    set(default_abi ms)
elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm")
    set(default_abi aapcs)
elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "mips")
    set(default_abi o32)
else()
    set(default_abi sysv)
endif()

if(APPLE)
    set(default_binary_format macho)
elseif(WIN32)
    set(default_binary_format pe)
else()
    set(default_binary_format elf)
endif()

if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm")
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(default_arch arm)
    else()
        set(default_arch arm64)
    endif()
elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "mips")
    set(default_arch mips32)
else()
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(default_arch i386)
    else()
        set(default_arch x86_64)
    endif()
endif()

set(BOOST_CONTEXT_ABI ${default_abi} CACHE STRING "ABI for Boost.Context (${abi})" )
set_property(CACHE BOOST_CONTEXT_ABI PROPERTY STRINGS "${abi}" )

set(BOOST_CONTEXT_BINARY_FORMAT ${default_binary_format} CACHE STRING "Binary format for Boost.Context (${binary_format})" )
set_property(CACHE BOOST_CONTEXT_BINARY_FORMAT PROPERTY STRINGS "${binary_format}" )

set(BOOST_CONTEXT_ARCHITECTURE ${default_arch} CACHE STRING "Architecture for Boost.Context (${arch})" )
set_property(CACHE BOOST_CONTEXT_ARCHITECTURE PROPERTY STRINGS "${arch}" )

if(NOTF_MSVC)
set(ASM_FORMAT "masm.asm")
else()
set(ASM_FORMAT "gas.S")
endif()

# PROJECT ##############################################################################################################

# discover all header files
file(GLOB_RECURSE SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
)

# add cross-plattform source files
add_sources(SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/
    fiber/algo/work_stealing.cpp
    fiber/algo/round_robin.cpp
    fiber/algo/shared_work.cpp
    fiber/algo/algorithm.cpp
    fiber/context.cpp
    fiber/fiber.cpp
    fiber/properties.cpp
    fiber/mutex.cpp
    fiber/scheduler.cpp
    fiber/numa/algo/work_stealing.cpp
    fiber/timed_mutex.cpp
    fiber/recursive_mutex.cpp
    fiber/recursive_timed_mutex.cpp
    fiber/barrier.cpp
    fiber/condition_variable.cpp
    fiber/future.cpp

    context/execution_context.cpp
)

# add plattform-dependent source files
if(NOTF_WINDOWS)
    add_sources(SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/
        fiber/numa/windows/topology.cpp
        fiber/numa/windows/pin_thread.cpp

        context/windows/stack_traits.cpp
    )
enable_language(ASM_MASM)
else()
    add_sources(SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/
        fiber/numa/linux/topology.cpp
        fiber/numa/linux/pin_thread.cpp

        context/posix/stack_traits.cpp
    )
enable_language(ASM)
endif()

# add boost::context assembly files
add_sources(SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/context/asm/"
    make_${BOOST_CONTEXT_ARCHITECTURE}_${BOOST_CONTEXT_ABI}_${BOOST_CONTEXT_BINARY_FORMAT}_${ASM_FORMAT}
    jump_${BOOST_CONTEXT_ARCHITECTURE}_${BOOST_CONTEXT_ABI}_${BOOST_CONTEXT_BINARY_FORMAT}_${ASM_FORMAT}
    ontop_${BOOST_CONTEXT_ARCHITECTURE}_${BOOST_CONTEXT_ABI}_${BOOST_CONTEXT_BINARY_FORMAT}_${ASM_FORMAT}
)

# library
add_library(${CURRENT_TARGET} STATIC
    ${SOURCE_FILES}
)

# include paths
target_include_directories(${CURRENT_TARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
    ${CMAKE_SOURCE_DIR}/thirdparty/fmt_5.3/include
)

# dependencies
target_link_libraries(${CURRENT_TARGET} notf-fmt)

# global defines and options
target_compile_options(${CURRENT_TARGET} PRIVATE ${NOTF_GLOBAL_OPTIONS})
target_compile_definitions(${CURRENT_TARGET} PUBLIC ${NOTF_GLOBAL_DEFINES}
    BOOST_CONTEXT_EXPORT
)

# CMake cannot not determine the linker language from the files, so we'll tell it
set_target_properties(${CURRENT_TARGET} PROPERTIES LINKER_LANGUAGE CXX)