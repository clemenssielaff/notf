#pragma once

#include <vector>

#include "backend.hpp"
#include "common/size2i.hpp"
#include "common/transform2.hpp"
#include "painter.hpp"
#include "path.hpp"

namespace notf {

class RenderContext {

private: // class
    /******************************************************************************************************************/
    struct RenderState {
        RenderState()
            : stroke_width(1)
            , miter_limit(10)
            , alpha(1)
            , xform(Transform2::identity())
            , composition()
            , line_cap(Painter::LineCap::BUTT)
            , line_join(Painter::LineJoin::MITER)
            , fill(Color(255, 255, 255))
            , stroke()
            , scissor()
        {
        }

        RenderState(const RenderState& other) = default;

        float stroke_width;
        float miter_limit;
        float alpha;
        Transform2 xform;
        BlendMode composition;
        Painter::LineCap line_cap;
        Painter::LineJoin line_join;
        Painter::Paint fill;
        Painter::Paint stroke;
        Painter::Scissor scissor;
    };

    /******************************************************************************************************************/
    /** The FrameGuard makes sure that for each call to `RenderContext::start_frame` there is a corresponding call to
     * either `RenderContext::_end_frame` on success or `RenderContext::_abort_frame` in case of an error.
     *
     * It is returned by `RenderContext::begin_frame` and must remain on the stack until the rendering has finished.
     * Then, you need to call `FrameGuard::end()` to cleanly end the frame.
     * If the FrameGuard is destroyed before `FrameGuard::end()` is called, the RenderContext is instructed to abort the
     * currently drawn frame.
     */
    class FrameGuard {

    public: // methods
        /** Constructor. */
        FrameGuard(RenderContext* context)
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
         * If the Frame object is destroyed before Frame::end() is called, the RenderContext's frame is cancelled.
         */
        ~FrameGuard()
        {
            if (m_context) {
                m_context->_abort_frame();
            }
        }

        /** Cleanly ends the RenderContext's current frame. */
        void end()
        {
            if (m_context) {
                m_context->_end_frame();
                m_context = nullptr;
            }
        }

    private: // fields
        /** RenderContext currently draing a frame.*/
        RenderContext* m_context;
    };

public: // methods
    RenderContext(Size2i window_size, float pixel_ratio = 1)
        : m_window_size(std::move(window_size))
        , m_pixel_ratio(1)
        , m_backend()
        , m_states()
    {
        set_pixel_ratio(std::move(pixel_ratio));
    }

    FrameGuard begin_frame()
    {
        m_states.clear();
        m_states.emplace_back(RenderState());

        m_backend.renderViewport(*this);

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

    void set_line_cap(const Painter::LineCap cap) { _get_current_state().line_cap = std::move(cap); }

    void set_line_join(const Painter::LineJoin join) { _get_current_state().line_join = std::move(join); }

    void set_alpha(const float alpha) { _get_current_state().alpha = std::move(alpha); }

    void set_stroke_color(const Color color) { _get_current_state().stroke.set_color(std::move(color)); }

    void set_stroke_paint(Painter::Paint paint)
    {
        RenderState& current_state = _get_current_state();
        paint.xform *= current_state.xform;
        current_state.stroke = std::move(paint);
    }

    void set_fill_color(const Color color) { _get_current_state().fill.set_color(std::move(color)); }

    void set_fill_paint(Painter::Paint paint)
    {
        RenderState& current_state = _get_current_state();
        paint.xform *= current_state.xform;
        current_state.fill = std::move(paint);
    }

    void transform(const Transform2& transform) { _get_current_state().xform *= transform; }

    void reset_transform() { _get_current_state().xform = Transform2::identity(); }

    const Transform2& get_transform() const { return get_current_state().xform; }

private: //
    void _abort_frame() { m_backend.renderCancel(); }

    void _end_frame() { m_backend.renderFlush(*this); }

    RenderState& _get_current_state()
    {
        assert(!m_states.empty());
        return m_states.back();
    }

private: // fields
    Size2i m_window_size;
    float m_pixel_ratio;
    RenderBackend m_backend;
    std::vector<RenderState> m_states;

    float* commands; // buffer containing command ids (as int) and arguments (as floats)
    int ccommands;   // maximal command id?
    int ncommands;   // current command id?

    float commandx, commandy;
    PathCache cache;
    float tessTol;
    float distTol;
    float fringeWidth;
};

/**********************************************************************************************************************/

} // namespace notf
