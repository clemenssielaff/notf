#include "cel/stroker.hpp"

#include "common/vector2.hpp"
#include "core/opengl.hpp"
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

} // namespace

namespace notf {

Stroker::Stroker() : m_shader(), m_vertices()
{
    { // vertices
        VertexArrayType::Args vertex_args;
        vertex_args.usage = GL_DYNAMIC_DRAW;
        m_vertices        = std::make_unique<LineVertexArray>(std::move(vertex_args));

        LineVertexArray* line_vertices = static_cast<LineVertexArray*>(m_vertices.get());
        line_vertices->init();
        line_vertices->update({
            {Vector2f{100, 070}, Vector2f{-1, +3}.normalize(), Vector2f{1, +3}.normalize()},
            {Vector2f{180, 310}, Vector2f{-1, -3}.normalize(), Vector2f{1, -3}.normalize()},
            {Vector2f{260, 070}, Vector2f{-1, +3}.normalize(), Vector2f{1, +3}.normalize()},
            {Vector2f{340, 310}, Vector2f{-1, -3}.normalize(), Vector2f{1, -3}.normalize()},
        });

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
}

} // namespace notf
