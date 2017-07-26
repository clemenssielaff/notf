#include "core/window_layout.hpp"

#include "common/log.hpp"
#include "core/controller.hpp"
#include "core/item_container.hpp"
#include "core/window.hpp"

namespace notf {

WindowLayout::WindowLayout(Window* window)
    : Layout(std::make_unique<detail::SingleItemContainer>())
    , m_controller()
{
    // the WindowLayout is at the root of the Item hierarchy and therefore special
    m_window               = window;
    m_scissor_layout       = this;
    m_has_explicit_scissor = true;
}

std::shared_ptr<WindowLayout> WindowLayout::create(Window* window)
{
    struct make_shared_enabler : public WindowLayout {
        make_shared_enabler(Window* window)
            : WindowLayout(window) {}
    };
    return std::make_shared<make_shared_enabler>(window);
}

std::vector<Widget*> WindowLayout::get_widgets_at(const Vector2f& screen_pos)
{
    std::vector<Widget*> result;
    _get_widgets_at(screen_pos, result);
    return result;
}

void WindowLayout::set_controller(ControllerPtr controller)
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
        log_critical << "Cannot remove unknown child Item " << child_item->get_id()
                     << " from WindowLayout " << get_id();
        return;
    }

    log_trace << "Removing controller from WindowLayout " << get_id();
    m_children->clear();
    m_controller = nullptr;

    on_child_removed(child_item);
}

void WindowLayout::_get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const
{
    if (m_controller) {
        if (ScreenItem* root_item = m_controller->get_root_item()) {
            ScreenItem::_get_widgets_at(root_item, local_pos, result);
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
    _set_aabr(get_grant());

    if (m_controller) {
        if (ScreenItem* root_item = m_controller->get_root_item()) {
            ScreenItem::_set_grant(root_item, get_size());
        }
    }
}

} // namespace notf
