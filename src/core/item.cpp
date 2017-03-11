#include "core/item.hpp"

#include <atomic>

#include "common/log.hpp"
#include "core/layout.hpp"
#include "core/layout_root.hpp"
#include "core/render_manager.hpp"
#include "core/window.hpp"

namespace { // anonymous
using namespace notf;

/** The next available Item ID, is ever-increasing. */
std::atomic<ItemID> g_nextID(1);

/** Returns the next, free ItemID.
 * Is thread-safe.
 */
ItemID get_next_id()
{
    return g_nextID++;
}

} // namespace anonymous

namespace notf {

Item::Item()
    : m_id(get_next_id())
    , m_parent()
    , m_render_layer() // empty by default
    , m_opacity(1)
    , m_size()
    , m_transform(Xform2f::identity())
    , m_claim()
#ifdef NOTF_PYTHON
    , m_py_object(nullptr, py_decref)
#endif
{
    log_trace << "Created Item #" << m_id;
}

Item::~Item()
{
    log_trace << "Destroying Item #" << m_id;
}

bool Item::has_ancestor(const Item* ancestor) const
{
    // invalid Item can never be an ancestor
    if (!ancestor) {
        return false;
    }

    // check all actual ancestors against the candidate
    const Item* my_parent = parent().get();
    while (my_parent) {
        if (my_parent == ancestor) {
            return true;
        }
        my_parent = my_parent->parent().get();
    }

    // if no ancestor matches, we have our answer
    return false;
}

std::shared_ptr<Window> Item::window() const
{
    std::shared_ptr<const Item> it = shared_from_this();
    while (it->has_parent()) {
        it = it->parent();
    }
    std::shared_ptr<const LayoutRoot> root_item = std::dynamic_pointer_cast<const LayoutRoot>(it);
    if (!root_item) {
        return {};
    }
    return root_item->window();
}

Xform2f Item::window_transform() const
{
    Xform2f result = Xform2f::identity();
    _window_transform(result);
    return result;
}

bool Item::set_opacity(float opacity)
{
    opacity = clamp(opacity, 0, 1);
    if (abs(m_opacity - opacity) <= precision_high<float>()) {
        return false;
    }
    m_opacity = opacity;
    opacity_changed(m_opacity);
    _redraw();
    return true;
}

void Item::set_render_layer(std::shared_ptr<RenderLayer> render_layer)
{
    if (m_parent.expired()) {
        log_critical << "Cannot change the RenderLayer of an Item outside the Item hierarchy.";
        return;
    }
    m_render_layer = std::move(render_layer);
}

void Item::_update_parent_layout()
{
    std::shared_ptr<Layout> parentLayout = _first_ancestor<Layout>();
    while (parentLayout) {
        // if the parent Layout's Claim changed, we also need to update its parent ...
        if (parentLayout->_update_claim()) {
            parentLayout = parentLayout->_first_ancestor<Layout>();
        }
        // ... otherwise, we have reached the end of the propagation through the ancestry
        // and continue to relayout all children from the parent downwards
        else {
            parentLayout->_relayout();
            return;
        }
    }
}

void Item::_redraw()
{
    // do not request a redraw, if this item cannot be drawn anyway
    if (!is_visible()) {
        return;
    }

    if (std::shared_ptr<Window> my_window = window()) {
        my_window->get_render_manager().request_redraw();
    }
}

bool Item::_set_size(const Size2f& size)
{
    if (size != m_size) {
        const Claim::Stretch& horizontal = m_claim.get_horizontal();
        const Claim::Stretch& vertical   = m_claim.get_vertical();
        // TODO: enforce width-to-height constraint in Item::_set_size (the StackLayout does something like it already)

        m_size.width  = max(horizontal.get_min(), min(horizontal.get_max(), size.width));
        m_size.height = max(vertical.get_min(), min(vertical.get_max(), size.height));
        size_changed(m_size);
        _redraw();
        return true;
    }
    return false;
}

bool Item::_set_transform(const Xform2f transform)
{
    if (transform == m_transform) {
        return false;
    }
    m_transform = std::move(transform);
    transform_changed(m_transform);
    _redraw();
    return true;
}

bool Item::_set_claim(const Claim claim)
{
    if (claim == m_claim) {
        return false;
    }
    m_claim = std::move(claim);
    return true;
}

#ifdef NOTF_PYTHON
void Item::_set_pyobject(PyObject* object)
{
    assert(!m_py_object); // you should only have to do this once
    py_incref(object);
    m_py_object.reset(std::move(object));
}
#endif

void Item::_set_parent(std::shared_ptr<Item> parent)
{
    // do nothing if the new parent is the same as the old (or both are invalid)
    std::shared_ptr<Item> old_parent = m_parent.lock();
    if (parent == old_parent) {
        return;
    }

    // check for cyclic ancestry
    if (has_ancestor(parent.get())) {
        log_critical << "Cannot make " << parent->id() << " the parent of " << id()
                     << " because " << parent->id() << " is already a child of " << id();
        return;
    }

    // remove yourself from the old parent Layout
    if (std::shared_ptr<Layout> old_layout = std::dynamic_pointer_cast<Layout>(old_parent)) {
        old_layout->_remove_child(this);
    }

    m_parent = parent;
    parent_changed(parent->id());
}

void Item::_window_transform(Xform2f& result) const
{
    std::shared_ptr<const Item> parent_item = parent();
    if (!parent_item) {
        return;
    }
    parent_item->_window_transform(result);
    result = m_transform * result;
}

} // namespace notf
