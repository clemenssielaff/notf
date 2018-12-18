#include "notf/app/widget/painterpreter.hpp"

#include "notf/common/bezier.hpp"
#include "notf/common/variant.hpp"
#include "notf/common/vector.hpp"

#include "notf/graphic/graphics_context.hpp"
#include "notf/graphic/renderer/plotter.hpp"

#include "notf/app/widget/any_widget.hpp"

NOTF_OPEN_NAMESPACE

// painterpreter ==================================================================================================== //

Painterpreter::Painterpreter(GraphicsContext& context) : m_plotter(std::make_shared<Plotter>(context)) { _reset(); }

void Painterpreter::paint(AnyWidget& widget) {
//    { // adopt the Widget's auxiliary information
//        m_base_xform = widget.get_xform<Widget::Space::WINDOW>();
//        m_base_clipping = widget.get_clipping_rect();
//    }

//    { // parse the commands
//        const WidgetDesign& design = Widget::Access<Painterpreter>::get_design(widget);
//        const std::vector<WidgetDesign::Command>& buffer = WidgetDesign::Access<Painterpreter>::get_buffer(design);
//        for (const WidgetDesign::Command& command : buffer) {
//            std::visit(
//                overloaded{
//                    [&](const WidgetDesign::PushStateCommand&) { _push_state(); },

//                    [&](const WidgetDesign::PopStateCommand&) { _pop_state(); },

//                    [&](const WidgetDesign::SetTransformationCommand& command) {
//                        State& state = _get_current_state();
//                        state.xform = command.data->transformation;
//                    },

//                    [&](const WidgetDesign::SetStrokeWidthCommand& command) {
//                        State& state = _get_current_state();
//                        state.stroke_width = command.stroke_width;
//                    },

//                    [&](const WidgetDesign::SetFontCommand& command) {
//                        State& state = _get_current_state();
//                        state.font = command.data->font;
//                    },

//                    [&](const WidgetDesign::SetPolygonPathCommand& command) {
//                        State& state = _get_current_state();
//                        state.path = m_plotter->add(command.data->polygon);
//                        m_paths.emplace_back(state.path);
//                    },

//                    [&](const WidgetDesign::SetSplinePathCommand& command) {
//                        State& state = _get_current_state();
//                        state.path = m_plotter->add(command.data->spline);
//                        m_paths.emplace_back(state.path);
//                    },

//                    [&](const WidgetDesign::SetPathIndexCommand& command) {
//                        NOTF_ASSERT(command.index);
//                        const size_t path_index = command.index.value() - Plotter::PathId::first().value();
//                        NOTF_ASSERT(path_index < m_paths.size());
//                        State& state = _get_current_state();
//                        state.path = m_paths[path_index];
//                    },

//                    [&](const WidgetDesign::WriteCommand& command) { _write(command.data->text); },

//                    [&](const WidgetDesign::FillCommand&) { _fill(); },

//                    [&](const WidgetDesign::StrokeCommand&) { _stroke(); },
//                },
//                command);
//        }
//    }

//    // plot it
//    m_plotter->swap_buffers();
//    m_plotter->render();
//    _reset();
}

void Painterpreter::_reset() {
    m_states.clear();
    m_states.emplace_back(); // always have at least one state

    m_paths.clear();

    m_base_xform = M3f::identity();
    m_base_clipping = {};
    m_bounds = Aabrf::wrongest();
}

Painterpreter::State& Painterpreter::_get_current_state() {
    assert(!m_states.empty());
    return m_states.back();
}

void Painterpreter::_push_state() { m_states.emplace_back(m_states.back()); }

void Painterpreter::_pop_state() {
    assert(m_states.size() > 1);
    m_states.pop_back();
}

void Painterpreter::_fill() {
    const State& state = _get_current_state();
    if (!state.path || is_approx(state.alpha, 0)) {
        return; // early out
    }

    // get the fill paint
    Paint paint = state.stroke_paint;
    paint.inner_color.a *= state.alpha;
    paint.outer_color.a *= state.alpha;

    // plot the shape
    Plotter::FillInfo fill_info;
    m_plotter->fill(state.path, std::move(fill_info));
}

void Painterpreter::_stroke() {
    const State& state = _get_current_state();
    if (!state.path || state.stroke_width <= 0 || is_approx(state.alpha, 0)) {
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
    m_plotter->stroke(state.path, std::move(stroke_info));
}

void Painterpreter::_write(const std::string& text) {
    const State& state = _get_current_state();
    if (!state.font || is_approx(state.alpha, 0)) {
        return; // early out
    }

    // plot the text
    Plotter::TextInfo text_info;
    text_info.font = state.font;
    text_info.translation = transform_by(V2f::zero(), state.xform);
    m_plotter->write(text, std::move(text_info));
}

NOTF_CLOSE_NAMESPACE
