#include "graphics/engine/plotter.hpp"

#include "common/log.hpp"
#include "common/matrix4.hpp"
#include "common/vector.hpp"
#include "common/vector2.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/core/index_array.hpp"
#include "graphics/core/opengl.hpp"
#include "graphics/core/pipeline.hpp"
#include "graphics/core/shader.hpp"
#include "graphics/core/vertex_array.hpp"
#include "utils/narrow_cast.hpp"

namespace {

using namespace notf;

struct VertexPos : public AttributeTrait {
    constexpr static uint location = 0;
    using type                     = Vector2f;
    using kind                     = AttributeKind::Position;
};

struct LeftCtrlPos : public AttributeTrait {
    constexpr static uint location = 1;
    using type                     = Vector2f;
};

struct RightCtrlPos : public AttributeTrait {
    constexpr static uint location = 2;
    using type                     = Vector2f;
};

using PlotVertexArray = VertexArray<VertexPos, LeftCtrlPos, RightCtrlPos>;
using PlotIndexArray  = IndexArray<GLuint>;

const GLenum g_index_type = to_gl_type(PlotIndexArray::index_t{});

void set_pos(PlotVertexArray::Vertex& vertex, Vector2f pos) { std::get<0>(vertex) = std::move(pos); }
void set_first_ctrl(PlotVertexArray::Vertex& vertex, Vector2f pos) { std::get<1>(vertex) = std::move(pos); }
void set_second_ctrl(PlotVertexArray::Vertex& vertex, Vector2f pos) { std::get<2>(vertex) = std::move(pos); }

void set_modified_first_ctrl(PlotVertexArray::Vertex& vertex, const CubicBezier2f::Segment& left_segment)
{
    const Vector2f delta = left_segment.ctrl2 - left_segment.end;
    set_first_ctrl(vertex, delta.is_zero() ? -(left_segment.tangent(1).normalize()) :
                                             delta.normalize() * (delta.magnitude() + 1));
}

void set_modified_second_ctrl(PlotVertexArray::Vertex& vertex, const CubicBezier2f::Segment& right_segment)
{
    const Vector2f delta = right_segment.ctrl1 - right_segment.start;
    set_second_ctrl(vertex, delta.is_zero() ? (right_segment.tangent(0).normalize()) :
                                              delta.normalize() * (delta.magnitude() + 1));
}

} // namespace

namespace notf {

Plotter::Plotter(GraphicsContextPtr& context)
    : m_graphics_context(*context), m_pipeline(), m_vao_id(0), m_vertices(), m_indices(), m_batches(), m_calls()
{
    // vao
    gl_check(glGenVertexArrays(1, &m_vao_id));
    if (!m_vao_id) {
        throw_runtime_error("Failed to allocate the Plotter VAO");
    }
    const auto vao_guard = VaoBindGuard(m_vao_id);

    { // pipeline
        const std::string vertex_src  = load_file("/home/clemens/code/notf/res/shaders/plotter.vert");
        VertexShaderPtr vertex_shader = VertexShader::build(context, "plotter.vert", vertex_src);

        const std::string tess_src       = load_file("/home/clemens/code/notf/res/shaders/plotter.tess");
        const std::string eval_src       = load_file("/home/clemens/code/notf/res/shaders/plotter.eval");
        TesselationShaderPtr tess_shader = TesselationShader::build(context, "plotter.tess", tess_src, eval_src);

        const std::string frag_src    = load_file("/home/clemens/code/notf/res/shaders/plotter.frag");
        FragmentShaderPtr frag_shader = FragmentShader::build(context, "plotter.frag", frag_src);

        m_pipeline = Pipeline::create(context, vertex_shader, tess_shader, frag_shader);

        tess_shader->set_uniform("aa_width", 1.5f);
    }

    { // vertices
        VertexArrayType::Args vertex_args;
        vertex_args.usage = GL_DYNAMIC_DRAW;
        m_vertices        = std::make_unique<PlotVertexArray>(std::move(vertex_args));
        m_vertices->init();
    }

    // indices
    m_indices = std::make_unique<PlotIndexArray>();
    m_indices->init();
}

Plotter::~Plotter() { gl_check(glDeleteVertexArrays(1, &m_vao_id)); }

/// The Plotter uses OpenGL shader tesselation for most of the primitive construction, it only passes the bare minimum
/// of information on to the GPU. There are however a few things to consider when transforming a Bezier curve into the
/// Plotter GPU representation.
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
///     v1.a1 == v2.a1 && v2.a2 == (0,0)
///
/// For the end cap:
///
///     v1.a1 == v2.a1 && v1.a3 == (0,0)
///
/// If the tangent at the cap is required, you can simply invert the tangent obtained from the other ctrl point.
///
/// Joints
/// ------
///
/// In order to render multiple segments without a visual break, the Plotter adds intermediary joints.
/// A joint segment also consists of two vertices but imposes additional requirements:
///
///     v1.a1 == v2.a1
///
/// This is easily accomplished by re-using indices of the existing vertices and doesn't increase the size of the
/// vertex array.
///
void Plotter::parse()
{
    /// @brief Parses various types of calls.
    struct Parser {

        // fields ----------------------------------------------------------------------------------------------------//

        /// @brief Vertex target.
        std::vector<PlotVertexArray::Vertex>& vertices;

        /// @brief Index target.
        std::vector<GLuint>& indices;

        /// @brief Batch target.
        std::vector<Batch>& batches;

        // methods ---------------------------------------------------------------------------------------------------//

        /// @brief Parse a stroke call.
        void operator()(const StrokeCall& call) const
        {
            const CubicBezier2f& spline = call.spline;
            assert(!spline.segments.empty());

            const size_t index_offset = indices.size();

            { // update indices
                GLuint next_index = narrow_cast<GLuint>(vertices.size());

                // start cap
                indices.emplace_back(next_index);
                indices.emplace_back(next_index);

                for (ushort segment_index = 0; segment_index < spline.segments.size() - 1; ++segment_index) {
                    // first -> (n-1) segment
                    indices.emplace_back(next_index++);
                    indices.emplace_back(next_index);

                    // joint
                    indices.emplace_back(next_index);
                    indices.emplace_back(next_index);
                }

                // last segment
                indices.emplace_back(next_index++);
                indices.emplace_back(next_index);

                // end cap
                indices.emplace_back(next_index);
                indices.emplace_back(next_index);
            }

            { // first vertex
                const CubicBezier2f::Segment& first_segment = spline.segments.front();
                PlotVertexArray::Vertex vertex;
                set_pos(vertex, first_segment.start);
                set_first_ctrl(vertex, Vector2f::zero());
                set_modified_second_ctrl(vertex, first_segment);
                vertices.emplace_back(std::move(vertex));
            }

            // middle vertices
            for (size_t i = 0; i < spline.segments.size() - 1; ++i) {
                const CubicBezier2f::Segment& left_segment  = spline.segments[i];
                const CubicBezier2f::Segment& right_segment = spline.segments[i + 1];
                PlotVertexArray::Vertex vertex;
                set_pos(vertex, left_segment.end);
                set_modified_first_ctrl(vertex, left_segment);
                set_modified_second_ctrl(vertex, right_segment);
                vertices.emplace_back(std::move(vertex));
            }

            { // last vertex
                const CubicBezier2f::Segment& last_segment = spline.segments.back();
                PlotVertexArray::Vertex vertex;
                set_pos(vertex, last_segment.end);
                set_modified_first_ctrl(vertex, last_segment);
                set_second_ctrl(vertex, Vector2f::zero());
                vertices.emplace_back(std::move(vertex));
            }

            // batch // TODO: combine batches with same type & info -- that's what batches are there for
            batches.emplace_back(Batch{std::move(call.info), narrow_cast<uint>(index_offset),
                                       narrow_cast<int>(indices.size() - index_offset)});
        }

        /// @brief Parse a convex fill call.
        void operator()(const ConvexFillCall& /*call*/) const {}

        /// @brief Parse a concave fill call.
        void operator()(const ConcaveFillCall& /*call*/) const {}
    };

    // function begins here ////////////////////////////////////////////////////////////////////////////////////////////

    const auto vao_guard = VaoBindGuard(m_vao_id);

    // TODO: estimate size of targets when adding new calls?
    std::vector<PlotVertexArray::Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Batch> batches;
    for (const Call& call : m_calls) {
        std::visit(Parser{vertices, indices, batches}, call);
    }
    m_calls.clear();

    static_cast<PlotVertexArray*>(m_vertices.get())->update(std::move(vertices));
    static_cast<PlotIndexArray*>(m_indices.get())->update(std::move(indices));
    std::swap(m_batches, batches);
}

void Plotter::render()
{
    /// @brief Render various batch types.
    struct Renderer {

        // fields ----------------------------------------------------------------------------------------------------//

        Pipeline& pipeline;

        /// @brief Batch target.
        const Batch& batch;

        // methods ---------------------------------------------------------------------------------------------------//

        /// @brief Draw a stroke.
        void operator()(const StrokeInfo& stroke) const
        {
            pipeline.tesselation_shader()->set_uniform("patch_type", 3);

            // TODO: stroke_width less than 1 should set a uniform that fades the line out and line widths of zero
            // should be ignored
            pipeline.tesselation_shader()->set_uniform("stroke_width", stroke.width);

            gl_check(glDrawElements(GL_PATCHES, static_cast<GLsizei>(batch.size), g_index_type,
                                    gl_buffer_offset(batch.offset * sizeof(PlotIndexArray::index_t))));
        }

        /// @brief Draw convex shape.
        void operator()(const ConvexFillInfo& /*call*/) const {}

        /// @brief Draw a concave shape.
        void operator()(const ConcaveFillInfo& /*call*/) const {}
    };

    // function begins here ////////////////////////////////////////////////////////////////////////////////////////////

    if (m_indices->is_empty()) {
        return;
    }
    const auto vao_guard = VaoBindGuard(m_vao_id);

    m_graphics_context.bind_pipeline(m_pipeline);

    gl_check(glPatchParameteri(GL_PATCH_VERTICES, 2));

    // pass the shader uniforms
    // TODO: get the window size from the graphics context?
    const TesselationShaderPtr& tess_shader = m_pipeline->tesselation_shader();
    const Matrix4f perspective              = Matrix4f::orthographic(0.f, 800.f, 0.f, 800.f, 0.f, 10000.f);
    tess_shader->set_uniform("projection", perspective);

    for (const Batch& batch : m_batches) {
        std::visit(Renderer{*m_pipeline.get(), batch}, batch.info);
    }

    m_graphics_context.unbind_pipeline();
}

} // namespace notf
