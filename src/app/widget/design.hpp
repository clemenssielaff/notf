#pragma once

#include <array>
#include <set>
#include <vector>

#include "app/forwards.hpp"
#include "app/widget/clipping.hpp"
#include "app/widget/paint.hpp"
#include "common/assert.hpp"
#include "common/vector2.hpp"
#include "graphics/core/gl_modes.hpp"
#include "utils/bit_cast.hpp"

NOTF_OPEN_NAMESPACE

namespace access {
template<class>
class _WidgetDesign;
} // namespace access

// ================================================================================================================== //

class WidgetDesign {

    friend class access::_WidgetDesign<Painterpreter>;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    using Word = std::underlying_type_t<notf::byte>;

    enum class CommandType : Word {
        PUSH_STATE,
        POP_STATE,
        START_PATH,
        SET_WINDING,
        CLOSE_PATH,
        MOVE,
        LINE,
        BEZIER,
        FILL,
        STROKE,
        SET_TRANSFORM,
        RESET_TRANSFORM,
        TRANSFORM,
        TRANSLATE,
        ROTATE,
        SET_CLIPPING,
        RESET_CLIPPING,
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
        WRITE,
    };

private:
    /// Empty command baseclass.
    class CommandBase {};

    /// Command "header", providing the type of a Command subclass as its first member.
    template<CommandType TYPE>
    class Command : public CommandBase {

        // types ---------------------------------------------------------------------------------------------------- //
    public:
        /// The type of this Command.
        static constexpr CommandType type = TYPE;

        // methods -------------------------------------------------------------------------------------------------- //
    public:
        /// The type of this Command.
        CommandType get_type() { return m_type; }

        // fields --------------------------------------------------------------------------------------------------- //
    protected:
        /// The type of this Command.
        const CommandType m_type = TYPE;
    };

    // ========================================================================
public:
    /// Access types.
    template<class T>
    using Access = access::_WidgetDesign<T>;

    NOTF_DISABLE_WARNING("padded")

    /// Copy the current Design state and push it on the stack.
    struct PushStateCommand final : public Command<CommandType::PUSH_STATE> {};

    /// Remove the current Design state and restore the previous one.
    struct PopStateCommand final : public Command<CommandType::POP_STATE> {};

    /// Start a new path.
    struct BeginPathCommand final : public Command<CommandType::START_PATH> {};

    /// Command setting the winding direction for the following fill or stroke commands.
    struct SetWindingCommand final : public Command<CommandType::SET_WINDING> {
        SetWindingCommand() = default;
        constexpr explicit SetWindingCommand(Paint::Winding winding)
            : Command<CommandType::SET_WINDING>(), winding(winding)
        {}
        ~SetWindingCommand();
        Paint::Winding winding;
    };

    /// Close the current path.
    struct ClosePathCommand final : public Command<CommandType::CLOSE_PATH> {};

    /// Move the Painter's stylus without drawing a line.
    /// Finishes the current path (if one exists) and starts a new one.
    struct MoveCommand final : public Command<CommandType::MOVE> {
        MoveCommand() = default;
        constexpr explicit MoveCommand(Vector2f pos) : Command<CommandType::MOVE>(), pos(std::move(pos)) {}
        ~MoveCommand();
        Vector2f pos;
    };

    /// Draw a line from the current stylus position to the one given.
    struct LineCommand final : public Command<CommandType::LINE> {
        LineCommand() = default;
        constexpr explicit LineCommand(Vector2f pos) : Command<CommandType::LINE>(), pos(std::move(pos)) {}
        ~LineCommand();
        Vector2f pos;
    };

    /// Draw a bezier spline from the current stylus position.
    struct BezierCommand final : public Command<CommandType::BEZIER> {
        BezierCommand() = default;
        constexpr explicit BezierCommand(Vector2f ctrl1, Vector2f ctrl2, Vector2f end)
            : Command<CommandType::BEZIER>(), ctrl1(std::move(ctrl1)), ctrl2(std::move(ctrl2)), end(std::move(end))
        {}
        ~BezierCommand();
        Vector2f ctrl1;
        Vector2f ctrl2;
        Vector2f end;
    };

    /// Fill the current paths using the current Design state.
    struct FillCommand final : public Command<CommandType::FILL> {};

    /// Stroke the current paths using the current Design state.
    struct StrokeCommand final : public Command<CommandType::STROKE> {};

    /// Change the transformation of the current Design state.
    struct SetTransformCommand final : public Command<CommandType::SET_TRANSFORM> {
        SetTransformCommand() = default;
        constexpr explicit SetTransformCommand(Matrix3f xform)
            : Command<CommandType::SET_TRANSFORM>(), xform(std::move(xform))
        {}
        ~SetTransformCommand();
        Matrix3f xform;
    };

    /// Reset the transformation of the current Design state.
    struct ResetTransformCommand final : public Command<CommandType::RESET_TRANSFORM> {};

    /// Transform the current transformation of the current Design state.
    struct TransformCommand final : public Command<CommandType::TRANSFORM> {
        TransformCommand() = default;
        constexpr explicit TransformCommand(Matrix3f xform) : Command<CommandType::TRANSFORM>(), xform(std::move(xform))
        {}
        ~TransformCommand();
        Matrix3f xform;
    };

    /// Translate the transformation of the current Design state.
    struct TranslationCommand final : public Command<CommandType::TRANSLATE> {
        TranslationCommand() = default;
        constexpr explicit TranslationCommand(Vector2f delta)
            : Command<CommandType::TRANSLATE>(), delta(std::move(delta))
        {}
        ~TranslationCommand();
        Vector2f delta;
    };

    /// Add a rotation in radians to the transformation of the current Design state.
    struct RotationCommand final : public Command<CommandType::ROTATE> {
        RotationCommand() = default;
        constexpr explicit RotationCommand(float angle) : Command<CommandType::ROTATE>(), angle(angle) {}
        ~RotationCommand();
        float angle;
    };

    /// Set the clipping rect of the current Design state.
    struct SetClippingCommand final : public Command<CommandType::SET_CLIPPING> {
        SetClippingCommand() = default;
        explicit SetClippingCommand(Clipping clipping) : Command<CommandType::SET_CLIPPING>(), clipping(clipping) {}
        ~SetClippingCommand();
        Clipping clipping;
    };

    /// Reset the clipping rect of the current Design state.
    struct ResetClippingCommand final : public Command<CommandType::RESET_CLIPPING> {};

    /// Update the fill color of the current Design state.
    struct FillColorCommand final : public Command<CommandType::SET_FILL_COLOR> {
        FillColorCommand() = default;
        constexpr explicit FillColorCommand(Color color) : Command<CommandType::SET_FILL_COLOR>(), color(color) {}
        ~FillColorCommand();
        Color color;
    };

    /// Update the fill Paint of the current Design state.
    struct FillPaintCommand final : public Command<CommandType::SET_FILL_PAINT> {
        FillPaintCommand() = default;
        explicit FillPaintCommand(Paint paint) : Command<CommandType::SET_FILL_PAINT>(), paint(paint) {}
        ~FillPaintCommand();
        Paint paint;
    };

    /// Set the stroke Color of the current Design state.
    struct StrokeColorCommand final : public Command<CommandType::SET_STROKE_COLOR> {
        StrokeColorCommand() = default;
        constexpr explicit StrokeColorCommand(Color color) : Command<CommandType::SET_STROKE_COLOR>(), color(color) {}
        ~StrokeColorCommand();
        Color color;
    };

    /// Set the stroke Paint of the current Design state.
    struct StrokePaintCommand final : public Command<CommandType::SET_STROKE_PAINT> {
        StrokePaintCommand() = default;
        explicit StrokePaintCommand(Paint paint) : Command<CommandType::SET_STROKE_PAINT>(), paint(paint) {}
        ~StrokePaintCommand();
        Paint paint;
    };

    /// Set the stroke width of the current Design state.
    struct StrokeWidthCommand final : public Command<CommandType::SET_STROKE_WIDTH> {
        StrokeWidthCommand() = default;
        constexpr explicit StrokeWidthCommand(float stroke_width)
            : Command<CommandType::SET_STROKE_WIDTH>(), stroke_width(stroke_width)
        {}
        ~StrokeWidthCommand();
        float stroke_width;
    };

    /// Command to set the BlendMode of the current Design state.
    struct BlendModeCommand final : public Command<CommandType::SET_BLEND_MODE> {
        BlendModeCommand() = default;
        explicit BlendModeCommand(BlendMode blend_mode) : Command<CommandType::SET_BLEND_MODE>(), blend_mode(blend_mode)
        {}
        ~BlendModeCommand();
        BlendMode blend_mode;
    };

    /// Command to set the alpha of the current Design state.
    struct SetAlphaCommand final : public Command<CommandType::SET_ALPHA> {
        SetAlphaCommand() = default;
        constexpr explicit SetAlphaCommand(float alpha) : Command<CommandType::SET_ALPHA>(), alpha(alpha) {}
        ~SetAlphaCommand();
        float alpha;
    };

    /// Command to set the MiterLimit of the current Design state.
    struct MiterLimitCommand final : public Command<CommandType::SET_MITER_LIMIT> {
        MiterLimitCommand() = default;
        constexpr explicit MiterLimitCommand(float miter_limit)
            : Command<CommandType::SET_MITER_LIMIT>(), miter_limit(miter_limit)
        {}
        ~MiterLimitCommand();
        float miter_limit;
    };

    /// Command to set the LineCap of the current Design state.
    struct LineCapCommand final : public Command<CommandType::SET_LINE_CAP> {
        LineCapCommand() = default;
        constexpr explicit LineCapCommand(Paint::LineCap line_cap)
            : Command<CommandType::SET_LINE_CAP>(), line_cap(line_cap)
        {}
        ~LineCapCommand();
        Paint::LineCap line_cap;
    };

    /// Command to set the LineJoin of the current Design state.
    struct LineJoinCommand final : public Command<CommandType::SET_LINE_JOIN> {
        LineJoinCommand() = default;
        constexpr explicit LineJoinCommand(Paint::LineJoin line_join)
            : Command<CommandType::SET_LINE_JOIN>(), line_join(line_join)
        {}
        ~LineJoinCommand();
        Paint::LineJoin line_join;
    };

    /// Command to render the given text in the given font.
    struct WriteCommand final : public Command<CommandType::WRITE> {
        WriteCommand() = default;
        explicit WriteCommand(std::shared_ptr<std::string> text, std::shared_ptr<Font> font)
            : Command<CommandType::WRITE>(), text(std::move(text)), font(std::move(font))
        {}
        ~WriteCommand();
        std::shared_ptr<std::string> text;
        std::shared_ptr<Font> font;
    };

    NOTF_ENABLE_WARNINGS

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Pushes a new Command onto the buffer.
    template<typename COMMAND, typename = notf::enable_if_t<std::is_base_of<CommandBase, COMMAND>::value>>
    void add_command(const COMMAND& command)
    {
        using array_t = std::array<Word, sizeof(command) / sizeof(Word)>;
        array_t raw_array = bit_cast_unsafe<array_t>(command);
        std::move(std::begin(raw_array), std::end(raw_array), std::back_inserter(m_buffer));
    }

    /// Returns the size of a Command in words.
    template<typename COMMAND, typename = notf::enable_if_t<std::is_base_of<CommandBase, std::decay_t<COMMAND>>::value>>
    static constexpr size_t get_command_size()
    {
        return sizeof(COMMAND) / sizeof(Word);
    }

    /// Creates a const reference of a specific Command type that maps directly into the buffer.
    template<typename COMMAND, typename = notf::enable_if_t<std::is_base_of<CommandBase, COMMAND>::value>>
    COMMAND map_command(const size_t index) const
    {
        NOTF_ASSERT((index + get_command_size<COMMAND>()) < m_buffer.size());
        COMMAND command = bit_cast_unsafe<COMMAND>(m_buffer[index]);
        NOTF_ASSERT(command.get_type() == COMMAND::type);
        return command;
    }

    /// Clears the content of the buffer and the vault.
    void reset()
    {
        m_buffer.clear();
        m_vault.clear();
    }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Buffer of untyped Command instances.
    std::vector<Word> m_buffer;

    /// Shared pointers coresponding to serialized shared_ptrs in the Command buffer.
    /// Commands have to be a fixed size at compile time, yet not all arguments to paint calls are of fixed size.
    /// Images for example, or strings that are rendered as text.
    /// In order to store them savely inside a Command, we pack them into a shared_ptr<> and serialize the shared_ptr
    /// into the command buffer. At the same time, to make sure that the actual object is not destroyed, we put another
    /// (live) shared_ptr into the vault, where it looses its type. It is called a vault because it keeps stuff save
    /// inside and also, because once it's in there - you are not getting it out again. When the WidgetDesign is
    /// destroyed, it clears both the command buffer and the vault, freeing all previously allocated memory that has
    /// gone out of scope.
    std::set<std::shared_ptr<void>> m_vault;
};

template<>
inline void WidgetDesign::add_command(const FillPaintCommand& command)
{
    using array_t = std::array<Word, sizeof(command) / sizeof(Word)>;
    array_t raw_array = bit_cast_unsafe<array_t>(command);
    std::move(std::begin(raw_array), std::end(raw_array), std::back_inserter(m_buffer));
    m_vault.insert(command.paint.texture);
}

template<>
inline void WidgetDesign::add_command(const StrokePaintCommand& command)
{
    using array_t = std::array<Word, sizeof(command) / sizeof(Word)>;
    array_t raw_array = bit_cast_unsafe<array_t>(command);
    std::move(std::begin(raw_array), std::end(raw_array), std::back_inserter(m_buffer));
    m_vault.insert(command.paint.texture);
}

template<>
inline void WidgetDesign::add_command(const WriteCommand& command)
{
    using array_t = std::array<Word, sizeof(command) / sizeof(Word)>;
    array_t raw_array = bit_cast_unsafe<array_t>(command);
    std::move(std::begin(raw_array), std::end(raw_array), std::back_inserter(m_buffer));
    m_vault.insert(command.text);
    m_vault.insert(command.font);
}

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_WidgetDesign<Painterpreter> {
    friend class notf::Painterpreter;

    /// The Design's buffer of untyped Command instances.
    static const std::vector<WidgetDesign::Word>& get_buffer(const WidgetDesign& design) { return design.m_buffer; }
};

NOTF_CLOSE_NAMESPACE
