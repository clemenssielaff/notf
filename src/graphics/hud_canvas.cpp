#include "graphics/hud_canvas.hpp"

namespace { // anonymous
using namespace notf;
void transform_command_point(const Transform2& xform, std::vector<float>& commands, size_t index)
{
    assert(commands.size() >= index + 2);
    Vector2& point = *reinterpret_cast<Vector2*>(&commands[index]);
    point          = xform.transform(point);
}

} // namespace anonymous

namespace notf {

void HUDCanvas::_append_commands(std::vector<float>&& commands)
{
    if (commands.empty()) {
        return;
    }
    m_commands.reserve(m_commands.size() + commands.size());

    // extract the last position of the stylus
    Command command = static_cast<Command>(m_commands[0]);
    if (command != Command::WINDING && command != Command::CLOSE) {
        assert(commands.size() >= 3);
        m_pos = *reinterpret_cast<Vector2*>(&commands[commands.size() - 2]);
    }

    // commands operate in the context's current transformation space, but we need them in global space
    const Transform2& xform = _get_current_state().xform;
    for (size_t i = 0; i < commands.size();) {
        command = static_cast<Command>(m_commands[i]);
        switch (command) {

        case Command::MOVE:
        case Command::LINE: {
            transform_command_point(xform, commands, i + 1);
            i += 3;
        } break;

        case Command::BEZIER: {
            transform_command_point(xform, commands, i + 1);
            transform_command_point(xform, commands, i + 3);
            transform_command_point(xform, commands, i + 5);
            i += 7;
        } break;

        case Command::WINDING:
            i += 2;
            break;

        case Command::CLOSE:
            i += 1;
            break;

        default:
            assert(0);
        }
    }

    // finally, append the new commands to the existing ones
    std::move(commands.begin(), commands.end(), std::back_inserter(m_commands));
}

/**
 * Compile-time sanity check.
 */
static_assert(sizeof(HUDCanvas::Command) == sizeof(float),
              "Floats on your system don't seem be to be 32 bits wide. "
              "Adjust the type of the underlying type of CommandBuffer::Command to fit your particular system.");

} // namespace notf
