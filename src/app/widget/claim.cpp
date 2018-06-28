#include "app/widget/claim.hpp"

#include "common/assert.hpp"
#include "common/log.hpp"

NOTF_OPEN_NAMESPACE

void Claim::set_ratio_limits(Rationali ratio_min, Rationali ratio_max)
{
    if (ratio_min.is_zero()) {
        if (!ratio_max.is_zero()) {
            log_warning << "Ignoring ratio_max value, since the ratio_min constraint is set to zero.";
        }
        m_ratios = {};
        return;
    }

    if (ratio_max < ratio_min) {
        std::swap(ratio_max, ratio_min);
    }

    if (ratio_max.is_zero()) {
        m_ratios = Ratios{ratio_min, std::move(ratio_min)};
    }
    else {
        m_ratios = Ratios{std::move(ratio_min), std::move(ratio_max)};
    }
}

Size2f Claim::apply(Size2f size) const
{
    // clamp to size first
    size.width = clamp(size.width, m_horizontal.get_min(), m_horizontal.get_max());
    size.height = clamp(size.height, m_vertical.get_min(), m_vertical.get_max());

    // apply ratio constraints by shrinking one side within the valid range
    if (!is_zero(size.area()) && !m_ratios.get_lower_bound().is_zero()) {
        NOTF_ASSERT(!m_ratios.get_upper_bound().is_zero());
        const float current_ratio = size.width / size.height;
        const float valid_ratio
            = clamp(current_ratio, m_ratios.get_lower_bound().as_real(), m_ratios.get_upper_bound().as_real());
        if (valid_ratio < current_ratio) {
            size.height = clamp(size.width / valid_ratio, m_vertical.get_min(), m_vertical.get_max());
        }
        else if (valid_ratio > current_ratio) {
            size.width = clamp(size.height * valid_ratio, m_horizontal.get_min(), m_horizontal.get_max());
        }
    }

    return size;
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
    const Claim::Ratios& ratios = claim.get_ratio_limits();
    return out << "Claim(\n"
               << "\thorizontal: [" << horizontal.get_min() << " <= " << horizontal.get_preferred()
               << " <= " << horizontal.get_max() << ", factor: " << horizontal.get_scale_factor() << ", priority "
               << horizontal.get_priority() << "]\n\tvertical: [" << vertical.get_min()
               << " <= " << vertical.get_preferred() << " <=" << vertical.get_max()
               << ", factor: " << vertical.get_scale_factor() << ", priority " << vertical.get_priority()
               << "]\n\tratio: " << ratios.get_lower_bound() << " : " << ratios.get_upper_bound() << ")";
}

NOTF_CLOSE_NAMESPACE
