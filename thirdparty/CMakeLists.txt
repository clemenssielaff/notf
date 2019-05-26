# The notf thirdparty library project
set(CURRENT_TARGET notf-thirdparty)

# PROJECT ##############################################################################################################


# add cross-plattform source files
add_sources(SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/
    glad/glad.cc
    msgpack11/msgpack11.cpp
    tinyutf8/tinyutf8.cpp
)

# some libraries complain about `stdlib=libc++` and this is the only way I found to get them to stop
add_compile_options("-Wno-unused-command-line-argument" )

# SUB-PROJECTS #########################################################################################################

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/benchmark_1.5)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/boost_1.70)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/fmt_5.3)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/freetype_2.10)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glad)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glfw_3.3)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lua_5.3.5)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/msgpack11_0.0.10)
