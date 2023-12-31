#include "notf/graphic/raw_image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "notf/meta/log.hpp"

NOTF_USING_NAMESPACE;

// raw image ======================================================================================================== //

RawImage::RawImage(std::string image_path, const int force_format) : m_filepath(std::move(image_path)) {
    stbi_set_flip_vertically_on_load(0);
    stbi_set_unpremultiply_on_load(1);
    stbi_convert_iphone_png_to_rgb(1);

    // load the image from file
    m_data = stbi_load(m_filepath.c_str(), &m_size.width(), &m_size.height(), &m_channels, force_format);
    if (!m_data) { NOTF_THROW(ResourceError, "Failed to load image from \"{}\"", m_filepath); }
    NOTF_ASSERT(m_size.is_valid());
    NOTF_ASSERT(m_channels);

    NOTF_LOG_TRACE("Loaded Image \"{}\"", m_filepath);
}

RawImage::~RawImage() {
    if (m_data) {
        stbi_image_free(m_data);
        NOTF_LOG_TRACE("Deleted Image \"{}\"", m_filepath);
    }
}
