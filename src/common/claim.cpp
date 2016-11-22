#include "common/claim.hpp"

#include "common/log.hpp"
#include "common/string_utils.hpp"

namespace { // anonymous

const float MIN_SCALE_FACTOR = 0.00001f;

} // namespace anonymous

namespace notf {

void Claim::Direction::set_preferred(const float preferred)
{
    if (!is_real(preferred) || preferred < 0) {
        log_warning << "Invalid preferred Claim value: " << preferred << " - using 0 instead.";
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

void Claim::Direction::set_min(const float min)
{
    if (!is_real(min) || min < 0) {
        log_warning << "Invalid minimum Claim value: " << min << " - using 0 instead.";
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
        log_warning << "Invalid maximum Claim value: " << max << " - using 0 instead.";
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

void Claim::Direction::set_scale_factor(const float factor)
{
    if (!is_real(factor) || factor <= 0) {
        log_warning << "Invalid scale factor: " << factor << " - using " << MIN_SCALE_FACTOR << " instead.";
        m_scale_factor = MIN_SCALE_FACTOR;
    }
    else {
        m_scale_factor = factor;
    }
}

void Claim::Direction::add_offset(const float offset)
{
    if (!is_real(offset)) {
        log_warning << "Ignored invalid offset value: " << offset;
        return;
    }
    m_min = max(0.f, m_min + offset);
    m_max = max(0.f, m_max + offset);
    m_preferred = max(0.f, m_preferred + offset);
}

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

std::ostream& operator<<(std::ostream& out, const Claim::Direction& direction)
{
    return out << string_format(
               "Claim::Direction([%f <= %f <=%f, factor: %f, priority %i])",
               direction.get_min(),
               direction.get_preferred(),
               direction.get_max(),
               direction.get_scale_factor(),
               direction.get_priority());
}

std::ostream& operator<<(std::ostream& out, const Claim& claim)
{
    const Claim::Direction& horizontal = claim.get_horizontal();
    const Claim::Direction& vertical = claim.get_horizontal();
    const std::pair<float, float> ratio = claim.get_width_to_height();
    return out << string_format(
               "Claim(\n"
               "\thorizontal: [%f <= %f <=%f, factor: %f, priority %i]\n"
               "\tvertical: [%f <= %f <=%f, factor: %f, priority %i]\n"
               "\tratio: %f : %f)",
               horizontal.get_min(),
               horizontal.get_preferred(),
               horizontal.get_max(),
               horizontal.get_scale_factor(),
               horizontal.get_priority(),
               vertical.get_min(),
               vertical.get_preferred(),
               vertical.get_max(),
               vertical.get_scale_factor(),
               vertical.get_priority(),
               ratio.first,
               ratio.second);
}

} // namespace notf
