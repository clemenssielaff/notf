#include "cel/stroker.hpp"

#include "common/matrix4.hpp"
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

} // namespace

namespace notf {

Stroker::Stroker(GraphicsContextPtr& context)
    : m_graphics_context(*context), m_pipeline(), m_vao_id(0), m_vertices(), m_indices()
{
    // vao
    gl_check(glGenVertexArrays(1, &m_vao_id));
    if (!m_vao_id) {
        throw_runtime_error("Failed to allocate the Stroker VAO");
    }
    gl_check(glBindVertexArray(m_vao_id));

    { // pipeline
        const std::string vertex_src  = load_file("/home/clemens/code/notf/res/shaders/line.vert");
        VertexShaderPtr vertex_shader = VertexShader::build(context, "line.vert", vertex_src);

        const std::string tess_src       = load_file("/home/clemens/code/notf/res/shaders/line.tess");
        const std::string eval_src       = load_file("/home/clemens/code/notf/res/shaders/line.eval");
        TesselationShaderPtr tess_shader = TesselationShader::build(context, "line.tess", tess_src.c_str(), eval_src);

        const std::string frag_src    = load_file("/home/clemens/code/notf/res/shaders/line.frag");
        FragmentShaderPtr frag_shader = FragmentShader::build(context, "line.frag", frag_src);

        m_pipeline = Pipeline::create(context, vertex_shader, tess_shader, frag_shader);
    }

    { // vertices
        VertexArrayType::Args vertex_args;
        vertex_args.usage = GL_DYNAMIC_DRAW;
        m_vertices        = std::make_unique<LineVertexArray>(std::move(vertex_args));

        LineVertexArray* line_vertices = static_cast<LineVertexArray*>(m_vertices.get());
        line_vertices->init();
        line_vertices->update({
            //        {Vector2f{100, 070}, Vector2f{-1, +3}.normalize(), Vector2f{1, +3}.normalize()},
            //        {Vector2f{180, 310}, Vector2f{-1, -3}.normalize(), Vector2f{1, -3}.normalize()},
            //        {Vector2f{260, 070}, Vector2f{-1, +3}.normalize(), Vector2f{1, +3}.normalize()},
            //        {Vector2f{340, 310}, Vector2f{-1, -3}.normalize(), Vector2f{1, -3}.normalize()},
            {Vector2f{100, 100}, Vector2f{-400, 0}, Vector2f{400, 0}},
            {Vector2f{700, 700}, Vector2f{-400, 0}, Vector2f{400, 0}},
        });
    }
    { // indices
        m_indices = std::make_unique<LineIndexArray>();
        m_indices->init();

        LineIndexArray* line_indices = static_cast<LineIndexArray*>(m_indices.get());
        line_indices->update({
            0, 1,
            //        1,
            //        2,
            //        2,
            //        3,
        });
    }
    gl_check(glBindVertexArray(0));

    // TODO: CONTINUE HERE by creating the stroker
    /// Die idee ist ja, dass man "lines" in den Stroker gibt und der dann dafuer sorgt, dass alles klappt
    /// Das alles klappt bedeutet, dass
    /// 1.  Alle Ctrls haben magnitude + 1. WIr brauchen fuer JEDEN CTRL POINT eine Richtung! Aber gleichzeitig
    ///     muss es auch erlaubt bleiben einen CTRL point (0,0) zu haben. Deshalb nehmen wir im Shader den vec2,
    ///     normalisieren ihn, ziehen 1.0 von der magnitude ab und benutzen das Ergebnis als actual ctrl point
    /// 2. Zwischen zwei linien brauchen wir einen Joint. Ein Joint zeichnet sich dadurch aus, dass zweimal der
    ///     selbe Vertex gegeben wird. Anhand der Richtungen (die wir garantiert haben (siehe 1)) kann man dann den
    ///     joint errechnen. Ob das dann ein Miter, Bevel or Round joint wird bestimmt sich durch uniforms
}

Stroker::~Stroker() { gl_check(glDeleteVertexArrays(1, &m_vao_id)); }

void Stroker::render()
{
    gl_check(glBindVertexArray(m_vao_id));
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
    gl_check(glBindVertexArray(0));
}

} // namespace notf
