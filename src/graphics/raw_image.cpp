#include "graphics/raw_image.hpp"

#include <assert.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include "common/exception.hpp"
#include "common/log.hpp"
#include "common/string.hpp"

namespace notf {

RawImage::RawImage(const std::string& image_path, int force_format)
    : m_filepath(image_path)
    , m_width(0)
    , m_height(0)
    , m_channels(0)
    , m_data(nullptr)
{
    stbi_set_flip_vertically_on_load(1);
    stbi_set_unpremultiply_on_load(1);
    stbi_convert_iphone_png_to_rgb(1);

    // load the image from file
    m_data = stbi_load(m_filepath.c_str(), &m_width, &m_height, &m_channels, force_format);
    if (!m_data) {
        throw_runtime_error(string_format("Failed to load image from '%s'", m_filepath.c_str()));
    }
    assert(m_height);
    assert(m_width);
    assert(m_channels);

    log_trace << "Loaded Image '" << m_filepath << "'";
}

RawImage::~RawImage()
{
    if (m_data) {
        stbi_image_free(m_data);
        log_trace << "Deleted Image '" << m_filepath << "'";
    }
}

} // namespace notf
