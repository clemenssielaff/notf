#include "notf/app/widget/widget_claim.hpp"

#include "notf/meta/log.hpp"

NOTF_OPEN_NAMESPACE

// claim ============================================================================================================ //

void WidgetClaim::set_ratio_limits(Ratioi ratio_min, Ratioi ratio_max) {
    if (ratio_min.is_zero()) {
        if (!ratio_max.is_zero()) {
            NOTF_LOG_WARN("Ignoring ratio_max value, since the ratio_min constraint is set to zero.");
        }
        m_ratios = {};
        return;
    }

    if (ratio_max < ratio_min) { std::swap(ratio_max, ratio_min); }

    if (ratio_max.is_zero()) {
        m_ratios = Ratios{ratio_min, std::move(ratio_min)};
    } else {
        m_ratios = Ratios{std::move(ratio_min), std::move(ratio_max)};
    }
}

Size2f WidgetClaim::apply(Size2f size) const {
    // clamp to size first
    size.width() = clamp(size.width(), m_horizontal.get_min(), m_horizontal.get_max());
    size.height() = clamp(size.height(), m_vertical.get_min(), m_vertical.get_max());

    // apply ratio constraints by shrinking one side within the valid range
    if (!is_zero(size.get_area()) && !m_ratios.get_lower_bound().is_zero()) {
        NOTF_ASSERT(!m_ratios.get_upper_bound().is_zero());
        const float current_ratio = size.width() / size.height();
        const float valid_ratio
            = clamp(current_ratio, m_ratios.get_lower_bound().as_real(), m_ratios.get_upper_bound().as_real());
        if (valid_ratio < current_ratio) {
            size.height() = clamp(size.width() / valid_ratio, m_vertical.get_min(), m_vertical.get_max());
        } else if (valid_ratio > current_ratio) {
            size.width() = clamp(size.height() * valid_ratio, m_horizontal.get_min(), m_horizontal.get_max());
        }
    }

    return size;
}

NOTF_CLOSE_NAMESPACE
