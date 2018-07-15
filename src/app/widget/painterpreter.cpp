#include "app/widget/painterpreter.hpp"

#include "app/widget/widget.hpp"
#include "common/bezier.hpp"
#include "common/vector.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/renderer/plotter.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

Painterpreter::Painterpreter(GraphicsContext& context) : m_plotter(std::make_shared<Plotter>(context)) {}

void Painterpreter::paint(Widget& widget)
{
    { // reset / adopt the Widget's auxiliary information
        m_states.clear();
        m_states.emplace_back(); // always have at least one state

        m_current_path.reset();

        m_base_xform = widget.get_xform<Widget::Space::WINDOW>();
        m_base_clipping = widget.get_clipping_rect();
        m_bounds = Aabrf::wrongest();
    }

    /// Execute the various Painterpreter Commands.
    struct CommandVisitor {

        // fields ----------------------------------------------------------------------------------------------------//

        /// This Painterpreter.
        Painterpreter& painterpreter;

        // methods ---------------------------------------------------------------------------------------------------//

        void operator()(const WidgetDesign::PushStateCommand&) const { painterpreter._push_state(); }

        void operator()(const WidgetDesign::PopStateCommand&) const { painterpreter._pop_state(); }

        void operator()(const WidgetDesign::ResetTransformCommand& command) const {}

        void operator()(const WidgetDesign::TranslationCommand& command) const {}

        void operator()(const WidgetDesign::RotationCommand& command) const {}

        void operator()(const WidgetDesign::SetStrokeWidthCommand& command) const
        {
            painterpreter._get_current_state().stroke_width = command.stroke_width;
        }

        void operator()(const WidgetDesign::SetPolygonPathCommand& command) const
        {
            painterpreter.m_current_path = painterpreter.m_plotter->add(command.data->polygon);
        }

        void operator()(const WidgetDesign::SetSplinePathCommand& command) const
        {
            painterpreter.m_current_path = painterpreter.m_plotter->add(command.data->spline);
        }

        void operator()(const WidgetDesign::WriteCommand& command) const
        {
            //
        }

        void operator()(const WidgetDesign::FillCommand&) const { painterpreter._fill(); }

        void operator()(const WidgetDesign::StrokeCommand&) const { painterpreter._stroke(); }
    };

    { // parse the commands
        const WidgetDesign& design = Widget::Access<Painterpreter>::get_design(widget);
        const std::vector<WidgetDesign::Command>& buffer = WidgetDesign::Access<Painterpreter>::get_buffer(design);
        for (const WidgetDesign::Command& command : buffer) {
            notf::visit(CommandVisitor{*this}, command);
        }
    }

    // plot it
    m_plotter->swap_buffers();
    m_plotter->render();
}

void Painterpreter::_push_state() { m_states.emplace_back(m_states.back()); }

void Painterpreter::_pop_state()
{
    assert(m_states.size() > 1);
    m_states.pop_back();
}

void Painterpreter::_fill()
{
    const State& state = _get_current_state();
    if (!m_current_path || is_approx(state.alpha, 0)) {
        return; // early out
    }

    // get the fill paint
    Paint paint = state.stroke_paint;
    paint.inner_color.a *= state.alpha;
    paint.outer_color.a *= state.alpha;

    // plot the shape
    Plotter::FillInfo fill_info;
    m_plotter->fill(m_current_path, std::move(fill_info));
}

void Painterpreter::_stroke()
{
    const State& state = _get_current_state();
    if (!m_current_path || state.stroke_width <= 0 || is_approx(state.alpha, 0)) {
        return; // early out
    }

    // get the stroke paint
    Paint paint = state.stroke_paint;
    paint.inner_color.a *= state.alpha;
    paint.outer_color.a *= state.alpha;

    // create a sane stroke width
    float stroke_width;
    {
        const float scale = (state.xform.scale_x() + state.xform.scale_y()) / 2;
        stroke_width = max(state.stroke_width * scale, 0);
        if (stroke_width < 1) {
            // if the stroke width is less than pixel size, use alpha to emulate coverage.
            const float alpha = clamp(stroke_width, 0.0f, 1.0f);
            paint.inner_color.a *= alpha * alpha; // since coverage is area, scale by alpha*alpha
            paint.outer_color.a *= alpha * alpha;
            stroke_width = 1;
        }
    }

    // plot the stroke
    Plotter::StrokeInfo stroke_info;
    stroke_info.width = stroke_width;
    m_plotter->stroke(m_current_path, std::move(stroke_info));
}

NOTF_CLOSE_NAMESPACE
