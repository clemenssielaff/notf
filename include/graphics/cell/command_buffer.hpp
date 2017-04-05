#pragma once

#include <array>
#include <stdint.h>
#include <vector>

#include "utils/enum_to_number.hpp"
#include "utils/sfinae.hpp"

namespace notf {

struct PainterCommand {
    using command_t = float; // TODO: use _Vector2<command_t> instead of Vector2f?

    enum Type : uint32_t {
        PUSH_STATE,
        POP_STATE,
        BEGIN_PATH,
        SET_WINDING,
        CLOSE,
        MOVE,
        LINE,
        BEZIER,
        FILL,
        STROKE,
        SET_XFORM,
        RESET_XFORM,
        TRANSFORM,
        TRANSLATE,
        ROTATE,
        SET_SCISSOR,
        RESET_SCISSOR,
        SET_FILL_COLOR,
        SET_FILL_PAINT,
        SET_STROKE_COLOR,
        SET_STROKE_PAINT,
        SET_STROKE_WIDTH,
        SET_BLEND_MODE,
        SET_ALPHA,
        SET_MITER_LIMIT,
        SET_LINE_CAP,
        SET_LINE_JOIN,
    };

public: // static methods
    /** Bitwise conversion from a command_t to a Type enum. */
    static constexpr Type command_t_to_type(command_t command)
    {
        union convert {
            command_t f;
            uint32_t i;
        };
        return static_cast<Type>(convert{command}.i);
    }

    /** Bitwise conversion from a Type enum to a command_t. */
    static constexpr command_t type_to_command_t(Type type)
    {
        union convert {
            uint32_t i;
            command_t f;
        };
        return convert{to_number(type)}.f;
    }

protected: // method
    PainterCommand(Type type)
        : type(type_to_command_t(type)) {}

public: // field
    const command_t type;
};

/**********************************************************************************************************************/

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

class PainterCommandBuffer : public std::vector<PainterCommand::command_t> {

public: // methods
    /** Pushes a new Command onto the buffer. */
    template <typename Subcommand, ENABLE_IF_SUBCLASS(Subcommand, PainterCommand)>
    inline void add_command(Subcommand command)
    {
        using array_t      = std::array<PainterCommand::command_t, sizeof(command) / sizeof(PainterCommand::command_t)>;
        array_t& raw_array = *(reinterpret_cast<array_t*>(&command));
        insert(end(), std::make_move_iterator(std::begin(raw_array)), std::make_move_iterator(std::end(raw_array)));
    }
};

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

/**********************************************************************************************************************/

/** Convenience function go get the size of a given Subcommand type in units of PainterCommand::command_t. */
template <typename Subcommand, ENABLE_IF_SUBCLASS(Subcommand, PainterCommand)>
constexpr auto command_size()
{
    static_assert(sizeof(Subcommand) % sizeof(PainterCommand::command_t) == 0,
                  "Cannot determine the exact size of Command in units of Command::command_t");
    return sizeof(Subcommand) / sizeof(PainterCommand::command_t);
}

/** Creates a const reference of a specific Subcommand that maps into the given PainterCommand buffer. */
template <typename Subcommand, ENABLE_IF_SUBCLASS(Subcommand, PainterCommand)>
constexpr const Subcommand& map_command(const std::vector<PainterCommand::command_t>& buffer, size_t index)
{
    return *(reinterpret_cast<const Subcommand*>(&buffer[index]));
}

} // namespace notf