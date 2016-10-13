#include "core/layout_item.hpp"

#include "common/log.hpp"
#include "common/vector_utils.hpp"
#include "core/application.hpp"

namespace signal {

LayoutItem::~LayoutItem()
{
}

Handle LayoutItem::get_parent_handle() const
{
    return Application::get_instance().get_parent(m_handle);
}

Transform2 LayoutItem::get_transform(const SPACE space) const
{
    const auto& app = Application::get_instance();

    switch (space) {
    case SPACE::WINDOW:
        if (std::shared_ptr<LayoutItem> parent = app.get_item(get_parent_handle())) {
            return parent->get_transform(SPACE::WINDOW) * m_transform;
        }
        break;

    case SPACE::SCREEN:
        log_critical << "get_transform(SPACE::SCREEN) is not emplemented yet";
        break;

    case SPACE::PARENT:
        break;
    }
    return m_transform;
}

void LayoutItem::set_parent(std::shared_ptr<LayoutItem> parent)
{
    Application::get_instance().set_parent(get_handle(), parent->get_handle());
}

void LayoutItem::remove_child(std::shared_ptr<LayoutItem> child)
{
    // if the internal child is removed, just invalidate the pointer
    if (child == m_internal_child) {
        m_internal_child.reset();
    }

    // external children are properly removed
    else if (!remove_one_unordered(m_external_children, child)) {
        log_critical << "Failed to remove unknown child " << child->get_handle() << " from parent: " << get_handle();
    }
}

} // namespace signal
