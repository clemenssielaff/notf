#pragma once

#include <exception>

#include <nanovg/nanovg.h>

#include "graphics/rendercontext.hpp"

namespace notf {

struct RenderContext;

/** A Painter is an object that paints into a Widget's CanvasComponent.
 * To use it, subclass the Painter and implement the `_paint()` method.
 *
 * This is a thin wrapper around C methods exposed from nanovg.h written by Mikko Mononen.
 * The main raison d'être is to provide a single object for which bindings can be created in Python.
 * Some of the docstrings are verbatim copies of their corresponding nvg methods.
 */
class Painter {

    friend class CanvasComponent; // may call paint

public: // methods
    explicit Painter() = default;

    virtual ~Painter() = default;

protected: // methods
    /** Actual paint method, must be implemented by subclasses.
     * @throw std::runtime_error    May throw an exception, which cancels the current frame
     */
    virtual void _paint(const RenderContext& context) = 0;

    // states

    /** Pushes and saves the current render state into a state stack.
     * A matching state_restore() must be used to restore the state.
     */
    void state_push() { nvgSave(m_context->nanovg_context); }

    /** Pops and restores current render state. */
    void state_restore()  { nvgRestore(m_context->nanovg_context); }

    /** Resets current render state to default values. Does not affect the render state stack. */
    void state_reset() { nvgReset(m_context->nanovg_context); }

private: // methods for CanvasComponent
    /** Paints into the given RenderContext. */
    void paint(const RenderContext& context);

private: // fields
    /** The RenderContext used for painting. */
    const RenderContext* m_context;
};

} // namespace notf
