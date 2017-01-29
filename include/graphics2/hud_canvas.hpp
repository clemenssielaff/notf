#pragma once

#include <vector>

#include "backend.hpp"
#include "common/aabr.hpp"
#include "common/float_utils.hpp"
#include "common/size2i.hpp"
#include "common/transform2.hpp"
#include "graphics2/hud_primitives.hpp"
#include "graphics2/hud_painter.hpp"
#include "utils/enum_to_number.hpp"

namespace notf {

class HUDCanvas {

public: // enum
    /** Command identifyers, type must be of the same size as a float. */
    enum class Command : uint32_t {
        MOVE = 0,
        LINE,
        BEZIER,
        WINDING,
        CLOSE,
    };

private: // class
    /******************************************************************************************************************/
    struct RenderState {
        RenderState()
            : stroke_width(1)
            , miter_limit(10)
            , alpha(1)
            , xform(Transform2::identity())
            , composition()
            , line_cap(LineCap::BUTT)
            , line_join(LineJoin::MITER)
            , fill(Color(255, 255, 255))
            , stroke()
            , scissor({Transform2::identity(), {-1, -1}})
        {
        }

        RenderState(const RenderState& other) = default;

        float stroke_width;
        float miter_limit;
        float alpha;
        Transform2 xform;
        BlendMode composition;
        LineCap line_cap;
        LineJoin line_join;
        Paint fill;
        Paint stroke;
        Scissor scissor;
    };

    /******************************************************************************************************************/
    /** The FrameGuard makes sure that for each call to `HUDCanvas::start_frame` there is a corresponding call to
     * either `HUDCanvas::_end_frame` on success or `HUDCanvas::_abort_frame` in case of an error.
     *
     * It is returned by `HUDCanvas::begin_frame` and must remain on the stack until the rendering has finished.
     * Then, you need to call `FrameGuard::end()` to cleanly end the frame.
     * If the FrameGuard is destroyed before `FrameGuard::end()` is called, the HUDCanvas is instructed to abort the
     * currently drawn frame.
     */
    class FrameGuard {

    public: // methods
        /** Constructor. */
        FrameGuard(HUDCanvas* context)
            : m_context(context) {}

        // no copy/assignment
        FrameGuard(const FrameGuard&) = delete;
        FrameGuard& operator=(const FrameGuard&) = default;

        /** Move Constructor. */
        FrameGuard(FrameGuard&& other)
            : m_context(other.m_context)
        {
            other.m_context = nullptr;
        }

        /** Destructor.
         * If the Frame object is destroyed before Frame::end() is called, the HUDCanvas's frame is cancelled.
         */
        ~FrameGuard()
        {
            if (m_context) {
                m_context->_abort_frame();
            }
        }

        /** Cleanly ends the HUDCanvas's current frame. */
        void end()
        {
            if (m_context) {
                m_context->_end_frame();
                m_context = nullptr;
            }
        }

    private: // fields
        /** HUDCanvas currently draing a frame.*/
        HUDCanvas* m_context;
    };

public: // methods
    HUDCanvas(Size2i window_size, float pixel_ratio = 1)
        : m_window_size(std::move(window_size))
        , m_pixel_ratio(1)
//        , m_backend()
        , m_states()
        , m_commands(256)
        , m_current_command(0)
        , m_pos(Vector2::zero())
    {
        set_pixel_ratio(std::move(pixel_ratio));
    }

    FrameGuard begin_frame()
    {
        m_states.clear();
        m_states.emplace_back(RenderState());

//        m_backend.renderViewport(*this);

        return FrameGuard(this);
    }

    size_t push_state()
    {
        assert(!m_states.empty());
        m_states.emplace_back(m_states.back());
        return m_states.size() - 1;
    }

    size_t pop_state()
    {
        if (m_states.size() > 1) {
            m_states.pop_back();
        }
        assert(!m_states.empty());
        return m_states.size() - 1;
    }

    const RenderState& get_current_state() const
    {
        assert(!m_states.empty());
        return m_states.back();
    }

    void set_pixel_ratio(float ratio)
    {
        tessTol       = 0.25f / ratio;
        distTol       = 0.01f / ratio;
        fringeWidth   = 1.0f / ratio;
        m_pixel_ratio = std::move(ratio);
    }

    void set_stroke_width(const float width) { _get_current_state().stroke_width = std::move(width); }

    void set_miter_limit(const float limit) { _get_current_state().miter_limit = std::move(limit); }

    void set_line_cap(const LineCap cap) { _get_current_state().line_cap = std::move(cap); }

    void set_line_join(const LineJoin join) { _get_current_state().line_join = std::move(join); }

    void set_alpha(const float alpha) { _get_current_state().alpha = std::move(alpha); }

    void set_stroke_color(const Color color) { _get_current_state().stroke.set_color(std::move(color)); }

    void set_stroke_paint(Paint paint)
    {
        RenderState& current_state = _get_current_state();
        paint.xform *= current_state.xform;
        current_state.stroke = std::move(paint);
    }

    void set_fill_color(const Color color) { _get_current_state().fill.set_color(std::move(color)); }

    void set_fill_paint(Paint paint)
    {
        RenderState& current_state = _get_current_state();
        paint.xform *= current_state.xform;
        current_state.fill = std::move(paint);
    }

    void set_blend_mode(const BlendMode mode) { _get_current_state().composition = std::move(mode); }

    void transform(const Transform2& transform) { _get_current_state().xform *= transform; }

    void reset_transform() { _get_current_state().xform = Transform2::identity(); }

    const Transform2& get_transform() const { return get_current_state().xform; }

    void set_scissor(const Aabr& aabr)
    {
        RenderState& current_state  = _get_current_state();
        current_state.scissor.xform = Transform2::translation(aabr.center());
        current_state.scissor.xform *= current_state.xform;
        current_state.scissor.extend = aabr.extend() / 2;
    }

    // nanovg has a function `nvgIntersectScissor` that tries to approximate the intersection of two (not aligned)
    // rectangles ... I leave that out for now

    // nanovg offers functions for image patterns (paint) and general image management
    // I think I want something more general, my own image class or something, so I'll leave that out for now

    void begin_path()
    {
        m_commands.clear();
        m_paths.clear();
    }

    void move_to(const Vector2 pos)
    {
        // it might actually be better to pass a Command and a vector of Vector2? instead of unwrapping the arguments
        // before sending them to _append_commands
        _append_commands({static_cast<float>(to_number(Command::MOVE)), std::move(pos.x), std::move(pos.y)});
    }

    void line_to(const Vector2 pos)
    {
        _append_commands({static_cast<float>(to_number(Command::LINE)), std::move(pos.x), std::move(pos.y)});
    }

    void bezier_to(const Vector2 ctrl1, const Vector2 ctrl2, const Vector2 end)
    {
        _append_commands({
            static_cast<float>(to_number(Command::BEZIER)),
            std::move(ctrl1.x), std::move(ctrl1.y),
            std::move(ctrl2.x), std::move(ctrl2.y),
            std::move(end.x), std::move(end.y),
        });
    }

    void add_rect(const Aabr& rect)
    {
        add_rect(rect.x(), rect.y(), rect.width(), rect.height());
    }

    void add_rect(const float x, const float y, const float w, const float h)
    {
        _append_commands({
            static_cast<float>(to_number(Command::MOVE)), x, y,
            static_cast<float>(to_number(Command::LINE)), x, y + h,
            static_cast<float>(to_number(Command::LINE)), x + w, y + h,
            static_cast<float>(to_number(Command::LINE)), x + w, y,
            static_cast<float>(to_number(Command::CLOSE)),
        });
    }

    void add_ellipse(const Vector2 center, const Size2f extend)
    {
        add_ellipse(std::move(center.x), std::move(center.y),
                    std::move(extend.width), std::move(extend.height));
    }

    void add_ellipse(const float cx, const float cy, const float rx, const float ry)
    {
        _append_commands({static_cast<float>(to_number(Command::MOVE)), cx - rx, cy,
                          static_cast<float>(to_number(Command::BEZIER)), cx - rx, cy + ry * KAPPA, cx - rx * KAPPA, cy + ry, cx, cy + ry,
                          static_cast<float>(to_number(Command::BEZIER)), cx + rx * KAPPA, cy + ry, cx + rx, cy + ry * KAPPA, cx + rx, cy,
                          static_cast<float>(to_number(Command::BEZIER)), cx + rx, cy - ry * KAPPA, cx + rx * KAPPA, cy - ry, cx, cy - ry,
                          static_cast<float>(to_number(Command::BEZIER)), cx - rx * KAPPA, cy - ry, cx - rx, cy - ry * KAPPA, cx - rx, cy,
                          static_cast<float>(to_number(Command::CLOSE))});
    }

    void add_circle(const Vector2 center, const float radius)
    {
        add_ellipse(std::move(center.x), std::move(center.y), radius, std::move(radius));
    }

    void quad_to(const Vector2 ctrl, const Vector2 end)
    {
        // in order to construct a quad spline with a bezier command we need the position of the last point
        // to infer, where the ctrl points for the bezier is located
        // this works, at the moment, as in nanovg, by storing the (local) position of the stylus, but that's a crutch
        // ideally, there should be a Command::QUAD, if we really need this function
        float x0 = m_pos.x;
        float y0 = m_pos.y;
        _append_commands({
            static_cast<float>(to_number(Command::BEZIER)),
            x0 + 2.0f / 3.0f * (ctrl.x - x0), y0 + 2.0f / 3.0f * (ctrl.y - y0),
            end.x + 2.0f / 3.0f * (ctrl.x - end.x), end.y + 2.0f / 3.0f * (ctrl.y - end.y),
            std::move(end.x), std::move(end.y),
        });
    }

    void add_rounded_rect(const Aabr& rect, const float radius)
    {
        add_rounded_rect(rect.x(), rect.y(), rect.width(), rect.height(),
                         radius, radius, radius, std::move(radius));
    }

    void add_rounded_rect(const float x, const float y, const float w, const float h,
                          const float rtl, const float rtr, const float rbr, const float rbl)
    {
        if (rtl < 0.1f && rtr < 0.1f && rbr < 0.1f && rbl < 0.1f) {
            add_rect(std::move(x), std::move(y), std::move(w), std::move(h));
            return;
        }
        const float halfw = abs(w) * 0.5f;
        const float halfh = abs(h) * 0.5f;
        const float rxBL = min(rbl, halfw) * sign(w), ryBL = min(rbl, halfh) * sign(h);
        const float rxBR = min(rbr, halfw) * sign(w), ryBR = min(rbr, halfh) * sign(h);
        const float rxTR = min(rtr, halfw) * sign(w), ryTR = min(rtr, halfh) * sign(h);
        const float rxTL = min(rtl, halfw) * sign(w), ryTL = min(rtl, halfh) * sign(h);
        _append_commands({static_cast<float>(to_number(Command::MOVE)), x, y + ryTL,
                          static_cast<float>(to_number(Command::LINE)), x, y + h - ryBL,
                          static_cast<float>(to_number(Command::BEZIER)), x, y + h - ryBL * (1 - KAPPA), x + rxBL * (1 - KAPPA), y + h, x + rxBL, y + h,
                          static_cast<float>(to_number(Command::LINE)), x + w - rxBR, y + h,
                          static_cast<float>(to_number(Command::BEZIER)), x + w - rxBR * (1 - KAPPA), y + h, x + w, y + h - ryBR * (1 - KAPPA), x + w, y + h - ryBR,
                          static_cast<float>(to_number(Command::LINE)), x + w, y + ryTR,
                          static_cast<float>(to_number(Command::BEZIER)), x + w, y + ryTR * (1 - KAPPA), x + w - rxTR * (1 - KAPPA), y, x + w - rxTR, y,
                          static_cast<float>(to_number(Command::LINE)), x + rxTL, y,
                          static_cast<float>(to_number(Command::BEZIER)), x + rxTL * (1 - KAPPA), y, x, y + ryTL * (1 - KAPPA), x, y + ryTL,
                          static_cast<float>(to_number(Command::CLOSE))});
    }

    // nanovg has two additional methods: `nvgArc` and `nvgArcTo`, which are quite large so I ignore them for now

    void set_winding(const Winding winding)
    {
        _append_commands({static_cast<float>(to_number(Command::WINDING)),
                          static_cast<float>(to_number(std::move(winding)))});
    }

    void close_path()
    {
        _append_commands({static_cast<float>(to_number(Command::CLOSE))});
    }

private: //
    void _append_commands(std::vector<float>&& commands);

    void _abort_frame() { /*m_backend.renderCancel();*/ }

    void _end_frame() { /*m_backend.renderFlush(*this);*/ }

    RenderState& _get_current_state()
    {
        assert(!m_states.empty());
        return m_states.back();
    }

private: // fields
    Size2i m_window_size;
    float m_pixel_ratio;
//    RenderBackend m_backend;
    std::vector<RenderState> m_states;

    /** Bytecode-like instructions, separated by COMMAND values. */
    std::vector<float> m_commands;

    /** Index of the current Command. */
    size_t m_current_command;

    /** Current position of the 'stylus', as the last Command left it. */
    Vector2 m_pos; // actually I think it should go into the state

    //    float* commands; // buffer containing command ids (as int) and arguments (as floats)
    //    int ccommands;   // maximal command id?
    //    int ncommands;   // current command id?

    //    float commandx, commandy;
    PathCache m_paths;
    float tessTol;
    float distTol;
    float fringeWidth;
};

} // namespace notf
