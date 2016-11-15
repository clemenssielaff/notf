#include "common/claim.hpp"

#include "common/log.hpp"

namespace notf {

void Claim::Direction::set_min(const float min)
{
    if (!is_valid(min) || min < 0) {
        log_warning << "Invalid minimum Stretch value: " << min << " - using 0 instead.";
        m_min = 0;
    }
    else {
        m_min = min;
    }
    if (m_min > m_preferred) {
        m_preferred = m_min;
        if (m_min > m_max) {
            m_max = m_min;
        }
    }
}

void Claim::Direction::set_max(const float max)
{
    if (is_nan(max) || max < 0) {
        log_warning << "Invalid maximum Stretch value: " << max << " - using 0 instead.";
        m_max = 0;
    }
    else {
        m_max = max;
    }
    if (m_max < m_preferred) {
        m_preferred = m_max;
        if (m_max < m_min) {
            m_min = m_max;
        }
    }
}

void Claim::Direction::add_offset(const float offset)
{
    if (!is_valid(offset)) {
        log_warning << "Ignored invalid offset value: " << offset;
        return;
    }
    m_min = max(0.f, m_min + offset);
    m_max = max(0.f, m_max + offset);
    m_preferred = max(0.f, m_preferred + offset);
}

void Claim::Direction::set_preferred(const float preferred)
{
    if (!is_valid(preferred) || preferred < 0) {
        log_warning << "Invalid preferred Stretch value: " << preferred << " - using 0 instead.";
        m_preferred = 0;
    }
    else {
        m_preferred = preferred;
    }
    if (m_preferred < m_min) {
        m_min = m_preferred;
    }
    if (m_preferred > m_max) {
        m_max = m_preferred;
    }
}

void Claim::Direction::set_scale_factor(const float factor)
{
    if (!is_valid(factor) || factor < 0) {
        log_warning << "Invalid Stretch scale factor: " << factor << " - using 0 instead.";
        m_scale_factor = 0;
    }
    else {
        m_scale_factor = factor;
    }
}

void Claim::set_height_for_width(const float ratio_min, const float ratio_max)
{
    if (!is_valid(ratio_min) || ratio_min < 0) {
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
        log_warning << "Ignoring ratio_max value " << ratio_max
                    << ", since it is smaller than the min_ratio " << ratio_min;
        m_ratios = std::make_pair(min_ratio, min_ratio);
        return;
    }

    if (approx(ratio_min, 0)) {
        log_warning << "Ignoring ratio_max value, since the min ratio constraint is set to 0.";
        m_ratios = std::make_pair(min_ratio, min_ratio);
        return;
    }

    m_ratios = std::make_pair(min_ratio, Ratio(ratio_max));
}

} // namespace notf
