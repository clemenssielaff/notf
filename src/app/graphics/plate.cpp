#include "app/graphics/plate.hpp"

#include "app/graphics/render_target.hpp"

namespace notf {

void Plate::render()
{
    m_render_target->clean();
}

} // namespace notf
