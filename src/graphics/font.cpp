#include <exception>

#include "graphics/font.hpp"

#include <nanovg/nanovg.h>

#include "common/log.hpp"
#include "common/string_utils.hpp"
#include "utils/smart_enabler.hpp"

namespace notf {

std::shared_ptr<Font> Font::load(NVGcontext* nvg_context, const std::string& name, const std::string& font_path)
{
    const int id = nvgCreateFont(nvg_context, name.c_str(), font_path.c_str());
    if (id == -1) {
        std::string message = string_format("Failed to load font \"%s\" from file: %s", name.c_str(), font_path.c_str());
        log_critical << message;
        throw std::runtime_error(message);
    }

    log_info << "Loaded Font \"" << name << "\" from: " << font_path;
    return std::make_shared<MakeSmartEnabler<Font>>(id);
}

} // namespace notf
