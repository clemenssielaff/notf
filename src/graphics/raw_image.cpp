#include "graphics/raw_image.hpp"

#include <assert.h>
#include <exception>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include "common/log.hpp"
#include "common/string_utils.hpp"

namespace notf {

RawImage::RawImage(const std::string& image_path, int force_format)
    : m_width(0)
    , m_height(0)
    , m_bytes(0)
    , m_filepath(image_path)
    , m_data(nullptr)
{
    // load the image from file
    stbi_set_unpremultiply_on_load(1);
    stbi_convert_iphone_png_to_rgb(1);
    m_data = stbi_load(m_filepath.c_str(), &m_width, &m_height, &m_bytes, force_format);
    if (!m_data) {
        const std::string message = string_format("Failed to load image from '%s'", m_filepath.c_str());
        log_critical << message;
        throw std::runtime_error(std::move(message));
    }
    assert(m_height);
    assert(m_width);
    assert(m_bytes);

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
