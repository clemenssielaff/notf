cmake_minimum_required(VERSION 3.9)

set(CURRENT_TARGET msgpack11)

# project
project(${CURRENT_TARGET})

# library
add_library (${CURRENT_TARGET} STATIC
    msgpack11.cpp
)

# global defines and options
target_compile_options(${CURRENT_TARGET} PRIVATE ${NOTF_GLOBAL_OPTIONS})
target_compile_definitions(${CURRENT_TARGET} PUBLIC ${NOTF_GLOBAL_DEFINES} ${LUA_DEFINITIONS})
