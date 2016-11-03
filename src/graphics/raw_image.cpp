#include "graphics/raw_image.hpp"

#include <assert.h>
#include <exception>

#include <stb_image/std_image.h>

#include "common/log.hpp"
#include "common/string_utils.hpp"

namespace notf {

Image::Image(const std::string& image_path)
    : m_width(0)
    , m_height(0)
    , m_bytes(0)
    , m_filepath(image_path)
    , m_data(nullptr)
{
    // load the image from file
    m_data = stbi_load(m_filepath.c_str(), &m_width, &m_height, &m_bytes, 0);
    if (!m_data) {
        const std::string message = string_format("Failed to load image from '%s'", m_filepath.c_str());
        log_critical << message;
        throw std::runtime_error(std::move(message));
    }

    log_trace << "Loaded Image '" << m_filepath << "'";
}

Image::~Image()
{
    if (m_data) {
        stbi_image_free(m_data);
        log_trace << "Destructed Image '" << m_filepath << "'";
    }
}

} // namespace notf
