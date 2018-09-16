# The notf core library project

# add core files
add_sources(CORE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/
    meta/random.cpp
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
)
