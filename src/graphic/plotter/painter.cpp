#include "notf/graphic/plotter/painter.hpp"

#include "notf/common/thread.hpp"

#include "notf/graphic/plotter/design.hpp"

NOTF_OPEN_NAMESPACE

// painter ========================================================================================================== //

Painter::Painter(PlotterDesign& design) : m_design(design) {
    NOTF_ASSERT(this_thread::get_kind() == Thread::Kind::RENDER);
    PlotterDesign::AccessFor<Painter>::reset(m_design);
}

Painter::~Painter() {
    NOTF_ASSERT(this_thread::get_kind() == Thread::Kind::RENDER);
    PlotterDesign::AccessFor<Painter>::complete(m_design);
}

void Painter::push_state() {
    NOTF_ASSERT(!m_states.empty());
    m_states.emplace_back(_get_state());
    PlotterDesign::AccessFor<Painter>::add_command<PlotterDesign::PushState>(m_design);
}

void Painter::pop_state() {
    NOTF_ASSERT(!m_states.empty());
    if (m_states.size() == 1) {
        reset_state();
    } else {
        PlotterDesign::AccessFor<Painter>::add_command<PlotterDesign::PopState>(m_design);
        m_states.pop_back();
    }
}

void Painter::reset_state() {
    const static State default_state = {};
    m_states.back() = default_state;
    PlotterDesign::AccessFor<Painter>::add_command<PlotterDesign::ResetState>(m_design);
}

void Painter::set_path(Path2Ptr path) {
    auto& state = _get_state();
    if (path != state.path) {
        state.path = std::move(path);
        PlotterDesign::AccessFor<Painter>::add_command<PlotterDesign::SetPath>(m_design, state.path);
    }
}

void Painter::set_font(FontPtr font) {
    auto& state = _get_state();
    if (font != state.font) {
        state.font = std::move(font);
        PlotterDesign::AccessFor<Painter>::add_command<PlotterDesign::SetFont>(m_design, state.font);
    }
}

void Painter::set_paint(Paint paint) {
    auto& state = _get_state();
    if (paint != state.paint) {
        state.paint = std::move(paint);
        PlotterDesign::AccessFor<Painter>::add_command<PlotterDesign::SetPaint>(m_design, state.paint);
    }
}

void Painter::set_clip(Aabrf clip) {
    auto& state = _get_state();
    if (clip != state.clip) {
        state.clip = std::move(clip);
        PlotterDesign::AccessFor<Painter>::add_command<PlotterDesign::SetClip>(m_design, state.clip);
    }
}

void Painter::set_transform(const M3f& xform) {
    auto& state = _get_state();
    if (xform != state.xform) {
        state.xform = std::move(xform);
        PlotterDesign::AccessFor<Painter>::add_command<PlotterDesign::SetXform>(m_design, state.xform);
    }
}

Painter& Painter::operator*=(const M3f& xform) {
    _get_state().xform *= xform;
    return *this;
}

void Painter::fill() { PlotterDesign::AccessFor<Painter>::add_command<PlotterDesign::Fill>(m_design); }

void Painter::stroke() { PlotterDesign::AccessFor<Painter>::add_command<PlotterDesign::Stroke>(m_design); }

void Painter::write(std::string text) {
    PlotterDesign::AccessFor<Painter>::add_command<PlotterDesign::Write>(m_design, std::move(text));
}

void Painter::set_blend_mode(const BlendMode mode) {
    auto& state = _get_state();
    if (mode != state.blend_mode) {
        state.blend_mode = mode;
        PlotterDesign::AccessFor<Painter>::add_command<PlotterDesign::SetBlendMode>(m_design, mode);
    }
}

void Painter::set_alpha(const float alpha) {
    auto& state = _get_state();
    if (!is_approx(alpha, state.alpha)) {
        state.alpha = alpha;
        PlotterDesign::AccessFor<Painter>::add_command<PlotterDesign::SetAlpha>(m_design, alpha);
    }
}

void Painter::set_cap_style(const CapStyle cap) {
    auto& state = _get_state();
    if (cap != state.line_cap) {
        state.line_cap = cap;
        PlotterDesign::AccessFor<Painter>::add_command<PlotterDesign::SetLineCap>(m_design, cap);
    }
}

void Painter::set_joint_style(const JointStyle joint) {
    auto& state = _get_state();
    if (joint != state.joint_style) {
        state.joint_style = joint;
        PlotterDesign::AccessFor<Painter>::add_command<PlotterDesign::SetLineJoin>(m_design, joint);
    }
}

void Painter::set_stroke_width(const float stroke_width) {
    auto& state = _get_state();
    if (!is_approx(stroke_width, state.stroke_width)) {
        state.stroke_width = stroke_width;
        PlotterDesign::AccessFor<Painter>::add_command<PlotterDesign::SetStrokeWidth>(m_design, stroke_width);
    }
}

NOTF_CLOSE_NAMESPACE
