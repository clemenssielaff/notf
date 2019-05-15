cmake_minimum_required(VERSION 3.9)

set(CURRENT_TARGET notf-lua)

# project
project(${CURRENT_TARGET}
    VERSION 5.3.5.0
    DESCRIPTION "Powerful, efficient, lightweight, embeddable scripting language"
    LANGUAGES C
)

# sources
add_sources(LUA_SRC_CORE src
    lapi.c
    lcode.c
    lctype.c
    ldebug.c
    ldo.c
    ldump.c
    lfunc.c
    lgc.c
    llex.c
    lmem.c
    lobject.c
    lopcodes.c
    lparser.c
    lstate.c
    lstring.c
    ltable.c
    ltm.c
    lundump.c
    lvm.c
    lzio.c
)

add_sources(LUA_SRC_LIB src
    lauxlib.c
    lbaselib.c
    lbitlib.c
    lcorolib.c
    ldblib.c
    linit.c
    liolib.c
    lmathlib.c
    loadlib.c
    loslib.c
    lstrlib.c
    ltablib.c
    lutf8lib.c
)

# options
option(LUA_32BITS "Build Lua with 32-bit integers and 32-bit floats." OFF)
option(LUA_USE_C89 "Avoid the use of C99 or Windows-specific features" OFF)

# compile type specifics
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    message(STATUS "notf: Building Lua with \"LUA_USE_APICHECK\" defined")
    list(APPEND LUA_DEFINITIONS "LUA_USE_APICHECK" )
endif()

# system specifics
if(WIN32)
    list(APPEND LUA_DEFINITIONS "LUA_USE_WINDOWS" )
elseif(APPLE)
    list(APPEND LUA_DEFINITIONS "LUA_USE_MACOSX" )
elseif(UNIX)
    list(APPEND LUA_DEFINITIONS "LUA_USE_LINUX" )
    list(APPEND LUA_DEPENDENCIES ${CMAKE_DL_LIBS} )
endif()

# configuration
configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/src/luaconf.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/luaconf.h"
)

# library
add_library (${CURRENT_TARGET} STATIC
    ${LUA_SRC_CORE}
    ${LUA_SRC_LIB}
)

# dependencies
target_link_libraries(${CURRENT_TARGET} ${LUA_DEPENDENCIES})

# global defines and options
target_compile_options(${CURRENT_TARGET} PRIVATE ${NOTF_GLOBAL_OPTIONS})
target_compile_definitions(${CURRENT_TARGET} PUBLIC ${NOTF_GLOBAL_DEFINES} ${LUA_DEFINITIONS})