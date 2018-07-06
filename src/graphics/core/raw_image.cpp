#include "graphics/core/raw_image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include "common/assert.hpp"
#include "common/exception.hpp"
#include "common/log.hpp"
#include "common/string.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

RawImage::RawImage(std::string image_path, const int force_format) : m_filepath(std::move(image_path))
{
    stbi_set_flip_vertically_on_load(0);
    stbi_set_unpremultiply_on_load(1);
    stbi_convert_iphone_png_to_rgb(1);

    // load the image from file
    m_data = stbi_load(m_filepath.c_str(), &m_width, &m_height, &m_channels, force_format);
    if (!m_data) {
        NOTF_THROW(runtime_error, "Failed to load image from \"{}\"", m_filepath);
    }
    NOTF_ASSERT(m_height);
    NOTF_ASSERT(m_width);
    NOTF_ASSERT(m_channels);

    log_trace << "Loaded Image '" << m_filepath << "'";
}

RawImage::~RawImage()
{
    if (m_data) {
        stbi_image_free(m_data);
        log_trace << "Deleted Image '" << m_filepath << "'";
    }
}

NOTF_CLOSE_NAMESPACE
