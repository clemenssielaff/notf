#include "notf/graphic/plotter/painter.hpp"

#include "notf/graphic/plotter/design.hpp"

NOTF_OPEN_NAMESPACE

// painter ========================================================================================================== //

Painter::Painter(PlotterDesign& design) : m_design(design) { m_design.reset(); }

void Painter::push_state() {
    NOTF_ASSERT(!m_states.empty());
    m_states.emplace_back(_get_state());
    m_design.add_command<PlotterDesign::PushState>();
}

void Painter::pop_state() {
    NOTF_ASSERT(!m_states.empty());
    if (m_states.size() == 1) {
        reset_state();
    } else {
        m_design.add_command<PlotterDesign::PopState>();
        m_states.pop_back();
    }
}

void Painter::reset_state() {
    const static State default_state = {};
    m_states.back() = default_state;
    m_design.add_command<PlotterDesign::ResetState>();
}

void Painter::set_path(Path2Ptr path) {
    auto& state = _get_state();
    if (path != state.path) {
        state.path = std::move(path);
        m_design.add_command<PlotterDesign::SetPath>(state.path);
    }
}

void Painter::set_font(FontPtr font) {
    auto& state = _get_state();
    if (font != state.font) {
        state.font = std::move(font);
        m_design.add_command<PlotterDesign::SetFont>(state.font);
    }
}

void Painter::set_paint(Paint paint) {
    auto& state = _get_state();
    if (paint != state.paint) {
        state.paint = std::move(paint);
        m_design.add_command<PlotterDesign::SetPaint>(state.paint);
    }
}

void Painter::set_clip(Aabrf clip) {
    auto& state = _get_state();
    if (clip != state.clip) {
        state.clip = std::move(clip);
        m_design.add_command<PlotterDesign::SetClip>(state.clip);
    }
}

void Painter::set_transform(const M3f& xform) {
    auto& state = _get_state();
    if (xform != state.xform) {
        state.xform = std::move(xform);
        m_design.add_command<PlotterDesign::SetXform>(state.xform);
    }
}

Painter& Painter::operator*=(const M3f& xform) {
    _get_state().xform *= xform;
    return *this;
}

void Painter::fill() { m_design.add_command<PlotterDesign::Fill>(); }

void Painter::stroke() { m_design.add_command<PlotterDesign::Stroke>(); }

void Painter::write(std::string text) { m_design.add_command<PlotterDesign::Write>(std::move(text)); }

void Painter::set_blend_mode(const BlendMode mode) {
    auto& state = _get_state();
    if (mode != state.blend_mode) {
        state.blend_mode = mode;
        m_design.add_command<PlotterDesign::SetBlendMode>(mode);
    }
}

void Painter::set_alpha(const float alpha) {
    auto& state = _get_state();
    if (!is_approx(alpha, state.alpha)) {
        state.alpha = alpha;
        m_design.add_command<PlotterDesign::SetAlpha>(alpha);
    }
}

void Painter::set_line_cap(const LineCap cap) {
    auto& state = _get_state();
    if (cap != state.line_cap) {
        state.line_cap = cap;
        m_design.add_command<PlotterDesign::SetLineCap>(cap);
    }
}

void Painter::set_line_join(const LineJoin join) {
    auto& state = _get_state();
    if (join != state.line_join) {
        state.line_join = join;
        m_design.add_command<PlotterDesign::SetLineJoin>(join);
    }
}

void Painter::set_stroke_width(const float stroke_width) {
    auto& state = _get_state();
    if (!is_approx(stroke_width, state.stroke_width)) {
        state.stroke_width = stroke_width;
        m_design.add_command<PlotterDesign::SetStrokeWidth>(stroke_width);
    }
}

NOTF_CLOSE_NAMESPACE
