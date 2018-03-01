#include "app/core/window_layout.hpp"

#include "app/core/controller.hpp"
#include "app/core/window.hpp"
#include "common/log.hpp"
#include "utils/make_smart_enabler.hpp"

namespace notf {

WindowLayout::WindowLayout(Window* window) : Layout(std::make_unique<detail::SingleItemContainer>()), m_controller()
{
    // the WindowLayout is at the root of the Item hierarchy and therefore special
    Item::Private<WindowLayout>(*this).set_window(window);
    ScreenItem::Private<WindowLayout>(*this).be_own_scissor(this);
}

WindowLayoutPtr WindowLayout::create(Window* window)
{
#ifdef NOTF_DEBUG
    return WindowLayoutPtr(new WindowLayout(window));
#else
    return std::make_shared<make_shared_enabler<WindowLayout>>(window);
#endif
}

std::vector<Widget*> WindowLayout::widgets_at(const Vector2f& screen_pos)
{
    std::vector<Widget*> result;
    _widgets_at(screen_pos, result);
    return result;
}

void WindowLayout::set_controller(const ControllerPtr& controller)
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

void WindowLayout::_remove_child(const Item* child_item)
{
    if (!child_item) {
        return;
    }

    if (child_item != m_controller) {
        log_critical << "Cannot remove unknown child Item " << child_item->name() << " from WindowLayout " << name();
        return;
    }

    log_trace << "Removing controller from WindowLayout " << name();
    m_children->clear();
    m_controller = nullptr;

    on_child_removed(child_item);
}

void WindowLayout::_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const
{
    if (m_controller) {
        if (ScreenItem* root_item = m_controller->root_item()) {
            ScreenItem::_widgets_at(root_item, local_pos, result);
        }
    }
}

Claim WindowLayout::_consolidate_claim()
{
    assert(0);
    return {};
}

void WindowLayout::_relayout()
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

} // namespace notf
