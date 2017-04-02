#include "graphics/cell/commands.hpp"

namespace notf {

/* The Command buffer is a tightly packed vector of `Command` subclasses.
 * Internally, everything is a float (or to be more precise, a Command::command_t) - even the Command type identifier,
 * which is just static cast to a command_t.
 * In order to map subclasses of `Command` into the buffer, we require the `Command` subclasses to be unaligned, meaning
 * they contain no padding.
 * This works fine on my machine (...) but just to be sure, here's a block of static asserts that fail at compile time
 * should your particular compiler behave differently.
 * If any of them fail, you might want to look into compiler-specific pragmas or flags in order to stop it from adding
 * padding bytes.
 */

static_assert(sizeof(PainterCommand::Type) == sizeof(PainterCommand::command_t),
              "The underlying type of Command::Type has a different size than Command::command_t. "
              "Adjust the underlying type of the Command::Type enum to fit your particular system.");

#define ASSERT_COMMAND_SIZE(T, S) static_assert(        \
    sizeof(T) == sizeof(PainterCommand::command_t) * S, \
    "Error: padding detected in Command `" #T "` - the Painterpreter requires its Commands to be tightly packed");

ASSERT_COMMAND_SIZE(PushStateCommand, 1);
ASSERT_COMMAND_SIZE(PopStateCommand, 1);
ASSERT_COMMAND_SIZE(BeginCommand, 1);
ASSERT_COMMAND_SIZE(SetWindingCommand, 2);
ASSERT_COMMAND_SIZE(CloseCommand, 1);
ASSERT_COMMAND_SIZE(MoveCommand, 3);
ASSERT_COMMAND_SIZE(LineCommand, 3);
ASSERT_COMMAND_SIZE(BezierCommand, 7);
ASSERT_COMMAND_SIZE(FillCommand, 1);
ASSERT_COMMAND_SIZE(StrokeCommand, 1);
ASSERT_COMMAND_SIZE(SetXformCommand, 7);
ASSERT_COMMAND_SIZE(ResetXformCommand, 1);
ASSERT_COMMAND_SIZE(TransformCommand, 7);
ASSERT_COMMAND_SIZE(TranslationCommand, 3);
ASSERT_COMMAND_SIZE(RotationCommand, 2);
ASSERT_COMMAND_SIZE(SetScissorCommand, 9);
ASSERT_COMMAND_SIZE(ResetScissorCommand, 1);
ASSERT_COMMAND_SIZE(FillColorCommand, 5);
ASSERT_COMMAND_SIZE(FillPaintCommand, 24);
ASSERT_COMMAND_SIZE(StrokeColorCommand, 5);
ASSERT_COMMAND_SIZE(StrokePaintCommand, 24);
ASSERT_COMMAND_SIZE(StrokeWidthCommand, 2);
ASSERT_COMMAND_SIZE(BlendModeCommand, 2);
ASSERT_COMMAND_SIZE(SetAlphaCommand, 2);
ASSERT_COMMAND_SIZE(MiterLimitCommand, 2);
ASSERT_COMMAND_SIZE(LineCapCommand, 2);
ASSERT_COMMAND_SIZE(LineJoinCommand, 2);

} // namespace notf
