#include "app/widget/claim.hpp"

#include "common/assert.hpp"
#include "common/log.hpp"

NOTF_OPEN_NAMESPACE

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

    if (approx(ratio_min, 0)) {
        m_ratios = std::make_pair(min_ratio, min_ratio);
        return;
    }

    m_ratios = std::make_pair(min_ratio, Ratio(ratio_max));
}

Size2f Claim::apply(const Size2f& size) const
{
    Size2f result = size;

    // clamp to size first
    result.width = clamp(size.width, m_horizontal.get_min(), m_horizontal.get_max());
    result.height = clamp(size.height, m_vertical.get_min(), m_vertical.get_max());

    // apply ratio constraints by shrinking one side within the valid range
    if (size.area() > precision_high<float>() && m_ratios.first.is_valid()) {
        NOTF_ASSERT(m_ratios.second.is_valid());

        const float current_ratio = size.height / size.width;
        const float valid_ratio
            = clamp(current_ratio, m_ratios.first.height_for_width(), m_ratios.second.height_for_width());
        if (valid_ratio < current_ratio) {
            result.height = clamp(size.width * valid_ratio, m_vertical.get_min(), m_vertical.get_max());
        }
        else if (valid_ratio > current_ratio) {
            result.width = clamp(size.height / valid_ratio, m_horizontal.get_min(), m_horizontal.get_max());
        }
    }

    return result;
}

std::ostream& operator<<(std::ostream& out, const Claim::Stretch& stretch)
{
    return out << "Claim::Stretch([ " << stretch.get_min() << "<= stretch.get_preferred()"
               << " <= " << stretch.get_max() << ", factor: " << stretch.get_scale_factor() << ", priority "
               << stretch.get_priority() << "])";
}

std::ostream& operator<<(std::ostream& out, const Claim& claim)
{
    const Claim::Stretch& horizontal = claim.get_horizontal();
    const Claim::Stretch& vertical = claim.get_horizontal();
    const std::pair<float, float> ratio = claim.get_width_to_height();
    return out << "Claim(\n"
               << "\thorizontal: [" << horizontal.get_min() << " <= " << horizontal.get_preferred()
               << " <= " << horizontal.get_max() << ", factor: " << horizontal.get_scale_factor() << ", priority "
               << horizontal.get_priority() << "]\n\tvertical: [" << vertical.get_min()
               << " <= " << vertical.get_preferred() << " <=" << vertical.get_max()
               << ", factor: " << vertical.get_scale_factor() << ", priority " << vertical.get_priority()
               << "]\n\tratio: " << ratio.first << " : " << ratio.second << ")";
}

NOTF_CLOSE_NAMESPACE
