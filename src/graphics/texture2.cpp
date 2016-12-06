#include "assert.h"
#include <exception>

#include "graphics/texture2.hpp"

#include <nanovg/nanovg.h>

#include "common/log.hpp"
#include "common/string_utils.hpp"
#include "graphics/raw_image.hpp"
#include "utils/make_smart_enabler.hpp"

namespace notf {

std::shared_ptr<Texture2> Texture2::load(NVGcontext* nvg_context, const std::string& texture_path, int flags)
{
    // load the texture from file
    RawImage image(texture_path);
    const int width = image.get_width();
    const int height = image.get_height();
    int id = nvgCreateImageRGBA(nvg_context, width, height, flags, image.get_data());
    if (id == 0) {
        std::string message = string_format("Failed to load Texture2 file: %s", texture_path.c_str());
        log_critical << message;
        throw std::runtime_error(std::move(message));
    }

    // log success
    {
#if SIGNAL_LOG_LEVEL <= SIGNAL_LOG_LEVEL_INFO
        static const std::string grayscale = "grayscale";
        static const std::string rgb = "rgb";
        static const std::string rgba = "rgba";

        const int bytes = image.get_bytes_per_pixel();
        const std::string* format_name;
        if (bytes == 1) {
            format_name = &grayscale;
        }
        else if (bytes == 3) {
            format_name = &rgb;
        }
        else { // color + alpha
            assert(bytes == 4);
            format_name = &rgba;
        }
        log_info << "Loaded " << image.get_width() << "x" << image.get_height() << " " << *format_name
                  << " OpenGL texture with ID: " << id << " from: " << texture_path;
#endif
    }

    return std::make_shared<MakeSmartEnabler<Texture2>>(id, nvg_context);
}

Texture2::~Texture2()
{
    nvgDeleteImage(m_context, m_id);
    log_info << "Deleted Texture2 with ID: " << m_id;
}

Size2i Texture2::get_size() const
{
    Size2i result;
    nvgImageSize(m_context, m_id, &result.width, &result.height);
    return result;
}

} // namespace notf
