#include "core/layout_root.hpp"

#include <assert.h>

#include "common/log.hpp"
#include "common/transform2.hpp"
#include "core/layout_item.hpp"
#include "core/render_manager.hpp"
#include "core/window.hpp"

namespace notf {

const LayoutItem* LayoutRootIterator::next()
{
    if (m_layout) {
        const LayoutItem* result = m_layout->_get_item();
        m_layout = nullptr;
        return result;
    }
    return nullptr;
}

std::shared_ptr<Window> LayoutRoot::get_window() const
{
    return m_window->shared_from_this();
}

void LayoutRoot::set_item(std::shared_ptr<LayoutItem> item)
{
    // remove the existing item first
    if (!is_empty()) {
        const auto& children = _get_children();
        assert(children.size() == 1);
        _remove_child(children.begin()->get());
    }
    _add_child(std::move(item));
    _relayout({});
}

std::shared_ptr<Widget> LayoutRoot::get_widget_at(const Vector2& local_pos)
{
    if (is_empty()) {
        return {};
    }
    return _get_item()->get_widget_at(local_pos);
}

void LayoutRoot::set_render_layer(std::shared_ptr<RenderLayer>)
{
    log_critical << "Cannot change the RenderLayer of the LayoutRoot.";
}

std::unique_ptr<LayoutIterator> LayoutRoot::iter_items() const
{
    return std::make_unique<MakeSmartEnabler<LayoutRootIterator>>(this);
}

LayoutRoot::LayoutRoot(Handle handle, const std::shared_ptr<Window>& window)
    : Layout(handle)
    , m_window(window.get())
{
    // the layout_root is always in the default render layer
    Layout::set_render_layer(window->get_render_manager().get_default_layer());
}

void LayoutRoot::_relayout(const Size2f /*size*/)
{
    Size2i canvas_size = m_window->get_buffer_size();
    _set_size({static_cast<float>(canvas_size.width), static_cast<float>(canvas_size.height)});
    if (!is_empty()) {
        _update_item(_get_item(), get_size(), Transform2::identity());
    }
}

LayoutItem* LayoutRoot::_get_item() const
{
    if (is_empty()) {
        return nullptr;
    }
    const auto& children = _get_children();
    assert(children.size() == 1);
    return children.begin()->get();
}

} // namespace notf
