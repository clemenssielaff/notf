#pragma once

#include "notf/meta/smart_ptr.hpp"

#include "notf/graphic/opengl_buffer.hpp"

NOTF_OPEN_NAMESPACE

// drawcall buffer ================================================================================================== //

namespace detail {

/// Directly taken from:
/// https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDrawElementsIndirect.xhtml
struct DrawElementsIndirectCommand {
    uint index_count;    // number of indices to render
    uint instance_count; // number of instances to render
    uint first_index;    // first entry in the index buffer
    int first_vertex;    // first entry in the vertex buffer, addressable by index = 0

private:
    NOTF_UNUSED const uint _reserved = 0; // must be zero (for some reason)
};

} // namespace detail

class DrawCallBuffer : public OpenGLBuffer<detail::OpenGLBufferType::DRAWCALL, detail::DrawElementsIndirectCommand> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// DrawCall struct used to call `glDrawElementsIndirect`.
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

public:
    /// Factory.
    /// @param name         Human-readable name of this OpenGLBuffer.
    /// @param usage_hint   The expected usage of the data stored in this buffer.
    /// @throws OpenGLError If the buffer could not be allocated.
    static auto create(std::string name, const UsageHint usage_hint) {
        return _create_shared(std::move(name), std::move(usage_hint));
    }
};

/// DrawCallBuffer factory.
/// @param name         Human-readable name of this OpenGLBuffer.
/// @param usage_hint   The expected usage of the data stored in this buffer.
/// @throws OpenGLError If the buffer could not be allocated.
auto make_drawcall_buffer(std::string name,
                          const DrawCallBuffer::UsageHint usage_hint = AnyIndexBuffer::UsageHint::DEFAULT) {
    return DrawCallBuffer::create(std::move(name), usage_hint);
}

NOTF_CLOSE_NAMESPACE
