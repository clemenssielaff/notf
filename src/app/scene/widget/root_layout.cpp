#include "app/scene/widget/root_layout.hpp"

#include "app/scene/widget/controller.hpp"
#include "common/log.hpp"
#include "utils/make_smart_enabler.hpp"

NOTF_OPEN_NAMESPACE

RootLayout::RootLayout() : Layout(std::make_unique<detail::SingleItemContainer>()), m_controller()
{
    ScreenItem::Access<RootLayout>(*this).be_own_scissor(this);
}

RootLayoutPtr RootLayout::create()
{
#ifdef NOTF_DEBUG
    return RootLayoutPtr(new RootLayout());
#else
    return std::make_shared<make_shared_enabler<RootLayout>>();
#endif
}

void RootLayout::set_controller(const ControllerPtr& controller)
{
    if (!controller) {
        log_warning << "Cannot add an empty Controller pointer to a Layout";
        return;
    }

    if (m_controller) {
        if (m_controller == controller.get()) {
            return;
        }
        _remove_child(m_controller);
    }

    detail::SingleItemContainer* container = static_cast<detail::SingleItemContainer*>(m_children.get());
    container->item                        = controller;
    m_controller                           = controller.get();
    if (m_controller) {
        Item::_set_parent(m_controller, this);
    }

    _relayout();
    on_child_added(m_controller);
}

void RootLayout::_remove_child(const Item* child_item)
{
    if (!child_item) {
        return;
    }

    if (child_item != m_controller) {
        log_critical << "Cannot remove unknown child Item " << child_item->name() << " from RootLayout " << name();
        return;
    }

    log_trace << "Removing controller from RootLayout " << name();
    m_children->clear();
    m_controller = nullptr;

    on_child_removed(child_item);
}

void RootLayout::widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const
{
    if (m_controller) {
        if (ScreenItem* root_item = m_controller->root_item()) {
            root_item->widgets_at(local_pos, result);
        }
    }
}

Claim RootLayout::_consolidate_claim()
{
    assert(0);
    return {};
}

void RootLayout::_relayout()
{
    _set_size(grant());
    _set_content_aabr(Aabrf::zero());

    if (m_controller) {
        if (ScreenItem* root_item = m_controller->root_item()) {
            ScreenItem::_set_grant(root_item, size());
            _set_content_aabr(root_item->content_aabr());
        }
    }
}

NOTF_CLOSE_NAMESPACE
