#pragma once

#include <memory>
#include <vector>

#include "common/enummap.hpp"
#include "core/component.hpp"
#include "core/layout_item.hpp"

namespace notf {

class Window;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief The Claim of a Widget determines how much space it will receive in its parent Layout.
 *
 * Widget Claims are not changed by the Layout, only by the Widget or User.
 * If the parent Layout cannot accompany the Items minimal size, then it must simply overflow the parent Layout.
 * Also has a min and max ratio betweeen horizontal and vertical.
 * For example, a circular Item may have a size range from 1 - 10 both vertically and horizontally, but should only
 * expand in the ration 1:1, to stay circular.
 * Also, scale factors, both for vertical and horizontal expansion.
 * Linear factors work great if you want all to expand at the same time, but not if you want some to expand before
 * others.
 * For that, you also need a priority system, where widgets in priority two are expanded before priority one.
 * Reversly, widget in priority -1 are shrunk before priority 0 etc.
 */
class Claim {

    /// @brief A Claim has two Directions: horizontal and vertical.
    /// Both need to enforce constraints but both Directions are largely independent.
    class Direction {

    public: // methods
        /// @brief Default Constructor.
        explicit Direction() = default;

        /// @brief Value Constructor.
        /// @param preferred    Preferred size in local units, is limited to values >= 0.
        /// @param min          (optional) Minimum size, is clamped to 0 <= value <= preferred, defaults to 'preferred'.
        /// @param max          (optional) Maximum size, is clamped to preferred <= value, can be INFINITY, defaults to 'preferred'.
        Direction(const Real preferred, const Real min = NAN, const Real max = NAN)
            : m_preferred(is_valid(preferred) ? notf::max(preferred, Real(0)) : 0)
            , m_min(is_valid(min) ? notf::min(std::max(Real(0), min), m_preferred) : m_preferred)
            , m_max(is_valid(preferred) ? (is_nan(max) ? m_preferred : notf::max(max, m_preferred)) : 0)
            , m_scale_factor(1)
            , m_priority(0)
        {
        }

        /// @brief Preferred size in local units, is >= 0.
        Real get_preferred() const { return m_preferred; }

        /// @brief Minimum size in local units, is 0 <= min <= preferred.
        Real get_min() const { return m_min; }

        /// @brief Maximum size in local units, is >= preferred.
        Real get_max() const { return m_max; }

        /// @brief Tests if this Stretch is a fixed size where all 3 values are the same.
        bool is_fixed() const { return approx(m_preferred, m_min) && approx(m_preferred, m_max); }

        /// @brief Returns the scale factor of the LayoutItem in this direction.
        Real get_scale_factor() const { return m_scale_factor; }

        /// @brief Returns the scale priority of the LayoutItem in this direction.
        int get_priority() const { return m_priority; }

        /// @brief Sets a new minimal size, accomodates both the preferred and max size if necessary.
        /// @param min  Minimal size, must be 0 <= size < INFINITY.
        void set_min(const Real min);

        /// @brief Sets a new minimal size, accomodates both the min and preferred size if necessary.
        /// @param max  Maximal size, must be 0 <= size <= INFINITY.
        void set_max(const Real max);

        /// @brief Sets a new preferred size, accomodates both the min and max size if necessary.
        /// @param preferred    Preferred size, must be 0 <= size < INFINITY.
        void set_preferred(const Real preferred);

        /// @brief Sets a new scale factor.
        /// @param preferred    Preferred size, must be 0 <= size < INFINITY.
        void set_scale_factor(const Real factor);

        /// @brief Sets a new scaling priority.
        /// @param priority Scaling priority.
        void set_priority(const int priority) { m_priority = priority; }

    private: // fields
        /// @brief Preferred size, is: min <= size <= max.
        Real m_preferred = 0;

        /// @brief Minimal size, is: 0 <= size <= preferred.
        Real m_min = 0;

        /// @brief Maximal size, is: preferred <= size <= INFINITY.
        Real m_max = INFINITY;

        /// @brief Scale factor, 0 means no scaling, is: 0 <= factor < INFINITY
        Real m_scale_factor = 1.;

        /// @brief Scaling priority, is INT_MIN <= priority <= INT_MAX.
        int m_priority = 0;
    };

public: // methods
    /// @brief Default Constructor.
    explicit Claim() = default;

    /// @brief Returns the horizontal part of this Claim.
    Direction& get_horizontal() { return m_horizontal; }

    /// @brief Returns the vertical part of this Claim.
    Direction& get_vertical() { return m_vertical; }

    /// @brief Returns the minimum and maximum ratio scaling constraint
    const std::pair<Real, Real>& get_height_for_width() const { return m_height_for_width; }

    /// @brief Sets the ratio constraint.
    /// @param ratio_min    Height for width (min/fixed value), is used as minimum value if the second parameter is set.
    /// @param ratio_max    Height for width (max value), 'ratio_min' is use by default.
    void set_height_for_width(const Real ratio_min, const Real ratio_max = NAN);

private: // members
    /// @brief The vertical part of this Claim.
    Direction m_horizontal;

    /// @brief The horizontal part of this Claim.
    Direction m_vertical;

    /// @brief Minimum and maximum ratio scaling constraint, 0 means no constraint, is: 0 <= min <= max < INFINITY.
    std::pair<Real, Real> m_height_for_width;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Something drawn on the screen, potentially able to interact with the user.
class Widget : public LayoutItem {

public: // methods
    /// @brief Returns the Window containing this Widget (can be nullptr).
    std::shared_ptr<Window> get_window() const;

    /// @brief Checks if this Widget contains a Component of the given kind.
    /// @param kind Component kind to check for.
    bool has_component_kind(Component::KIND kind) const { return m_components.count(kind); }

    /// @brief Requests the Component of a given kind from this Widget.
    /// @return The requested Component, shared pointer is empty if this Widget has no Component of the requested kind.
    template <typename COMPONENT>
    std::shared_ptr<COMPONENT> get_component() const
    {
        auto it = m_components.find(get_kind<COMPONENT>());
        if (it == m_components.end()) {
            return {};
        }
        return std::static_pointer_cast<COMPONENT>(it->second);
    }

    /// @brief Attaches a new Component to this Widget.
    /// @param component    The Component to attach.
    ///
    /// Each Widget can only have one instance of each Component kind attached.
    void add_component(std::shared_ptr<Component> component);

    /// @brief Removes an existing Component of the given kind from this Widget.
    /// @param kind Kind of the Component to remove.
    ///
    /// If the Widget doesn't have the given Component kind, the call is ignored.
    void remove_component(Component::KIND kind);

    virtual void redraw() override;

    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) override;

protected: // methods
    /// @brief Value Constructor.
    /// @param handle   Handle of this Widget.
    explicit Widget(const Handle handle)
        : LayoutItem(handle)
        , m_components()
    {
    }

public: // static methods
    /// @brief Factory function to create a new Widget instance.
    /// @param handle   [optional] Handle of the new widget.
    /// @return The created Widget, pointer is empty on error.
    ///
    /// If an explicit handle is passed, it is assigned to the new Widget.
    /// This function will fail if the existing Handle is already taken.
    /// If no handle is passed, a new one is created.
    static std::shared_ptr<Widget> create(Handle handle = BAD_HANDLE)
    {
        return create_item<Widget>(handle);
    }

private: // fields
    /// @brief The Claim of a Widget determines how much space it receives in the parent Layout.
    Claim m_claim;

    /// @brief All components of this Widget.
    EnumMap<Component::KIND, std::shared_ptr<Component>> m_components;
};

} // namespace notf
