#pragma once

#include "app/forwards.hpp"
#include "common/bezier.hpp"
#include "common/polygon.hpp"
#include "common/variant.hpp"
#include "graphics/renderer/plotter.hpp"

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
    /// Access types.
    template<class T>
    using Access = access::_WidgetDesign<T>;

    /// Paint to use in the next fill / stroke / write.
    using Paint = Plotter::Paint;

    // ========================================================================
private:
    /// Member type detector to differentiate Commands with a `Data` type from those without.
    NOTF_HAS_MEMBER_TYPE_DETECTOR(Data);

public:
    /// Copy the current Design state and push it on the stack.
    struct PushStateCommand {};

    /// Remove the current Design state and restore the previous one.
    struct PopStateCommand {};

    /// Reset the transformation of the Path space.
    struct ResetTransformCommand {};

    /// Translates the Path space.
    struct TranslationCommand {
        Vector2f pos;
    };

    /// Rotates the Path space in the screen plane around its origin in radians.
    struct RotationCommand {
        float angle;
    };

    /// Changes the stroke width of the current Paint.
    struct SetStrokeWidthCommand {
        float stroke_width;
    };

    /// Sets the current Path of the Design to the given Polygon.
    /// Note that the Polgyon will still be transformed by the Path transformation before drawn on screen.
    struct SetPolygonPathCommand {
        struct Data {
            Polygonf polygon;
        };
        std::unique_ptr<Data> data;
    };

    /// Sets the current Path of the Design to the given Spline.
    /// Note that the Spline will still be transformed by the Path transformation before drawn on screen.
    struct SetSplinePathCommand {
        struct Data {
            CubicBezier2f spline;
        };
        std::unique_ptr<Data> data;
    };

    /// Writes the given text on screen using the current Path transform at the baseline start point.
    struct WriteCommand {
        struct Data {
            std::string text;
            FontPtr font;
        };
        std::unique_ptr<Data> data;
    };

    /// Fill the current Path using the current Paint.
    struct FillCommand {};

    /// Strokes the current Path using the current Paint.
    struct StrokeCommand {};

    using Command = notf::variant<PushStateCommand, PopStateCommand, ResetTransformCommand, TranslationCommand,
                                  RotationCommand, SetStrokeWidthCommand, SetPolygonPathCommand, SetSplinePathCommand,
                                  WriteCommand, FillCommand, StrokeCommand>;
    static_assert(sizeof(Command) == (sizeof(std::unique_ptr<void>) * 2),
                  "Make sure to wrap supplementary data of your Command type in a unique_ptr<>, "
                  "so it doesn't inflate the size of the Command variant");
    static_assert(has_member_type_Data<WriteCommand>::value && !has_member_type_Data<FillCommand>::value,
                  "Check NOTF_HAS_MEMBER_TYPE_DETECTOR, "
                  "it no longer differentiates between Commands with a `Data` type and those without");

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Pushes a new Command onto the buffer.
    /// @param args Arguments used to initialize the data of the given Command.
    template<class T, class... Args>
    notf::enable_if_t<has_member_type_Data<T>::value, void> add_command(Args&&... args)
    {
        m_buffer.emplace_back(T{std::make_unique<aggregate_adapter<typename T::Data>>(std::forward<Args>(args)...)});
    }
    template<class T, class... Args>
    notf::enable_if_t<!has_member_type_Data<T>::value, void> add_command(Args&&... args)
    {
        m_buffer.emplace_back(T{std::forward<Args>(args)...});
    }

    /// Clears the content of the buffer and the vault.
    void reset() { m_buffer.clear(); }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Buffer of untyped Command instances.
    std::vector<Command> m_buffer;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_WidgetDesign<Painterpreter> {
    friend class notf::Painterpreter;

    /// The Design's buffer of untyped Command instances.
    static const std::vector<WidgetDesign::Command>& get_buffer(const WidgetDesign& design) { return design.m_buffer; }
};

NOTF_CLOSE_NAMESPACE
