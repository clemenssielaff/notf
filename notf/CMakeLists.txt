# The notf core library project
set(CURRENT_TARGET notf-core)

# add core files
add_sources(CORE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/
    common/color.cpp
    common/filesystem.cpp
    common/mnemonic.cpp
    common/msgpack.cpp
    common/string.cpp
    common/thread.cpp
    common/thread_pool.cpp
    common/uuid.cpp
    common/utf8.cpp

    common/geo/matrix3.cpp
    common/geo/path2.cpp
    common/geo/polyline.cpp
    common/geo/segment.cpp

    graphic/frame_buffer.cpp
    graphic/graphics_context.cpp
    graphic/graphics_system.cpp
    graphic/opengl.cpp
    graphic/opengl_buffer.cpp
    graphic/raw_image.cpp
    graphic/shader.cpp
    graphic/shader_program.cpp
    graphic/texture.cpp
    graphic/vertex_object.cpp
#    graphic/prefab_factory.cpp

    graphic/plotter/design.cpp
    graphic/plotter/painter.cpp
    graphic/plotter/plotter.cpp

    graphic/renderer/fragment_renderer.cpp

    graphic/text/font.cpp
    graphic/text/font_atlas.cpp
    graphic/text/font_manager.cpp
    graphic/text/font_utils.cpp

    meta/half.cpp
)

# declare core library
add_library(${CURRENT_TARGET} STATIC
    ${CORE_FILES}
    ${NOTF_HEADERS}
    ${NOTF_DOCS} # just so they show up as part of the project
)

# additional requirements
find_package(Freetype REQUIRED)

if(NOTF_CLANG)
    set(BOOST_ROOT "/home/clemens/code/3rdparty/boost-1.70/clang")
endif()
set(Boost_USE_STATIC_LIBS        ON)  # only find static libs
set(Boost_USE_DEBUG_LIBS         OFF) # ignore debug libs and
set(Boost_USE_RELEASE_LIBS       ON)  # only find release libs
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.67 REQUIRED COMPONENTS fiber)

# include paths
target_include_directories(${CURRENT_TARGET} PUBLIC
    ${CMAKE_SOURCE_DIR}/include/
    ${Boost_INCLUDE_DIR}
    ${FREETYPE_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/thirdparty/fmt_5.3/include/
    ${CMAKE_SOURCE_DIR}/thirdparty/glad/include/
    ${CMAKE_SOURCE_DIR}/thirdparty/glfw_3.3/include/
    ${CMAKE_SOURCE_DIR}/thirdparty/lua_5.3.5/src/
    ${CMAKE_SOURCE_DIR}/thirdparty/randutils/
    ${CMAKE_SOURCE_DIR}/thirdparty/spdlog_1.3.1/include/
    ${CMAKE_SOURCE_DIR}/thirdparty/stb_image_2.22/
    ${CMAKE_SOURCE_DIR}/thirdparty/tiny-utf8_3.0/
)

# library paths
target_link_directories(${CURRENT_TARGET} PUBLIC 
    ${Boost_LIBRARY_DIRS}
)

# dependencies
target_link_libraries(${CURRENT_TARGET} PUBLIC
    Boost::fiber
    notf-fmt
    notf-glad
    glfw
    notf-lua
    uuid
    ${FREETYPE_LIBRARIES}
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
