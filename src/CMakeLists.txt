# The notf core library project
set(CURRENT_TARGET notf-core)

# add core files
add_sources(CORE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/
    app/application.cpp
    app/event_handler.cpp
    app/glfw_callbacks.cpp
    app/input.cpp
    app/render_manager.cpp
    app/resource_manager.cpp
    app/timer_pool.cpp

    app/graph/any_node.cpp
    app/graph/graph.cpp
    app/graph/property.cpp
    app/graph/root_node.cpp
    app/graph/scene.cpp
    app/graph/visualizer.cpp
    app/graph/window.cpp
    app/graph/window_driver.cpp

    app/widget/any_widget.cpp
    app/widget/widget_claim.cpp
    app/widget/widget_scene.cpp
    app/widget/widget_visualizer.cpp

    common/color.cpp
    common/filesystem.cpp
    common/mnemonic.cpp
    common/msgpack.cpp
    common/string.cpp
    common/thread.cpp
    common/thread_pool.cpp
    common/uuid.cpp

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

# add thirdparty files
add_sources(CORE_FILES ${CMAKE_SOURCE_DIR}/thirdparty/include/
    randutils/randutils.hpp
)

# declare core library
add_library(${CURRENT_TARGET} STATIC
    ${CORE_FILES}
    ${NOTF_HEADERS}
    ${NOTF_DOCS} # just so they show up as part of the project
)

# include paths
target_include_directories(${CURRENT_TARGET} PUBLIC
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_SOURCE_DIR}/thirdparty/include/
    ${CMAKE_SOURCE_DIR}/thirdparty/lib/glfw-3.3.0/include
    ${CMAKE_SOURCE_DIR}/thirdparty/lib/freetype-2.9.1/include
)

# dependencies
target_link_libraries(${CURRENT_TARGET} PUBLIC
    notf-thirdparty
    glfw
    uuid
    freetype
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
