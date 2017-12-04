#include "cel/stroker.hpp"

#include "common/log.hpp"
#include "common/matrix4.hpp"
#include "common/vector.hpp"
#include "common/vector2.hpp"
#include "core/opengl.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/index_array.hpp"
#include "graphics/pipeline.hpp"
#include "graphics/shader.hpp"
#include "graphics/vertex_array.hpp"

namespace {

using namespace notf;

struct VertexPosh : public AttributeTrait {
    constexpr static uint location = 0;
    using type                     = Vector2f;
    using kind                     = AttributeKind::Position;
};

struct LeftCtrlPosh : public AttributeTrait {
    constexpr static uint location = 1;
    using type                     = Vector2f;
};

struct RightCtrlPosh : public AttributeTrait {
    constexpr static uint location = 2;
    using type                     = Vector2f;
};

using LineVertexArray = VertexArray<VertexPosh, LeftCtrlPosh, RightCtrlPosh>;
using LineIndexArray  = IndexArray<GLuint>;

void set_pos(LineVertexArray::Vertex& vertex, Vector2f pos) { std::get<0>(vertex) = std::move(pos); }
void set_first_ctrl(LineVertexArray::Vertex& vertex, Vector2f pos) { std::get<1>(vertex) = std::move(pos); }
void set_second_ctrl(LineVertexArray::Vertex& vertex, Vector2f pos) { std::get<2>(vertex) = std::move(pos); }

void set_modified_first_ctrl(LineVertexArray::Vertex& vertex, const CubicBezier2f::Segment& left_segment)
{
    const Vector2f delta = left_segment.ctrl2 - left_segment.end;
    set_first_ctrl(vertex, delta.is_zero() ? -(left_segment.tangent(1).normalize()) :
                                             delta.normalize() * (delta.magnitude() + 1));
}

void set_modified_second_ctrl(LineVertexArray::Vertex& vertex, const CubicBezier2f::Segment& right_segment)
{
    const Vector2f delta = right_segment.ctrl1 - right_segment.start;
    set_second_ctrl(vertex, delta.is_zero() ? (right_segment.tangent(0).normalize()) :
                                             delta.normalize() * (delta.magnitude() + 1));
}

} // namespace

namespace notf {

Stroker::Stroker(GraphicsContextPtr& context)
    : m_graphics_context(*context), m_pipeline(), m_vao_id(0), m_vertices(), m_indices(), m_spline_buffer()
{
    // vao
    gl_check(glGenVertexArrays(1, &m_vao_id));
    if (!m_vao_id) {
        throw_runtime_error("Failed to allocate the Stroker VAO");
    }
    auto vao_guard = VaoGuard(m_vao_id);

    { // pipeline
        const std::string vertex_src  = load_file("/home/clemens/code/notf/res/shaders/line.vert");
        VertexShaderPtr vertex_shader = VertexShader::build(context, "line.vert", vertex_src);

        const std::string tess_src       = load_file("/home/clemens/code/notf/res/shaders/line.tess");
        const std::string eval_src       = load_file("/home/clemens/code/notf/res/shaders/line.eval");
        TesselationShaderPtr tess_shader = TesselationShader::build(context, "line.tess", tess_src.c_str(), eval_src);

        const std::string frag_src    = load_file("/home/clemens/code/notf/res/shaders/line.frag");
        FragmentShaderPtr frag_shader = FragmentShader::build(context, "line.frag", frag_src);

        m_pipeline = Pipeline::create(context, vertex_shader, tess_shader, frag_shader);

        tess_shader->set_uniform("aa_width", 1.5f);
    }

    { // vertices
        VertexArrayType::Args vertex_args;
        vertex_args.usage = GL_DYNAMIC_DRAW;
        m_vertices        = std::make_unique<LineVertexArray>(std::move(vertex_args));
        m_vertices->init();
    }

    // indices
    m_indices = std::make_unique<LineIndexArray>();
    m_indices->init();
}

Stroker::~Stroker() { gl_check(glDeleteVertexArrays(1, &m_vao_id)); }

void Stroker::add_spline(CubicBezier2f spline)
{
    if (spline.segments.empty()) {
        return;
    }
    m_spline_buffer.emplace_back(std::move(spline));
}

/// The Stroker uses OpenGL shader tesselation for most of the primitive construction, it only passes the bare minimum
/// of information on to the GPU. There are however a few things to consider when transforming a Bezier curve into the
/// Stroker GPU representation.
///
/// The shader takes a patch that is made up of two vertices v1 and v2.
/// Each vertex has 3 attributes:
///     a1. its position in absolute screen coordinates
///     a2. the modified(*) position of a bezier control point to the left, in screen coordinates relative to a1.
///     a3. the modified(*) position of a bezier control point to the right, in screen coodinates relative to a1.
///
/// When drawing the spline from a patch of two vertices, only the middle 4 attributes are used:
///
///     v1.a1       is the start point of the bezier spline.
///     v1 + v1.a3  is the first control point
///     v2 + v2.a2  is the second control point
///     v2.a1       is the end point
///
/// (*)
/// For the correct calculation of caps and joints, we need the tangent directions at each vertex.
/// This information is easy to derive if a2 != a1 and a3 != a1. If however, one of the two control points has zero
/// distance to the vertex, the shader would require the next patch in order to get the tangent - which doesn't work.
/// Therefore each control point is modified with the following formula (with T begin the tangent normal in the
/// direction of ax):
///
///     if ax - a1 == (0, 0):
///         ax' = T
///     else:
///         ax' = T * (||ax - a1|| + 1)     (whereby T = |ax - a1|)
///
/// Caps
/// ----
///
/// Without caps, lines would only be antialiased on either sides of the line, not their end points.
/// In order to tell the shader to render a start- or end-cap, we pass two vertices with special requirements:
///
/// For the start cap:
///
///     v1.a1 == v2.a1 && v2.a2 == v2.a3 == (0,0)
///
/// For the end cap:
///
///     v1.a1 == v2.a1 && v1.a2 == v1.a3 == (0,0)
///
/// Thereby increasing the number of vertices per spline by two.
///
/// Joints
/// ------
///
/// In order to render multiple segments without a visual break, the Stroker adds intermediary joints.
/// A joint segment also consists of two vertices but imposes additional requirements:
///
///     v1.a1 == v2.a1
///
/// This is easily accomplished by re-using indices of the existing vertices and doesn't increase the size of the
/// vertex array.
///
void Stroker::apply_new()
{
    auto vao_guard = VaoGuard(m_vao_id);

    { // update line vertices
        std::vector<LineVertexArray::Vertex> vertices;
        for (const CubicBezier2f& spline : m_spline_buffer) {
            assert(!spline.segments.empty());

            { // first vertex
                const CubicBezier2f::Segment& first_segment = spline.segments.front();
                LineVertexArray::Vertex vertex;
                set_pos(vertex, first_segment.start);
                set_first_ctrl(vertex, -(first_segment.tangent(0).normalize()));
                set_modified_second_ctrl(vertex, first_segment);
                vertices.emplace_back(std::move(vertex));
            }

            // middle vertices
            for (size_t i = 0; i < spline.segments.size() - 1; ++i) {
                const CubicBezier2f::Segment& left_segment  = spline.segments[i];
                const CubicBezier2f::Segment& right_segment = spline.segments[i + 1];
                LineVertexArray::Vertex vertex;
                set_pos(vertex, left_segment.end);
                set_modified_first_ctrl(vertex, left_segment);
                set_modified_second_ctrl(vertex, right_segment);
                vertices.emplace_back(std::move(vertex));
            }

            { // last vertex
                const CubicBezier2f::Segment& last_segment = spline.segments.back();
                LineVertexArray::Vertex vertex;
                set_pos(vertex, last_segment.end);
                set_modified_first_ctrl(vertex, last_segment);
                set_second_ctrl(vertex, last_segment.tangent(1).normalize());
                vertices.emplace_back(std::move(vertex));
            }
        }

        LineVertexArray* line_vertices = static_cast<LineVertexArray*>(m_vertices.get());
        line_vertices->update(std::move(vertices));
    }

    { // update line indices
        std::vector<GLuint> indices;
        indices.reserve(static_cast<size_t>(m_vertices->size()) * 2);

        GLuint next_index = 0;
        for (const CubicBezier2f& spline : m_spline_buffer) {
            const size_t segment_count = spline.segments.size();
            for (ushort segment_index = 0; segment_index < segment_count; ++segment_index) {
                indices.emplace_back(next_index++);
                indices.emplace_back(next_index);
            }
            ++next_index;
        }

        LineIndexArray* line_indices = static_cast<LineIndexArray*>(m_indices.get());
        line_indices->update(std::move(indices));
    }

    m_spline_buffer.clear();
}

void Stroker::render()
{
    auto vao_guard = VaoGuard(m_vao_id);

    m_graphics_context.bind_pipeline(m_pipeline);

    // pass the shader uniforms
    // TODO: get the window size from the graphics context?
    const TesselationShaderPtr& tess_shader = m_pipeline->tesselation_shader();
    const Matrix4f perspective              = Matrix4f::orthographic(0.f, 800.f, 0.f, 800.f, 0.f, 10000.f);
    tess_shader->set_uniform("projection", perspective);

    // TODO: stroke_width less than 1 should set a uniform that fades the line out and line widths of zero
    // should be ignored
    tess_shader->set_uniform("stroke_width", 10.f);

    gl_check(glPatchParameteri(GL_PATCH_VERTICES, 2));
    gl_check(glDrawElements(GL_PATCHES, static_cast<GLsizei>(m_indices->size()), m_indices->type(), nullptr));

    m_graphics_context.unbind_pipeline();
}

} // namespace notf
