#include "app/widget/widget_visualizer.hpp"

#include "app/widget/painterpreter.hpp"
#include "app/widget/widget.hpp"
#include "app/widget/widget_scene.hpp"
#include "app/window.hpp"
#include "common/log.hpp"

namespace {
NOTF_USING_NAMESPACE;

class WidgetIterator {

    // types -------------------------------------------------------------------------------------------------------- //
private:
    struct Iterator {

        // methods -------------------------------------------------------------------------------------------------- //
    public:
        /// Constructor.
        /// @param widget       Widget to iterate.
        /// @param child_count  Number of children in the Widget.
        Iterator(NodeHandle<Widget>&& widget, size_t child_count) : widget(std::move(widget)), end(child_count) {}

        // fields --------------------------------------------------------------------------------------------------- //
    public:
        /// The iterated widget.
        NodeHandle<Widget> widget;

        /// Current child index.
        size_t index = 0;

        /// One index past the last child.
        const size_t end;
    };

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param widget   Root widget of the iteration.
    WidgetIterator(NodeHandle<Widget>&& widget)
    {
        size_t child_count = widget->count_children();
        m_iterators.emplace_back(std::move(widget), std::move(child_count));
    }

    /// Finds and returns the next Widget in the iteration.
    /// @param  widget  [OUT] Next Widget in the iteration.
    /// @returns        True if a new widget was found.
    bool next(NodeHandle<Widget>& widget)
    {
        while (true) {
            Iterator& it = m_iterators.back();
            widget = it.widget;

            if (it.index == it.end) {
                if (m_iterators.size() == 1) {
                    return false;
                }
                else {
                    m_iterators.pop_back();
                    continue;
                }
            }

            size_t child_count = widget->count_children();
            if (child_count > 0) {
                m_iterators.emplace_back(widget->get_child<Widget>(0), std::move(child_count));
            }

            return true;
        }
    }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Stack of Iterators.
    std::vector<Iterator> m_iterators;
};

} // namespace

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

WidgetVisualizer::WidgetVisualizer(GraphicsContext& context)
    : Visualizer(), m_painterpreter(std::make_unique<Painterpreter>(context))
{}

WidgetVisualizer::~WidgetVisualizer() = default;

void WidgetVisualizer::visualize(valid_ptr<Scene*> scene) const
{
    WidgetScene* widget_scene = dynamic_cast<WidgetScene*>(raw_pointer(scene));
    if (!widget_scene) {
        log_critical << "Failed to visualize Scene \"" << scene->get_name()
                     << "\": WidgetVisualizer can only visualize WidgetScenes";
        return;
    }

    WidgetIterator iterator(widget_scene->get_widget<Widget>());
    NodeHandle<Widget> widget;
    while (true) {
        bool has_more = iterator.next(widget);
        m_painterpreter->paint(*widget);
        if (!has_more) {
            break;
        }
    }
}

NOTF_CLOSE_NAMESPACE
