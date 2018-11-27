# The notf thirdparty library project
set(CURRENT_TARGET notf-thirdparty)

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

# discover all thirdparty header files
file(GLOB_RECURSE SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw-3.3.0/include
)

# add cross-plattform source files
add_sources(SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/

    boost/fiber/algo/work_stealing.cpp
    boost/fiber/algo/round_robin.cpp
    boost/fiber/algo/shared_work.cpp
    boost/fiber/algo/algorithm.cpp
    boost/fiber/context.cpp
    boost/fiber/fiber.cpp
    boost/fiber/properties.cpp
    boost/fiber/mutex.cpp
    boost/fiber/scheduler.cpp
    boost/fiber/numa/algo/work_stealing.cpp
    boost/fiber/timed_mutex.cpp
    boost/fiber/recursive_mutex.cpp
    boost/fiber/recursive_timed_mutex.cpp
    boost/fiber/barrier.cpp
    boost/fiber/condition_variable.cpp
    boost/fiber/future.cpp

    boost/context/execution_context.cpp

    fmt/format.cc
    fmt/posix.cc

    glad/glad.cc

    msgpack11/msgpack11.cpp

    tinyutf8/tinyutf8.cpp
)

# add plattform-dependent source files
if(NOTF_WINDOWS)
    add_sources(SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/
        boost/fiber/numa/windows/topology.cpp
        boost/fiber/numa/windows/pin_thread.cpp

        boost/context/windows/stack_traits.cpp
    )
else()
    add_sources(SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/
        boost/fiber/numa/linux/topology.cpp
        boost/fiber/numa/linux/pin_thread.cpp

        boost/context/posix/stack_traits.cpp
    )
endif()

# add boost::context assembly files
enable_language(ASM)
add_sources(SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/boost/context/asm/"
    make_${BOOST_CONTEXT_ARCHITECTURE}_${BOOST_CONTEXT_ABI}_${BOOST_CONTEXT_BINARY_FORMAT}_${ASM_FORMAT}
    jump_${BOOST_CONTEXT_ARCHITECTURE}_${BOOST_CONTEXT_ABI}_${BOOST_CONTEXT_BINARY_FORMAT}_${ASM_FORMAT}
    ontop_${BOOST_CONTEXT_ARCHITECTURE}_${BOOST_CONTEXT_ABI}_${BOOST_CONTEXT_BINARY_FORMAT}_${ASM_FORMAT}
)

# add google benchmark library only on request
if(NOTF_BUILD_BENCHMARKS)
    message(STATUS ">>> notf: Including benchmark in thirdparty library")
    add_sources(SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/benchmark
        benchmark_api_internal.cc
        benchmark.cc
        benchmark_main.cc
        benchmark_register.cc
        benchmark_runner.cc
        colorprint.cc
        commandlineflags.cc
        complexity.cc
        console_reporter.cc
        counter.cc
        csv_reporter.cc
        json_reporter.cc
        reporter.cc
        sleep.cc
        statistics.cc
        string_util.cc
        sysinfo.cc
        timers.cc
    )
endif()

# declare thirdparty library
add_library(${CURRENT_TARGET} STATIC
    ${SOURCE_FILES}
)

# thirdparty library include paths
target_include_directories(${CURRENT_TARGET} PUBLIC
    ${CMAKE_SOURCE_DIR}/thirdparty/include/
)

# global defines and options
target_compile_options(${CURRENT_TARGET} PRIVATE ${NOTF_GLOBAL_OPTIONS})
target_compile_definitions(${CURRENT_TARGET} PUBLIC ${NOTF_GLOBAL_DEFINES})

# CMake cannot not determine the linker language from the files, so we'll tell it
set_target_properties(${CURRENT_TARGET} PROPERTIES LINKER_LANGUAGE CXX)

# SUB-PROJECTS #########################################################################################################

# glfw
set( GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw-3.3.0)
