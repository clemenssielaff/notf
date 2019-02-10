#pragma once

#include "notf/meta/smart_ptr.hpp"

#include "notf/graphic/opengl_buffer.hpp"

NOTF_OPEN_NAMESPACE

// drawcall buffer ================================================================================================== //

namespace detail {

struct DrawElementsIndirectCommand {
    uint count;
    uint instanceCount;
    uint firstIndex;
    int baseVertex;

private:
    NOTF_UNUSED const uint _reserved = 0;
};

} // namespace detail

class DrawCallBuffer : public OpenGLBuffer<detail::OpenGLBufferType::DRAWCALL, detail::DrawElementsIndirectCommand> {

    // types ----------------------------------------------------------------------------------- //
public:
    ///
    using DrawCall = detail::DrawElementsIndirectCommand;

private:
    /// Base OpenGLBuffer class.
    using super_t = OpenGLBuffer<detail::OpenGLBufferType::DRAWCALL, DrawCall>;

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(DrawCallBuffer);

    /// Constructor.
    /// @param name         Human-readable name of this OpenGLBuffer.
    /// @param usage_hint   The expected usage of the data stored in this buffer.
    /// @throws OpenGLError If the buffer could not be allocated.
    DrawCallBuffer(std::string name, const UsageHint usage_hint) : super_t(std::move(name), usage_hint) {}
};

NOTF_CLOSE_NAMESPACE
