#include "graphics/engine/plate.hpp"

#include "graphics/engine/render_target.hpp"

namespace notf {

void Plate::render()
{
    m_render_target->clean();
}

} // namespace notf
