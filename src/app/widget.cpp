#include "app/widget.hpp"

#include "app/application.hpp"
#include "common/log.hpp"
#include "common/vector_utils.hpp"

namespace { // anonymous

// https://stackoverflow.com/a/8147213/3444217 and https://stackoverflow.com/a/25069711/3444217
class MakeSharedWidgetEnabler : public signal::Widget {
public:
    template <typename... Args>
    MakeSharedWidgetEnabler(Args&&... args)
        : signal::Widget(std::forward<Args>(args)...)
    {
    }
};

} // namespace anonymous

namespace signal {

Widget::Widget(Handle handle)
    : m_handle{ std::move(handle) }
    , m_framing{ FRAMING::WITHIN }
    , m_parent{}
    , m_components{} // initialize all to empty
    , m_children{}
{
    log_debug << "Created Widget with handle:" << m_handle;
}

Widget::~Widget()
{
    log_debug << "Destroyed Widget with handle:" << m_handle;
}

void Widget::set_parent(std::shared_ptr<Widget> parent)
{
    std::shared_ptr<Widget> this_widget = Application::get_instance().get_widget(m_handle);
    assert(this_widget);

    // remove yourself from your current parent
    if (std::shared_ptr<Widget> current_parent = m_parent.lock()) {
        bool success = remove_one_unordered(current_parent->m_children, this_widget);
        assert(success);
    }

    // register with the new parent
    m_parent = parent;
    parent->m_children.emplace_back(std::move(this_widget));
}

std::shared_ptr<Component> Widget::set_component(std::shared_ptr<Component> component)
{
    std::swap(m_components[static_cast<size_t>(to_number(component->get_kind()))], component);
    return component;
}

std::shared_ptr<Widget> Widget::make_widget(Handle handle)
{
    return std::make_shared<MakeSharedWidgetEnabler>(std::move(handle));
}

} // namespace signal
