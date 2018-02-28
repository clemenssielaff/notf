#include "app/core/claim.hpp"

#include <cassert>

#include "common/log.hpp"

namespace notf {

void Claim::set_width_to_height(const float ratio_min, const float ratio_max)
{
    if (!is_real(ratio_min) || ratio_min < 0) {
        log_warning << "Invalid min ratio: " << ratio_min << " - using 0 instead.";
        if (!is_nan(ratio_max)) {
            log_warning << "Ignoring ratio_max value, since the min ratio constraint is set to 0.";
        }
        m_ratios = std::make_pair(Ratio(), Ratio());
        return;
    }

    const Ratio min_ratio = Ratio(ratio_min);
    if (is_nan(ratio_max)) {
        m_ratios = std::make_pair(min_ratio, min_ratio);
        return;
    }

    if (ratio_max < ratio_min) {
        log_warning << "Ignoring ratio_max value " << ratio_max << ", since it is smaller than the min_ratio "
                    << ratio_min;
        m_ratios = std::make_pair(min_ratio, min_ratio);
        return;
    }

    if (ratio_min == approx(0.)) {
        m_ratios = std::make_pair(min_ratio, min_ratio);
        return;
    }

    m_ratios = std::make_pair(min_ratio, Ratio(ratio_max));
}

Size2f Claim::apply(const Size2f& size) const
{
    Size2f result = size;

    // clamp to size first
    result.width  = clamp(size.width, m_horizontal.min(), m_horizontal.max());
    result.height = clamp(size.height, m_vertical.min(), m_vertical.max());

    // apply ratio constraints by shrinking one side within the valid range
    if (size.area() > precision_high<float>() && m_ratios.first.is_valid()) {
        assert(m_ratios.second.is_valid());

        const float current_ratio = size.height / size.width;
        const float valid_ratio
            = clamp(current_ratio, m_ratios.first.height_for_width(), m_ratios.second.height_for_width());
        if (valid_ratio < current_ratio) {
            result.height = clamp(size.width * valid_ratio, m_vertical.min(), m_vertical.max());
        }
        else if (valid_ratio > current_ratio) {
            result.width = clamp(size.height / valid_ratio, m_horizontal.min(), m_horizontal.max());
        }
    }

    return result;
}

std::ostream& operator<<(std::ostream& out, const Claim::Stretch& stretch)
{
    return out << "Claim::Stretch([ " << stretch.min() << "<= stretch.get_preferred()"
               << " <= " << stretch.max() << ", factor: " << stretch.scale_factor() << ", priority "
               << stretch.priority() << "])";
}

std::ostream& operator<<(std::ostream& out, const Claim& claim)
{
    const Claim::Stretch& horizontal    = claim.horizontal();
    const Claim::Stretch& vertical      = claim.horizontal();
    const std::pair<float, float> ratio = claim.width_to_height();
    return out << "Claim(\n"
               << "\thorizontal: [" << horizontal.min() << " <= " << horizontal.preferred()
               << " <= " << horizontal.max() << ", factor: " << horizontal.scale_factor() << ", priority "
               << horizontal.priority() << "]\n\tvertical: [" << vertical.min() << " <= " << vertical.preferred()
               << " <=" << vertical.max() << ", factor: " << vertical.scale_factor() << ", priority "
               << vertical.priority() << "]\n\tratio: " << ratio.first << " : " << ratio.second << ")";
}

} // namespace notf
