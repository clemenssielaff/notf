#pragma once

#include "app/widget/layout.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// The RootLayout is owned by a Window and root of all LayoutItems displayed within the Window.
class RootLayout : public Layout {
    friend class Item;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    NOTF_ALLOW_ACCESS_TYPES(ItemHierarchy);

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// @param token    Factory token provided by Item::_create.
    RootLayout(const Token& token);

    /// Factory.
    static RootLayoutPtr create() { return _create<RootLayout>(); }

public:
    /// Destructor.
    virtual ~RootLayout() override {}

    /// Find all Widgets at a given position in the Window.
    /// @param local_pos     Local coordinates where to look for a Widget.
    /// @return              All Widgets at the given coordinate, ordered from front to back.
    void widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const override;

    /// Sets a new Controller for the RootLayout.
    void set_controller(const ControllerPtr& controller);

private:
    virtual void _remove_child(const Item* child_item) override;

    virtual Claim _consolidate_claim() override;

    virtual void _relayout() override;

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The Window Controller.
    Controller* m_controller;
};

// ===================================================================================================================//

template<>
class RootLayout::Access<ItemHierarchy> {
    friend class ItemHierarchy;

    /// Constructor.
    /// @param root_layout    RootLayout to access.
    Access(RootLayout& root_layout) : m_root_layout(root_layout) {}

    /// Factory.
    static RootLayoutPtr create() { return RootLayout::create(); }

    /// Updates the Grant of this Item and might cause a relayout.
    /// @return      True iff the Grant was modified.
    bool set_grant(Size2f grant) { return m_root_layout._set_grant(std::move(grant)); }

    /// The RootLayout to access.
    RootLayout& m_root_layout;
};

NOTF_CLOSE_NAMESPACE
