#include "notf/app/widget/widget_scene.hpp"

#include "notf/meta/log.hpp"

#include "notf/app/graph/window.hpp"

NOTF_USING_NAMESPACE;

// widget scene ===================================================================================================== //

WidgetScene::WidgetScene(valid_ptr<AnyNode*> parent) : Scene(parent) {

    Window* window = _get_first_ancestor<Window>(); // TODO: hack (see comment below `set_visualizer`)
    NOTF_ASSERT(window);
    set_visualizer(std::make_unique<WidgetVisualizer>(*window));

    // update the clipping rect whenever the Scene area changes
    _set_property_callback<area>([this](Aabri& aabr) {
        m_clipping = Aabrf(aabr);

        if (m_root_widget) { WidgetHandle::AccessFor<WidgetScene>::set_grant(m_root_widget, m_clipping.get_size()); }

        return true;
    });
}
