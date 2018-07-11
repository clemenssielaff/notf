#include "app/widget/painterpreter.hpp"

#include "app/widget/widget.hpp"
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

        m_paths.clear();
        m_points.clear();

        m_base_xform = widget.get_xform<Widget::Space::WINDOW>();
        m_base_clipping = widget.get_clipping_rect();
        m_bounds = Aabrf::wrongest();
    }

    { // parse the Widget Design
        using Word = WidgetDesign::Word;
        const WidgetDesign& design = Widget::Access<Painterpreter>::get_design(widget);
        const std::vector<Word>& buffer = WidgetDesign::Access<Painterpreter>::get_buffer(design);
        for (size_t index = 0; index < buffer.size();) {
            switch (WidgetDesign::CommandType(buffer[index])) {

            case WidgetDesign::CommandType::PUSH_STATE: {
                _push_state();
                index += WidgetDesign::get_command_size<WidgetDesign::PushStateCommand>();
            } break;

            case WidgetDesign::CommandType::POP_STATE: {
                _pop_state();
                index += WidgetDesign::get_command_size<WidgetDesign::PopStateCommand>();
            } break;

            case WidgetDesign::CommandType::START_PATH: {
                m_paths.clear();
                m_points.clear();
                m_bounds = Aabrf::wrongest();
                index += WidgetDesign::get_command_size<WidgetDesign::BeginPathCommand>();
            } break;

            case WidgetDesign::CommandType::SET_WINDING: {
                if (!m_paths.empty()) {
                    m_paths.back().winding = static_cast<Paint::Winding>(buffer[index + 1]);
                }
                index += WidgetDesign::get_command_size<WidgetDesign::SetWindingCommand>();
            } break;

            case WidgetDesign::CommandType::CLOSE_PATH: {
                if (!m_paths.empty()) {
                    m_paths.back().is_closed = true;
                }
                index += WidgetDesign::get_command_size<WidgetDesign::ClosePathCommand>();
            } break;

            case WidgetDesign::CommandType::MOVE: {
                _create_path();
                const auto cmd = design.map_command<WidgetDesign::MoveCommand>(index);
                _add_point(cmd.pos, Point::Flags::CORNER);
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;

            case WidgetDesign::CommandType::LINE: {
                if (m_paths.empty()) {
                    _create_path();
                }
                const auto cmd = design.map_command<WidgetDesign::LineCommand>(index);
                _add_point(cmd.pos, Point::Flags::CORNER);
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;

            case WidgetDesign::CommandType::BEZIER: {
                //                if (m_paths.empty()) {
                //                    _add_path();
                //                }
                //                Vector2f stylus;
                //                if (m_points.empty()) {
                //                    stylus = Vector2f::zero();
                //                    _add_point(stylus, Point::Flags::CORNER);
                //                }
                //                else {
                //                    Vector2f pos = m_points.back().pos;
                //                    make_const(_get_current_state()).xform.inverse().transform(pos);
                //                    stylus = pos;
                //                }
                const auto cmd = design.map_command<WidgetDesign::BezierCommand>(index);
                //                _tesselate_bezier(stylus.x(), stylus.y(), cmd.ctrl1.x(), cmd.ctrl1.y(), cmd.ctrl2.x(),
                //                cmd.ctrl2.y(),
                //                                  cmd.end.x(), cmd.end.y());
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;

            case WidgetDesign::CommandType::FILL: {
                //                _fill();
                index += WidgetDesign::get_command_size<WidgetDesign::FillCommand>();
            } break;

            case WidgetDesign::CommandType::STROKE: {
                //                _stroke();
                index += WidgetDesign::get_command_size<WidgetDesign::StrokeCommand>();
            } break;

            case WidgetDesign::CommandType::SET_TRANSFORM: {
                const auto cmd = design.map_command<WidgetDesign::SetTransformCommand>(index);
                _get_current_state().xform = cmd.xform;
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;

            case WidgetDesign::CommandType::RESET_TRANSFORM: {
                const auto cmd = design.map_command<WidgetDesign::ResetTransformCommand>(index);
                _get_current_state().xform = m_base_xform;
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;

            case WidgetDesign::CommandType::TRANSFORM: {
                const auto cmd = design.map_command<WidgetDesign::TransformCommand>(index);
                _get_current_state().xform *= cmd.xform;
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;

            case WidgetDesign::CommandType::TRANSLATE: {
                const auto cmd = design.map_command<WidgetDesign::TranslationCommand>(index);
                _get_current_state().xform.translate(cmd.delta);
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;

            case WidgetDesign::CommandType::ROTATE: {
                const auto cmd = design.map_command<WidgetDesign::RotationCommand>(index);
                _get_current_state().xform.rotate(cmd.angle);
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;

            case WidgetDesign::CommandType::SET_CLIPPING: {
                const auto cmd = design.map_command<WidgetDesign::SetClippingCommand>(index);
                //                _set_scissor(cmd.scissor);
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;

            case WidgetDesign::CommandType::RESET_CLIPPING: {
                const auto cmd = design.map_command<WidgetDesign::ResetClippingCommand>(index);
                _get_current_state().clipping = m_base_clipping;
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;

            case WidgetDesign::CommandType::SET_FILL_COLOR: {
                const auto cmd = design.map_command<WidgetDesign::FillColorCommand>(index);
                _get_current_state().fill_paint.set_color(cmd.color);
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;

            case WidgetDesign::CommandType::SET_FILL_PAINT: {
                const auto cmd = design.map_command<WidgetDesign::FillPaintCommand>(index);
                _get_current_state().fill_paint = cmd.paint;
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;

            case WidgetDesign::CommandType::SET_STROKE_COLOR: {
                const auto cmd = design.map_command<WidgetDesign::StrokeColorCommand>(index);
                _get_current_state().stroke_paint.set_color(cmd.color);
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;

            case WidgetDesign::CommandType::SET_STROKE_PAINT: {
                const auto cmd = design.map_command<WidgetDesign::StrokePaintCommand>(index);
                _get_current_state().stroke_paint = cmd.paint;
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;

            case WidgetDesign::CommandType::SET_STROKE_WIDTH: {
                const auto cmd = design.map_command<WidgetDesign::StrokeWidthCommand>(index);
                _get_current_state().stroke_width = cmd.stroke_width;
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;

            case WidgetDesign::CommandType::SET_BLEND_MODE: {
                const auto cmd = design.map_command<WidgetDesign::BlendModeCommand>(index);
                _get_current_state().blend_mode = cmd.blend_mode;
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;

            case WidgetDesign::CommandType::SET_ALPHA: {
                const auto cmd = design.map_command<WidgetDesign::SetAlphaCommand>(index);
                _get_current_state().alpha = cmd.alpha;
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;

            case WidgetDesign::CommandType::SET_MITER_LIMIT: {
                const auto cmd = design.map_command<WidgetDesign::MiterLimitCommand>(index);
                _get_current_state().miter_limit = cmd.miter_limit;
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;

            case WidgetDesign::CommandType::SET_LINE_CAP: {
                const auto cmd = design.map_command<WidgetDesign::LineCapCommand>(index);
                _get_current_state().line_cap = cmd.line_cap;
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;

            case WidgetDesign::CommandType::SET_LINE_JOIN: {
                const auto cmd = design.map_command<WidgetDesign::LineJoinCommand>(index);
                _get_current_state().line_join = cmd.line_join;
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;

            case WidgetDesign::CommandType::WRITE: {
                const auto cmd = design.map_command<WidgetDesign::WriteCommand>(index);
                //                _render_text(*cmd.text.get(), cmd.font);
                index += WidgetDesign::get_command_size<decltype(cmd)>();
            } break;
            }
        }
    }
}

void Painterpreter::_push_state() { m_states.emplace_back(m_states.back()); }

void Painterpreter::_pop_state()
{
    assert(m_states.size() > 1);
    m_states.pop_back();
}

void Painterpreter::_add_point(Vector2f position, const Point::Flags flags)
{
    if (m_paths.empty()) {
        _create_path();
    }

    const State& current_state = _get_current_state();

    // bring the Point from Painter- into Cell-space
    current_state.xform.transform(position);

    // if the Point is not significantly different from the last one, use that instead
    if (!m_points.empty()) {
        Point& last_point = m_points.back();
        if (position.is_approx(last_point.pos, current_state.distance_tolerance)) {
            last_point.flags = static_cast<Point::Flags>(last_point.flags | flags);
            return;
        }
    }

    // ... or if it is significantly different, append a new Point to the current Path
    m_points.emplace_back(Point{std::move(position), Vector2f::zero(), Vector2f::zero(), 0.f, flags});
    m_paths.back().point_count++;
}

void Painterpreter::_create_path()
{
    Path& new_path = create_back(m_paths);
    new_path.first_point = m_points.size();
}

NOTF_CLOSE_NAMESPACE
