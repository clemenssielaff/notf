#pragma once

#include <any>
#include <variant>

#include "notf/meta/smart_ptr.hpp"

#include "notf/common/bezier.hpp"
#include "notf/common/polygon.hpp"

#include "notf/graphic/renderer/plotter.hpp"

#include "notf/app/fwd.hpp"

NOTF_OPEN_NAMESPACE

// widget design ==================================================================================================== //

class WidgetDesign {

    friend Accessor<WidgetDesign, Painterpreter>;

    // types ----------------------------------------------------------------------------------- //
private:
    struct DataMemberDetector {
        template<class T>
        static constexpr auto has_data_member() {
            if constexpr (decltype(_has_data_member<T>(declval<T>()))::value) {
                return std::true_type{};
            } else {
                return std::false_type{};
            }
        }

    private:
        template<class T>
        static constexpr auto _has_data_member(const T&) -> decltype(declval<typename T::Data>(), std::true_type{});
        template<class>
        static constexpr auto _has_data_member(...) -> std::false_type;
    };

    /// Constexpr boolean that is true only if T has a member type "Data".
    template<class T>
    static constexpr bool has_data_member_v = decltype(DataMemberDetector::has_data_member<T>())::value;

public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(WidgetDesign);

    /// Paint to use in the next fill / stroke / write.
    using Paint = Plotter::Paint;

    /// Id identifying a Path in the Design.
    using PathId = Plotter::PathId;

    // commands ---------------------------------------------------------------
public:
    /// Copy the current Design state and push it on the stack.
    struct PushStateCommand {};

    /// Remove the current Design state and restore the previous one.
    struct PopStateCommand {};

    /// Updates the Path space.
    struct SetTransformationCommand {
        struct Data {
            M3f transformation;
        };
        std::unique_ptr<Data> data;
    };

    /// Changes the stroke width of the current Paint.
    struct SetStrokeWidthCommand {
        float stroke_width;
    };

    /// Changes the current Font.
    struct SetFontCommand {
        struct Data {
            FontPtr font;
        };
        std::unique_ptr<Data> data;
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

    /// Make an existing Path current.
    struct SetPathIndexCommand {
        PathId index;
    };

    /// Writes the given text on screen using the current Path transform at the baseline start point.
    struct WriteCommand {
        struct Data {
            std::string text;
        };
        std::unique_ptr<Data> data;
    };

    /// Fill the current Path using the current Paint.
    struct FillCommand {};

    /// Strokes the current Path using the current Paint.
    struct StrokeCommand {};

    using Command = std::variant<PushStateCommand, PopStateCommand, SetTransformationCommand, SetStrokeWidthCommand,
                                 SetFontCommand, SetPolygonPathCommand, SetSplinePathCommand, SetPathIndexCommand,
                                 WriteCommand, FillCommand, StrokeCommand>;
    static_assert(sizeof(Command) == (sizeof(std::unique_ptr<void>) * 2),
                  "Make sure to wrap supplementary data of your Command type in a unique_ptr<>, "
                  "so it doesn't inflate the size of the Command variant");

    // methods --------------------------------------------------------------------------------- //
public:
    /// @{
    /// Pushes a new Command onto the buffer.
    /// @param args Arguments used to initialize the data of the given Command.
    template<class T, class... Args>
    std::enable_if_t<has_data_member_v<T>> add_command(Args&&... args) {
        m_buffer.emplace_back(T{make_unique_aggregate<typename T::Data>(std::forward<Args>(args)...)});
    }
    template<class T, class... Args>
    std::enable_if_t<!has_data_member_v<T>> add_command(Args&&... args) {
        m_buffer.emplace_back(T{std::forward<Args>(args)...});
    }
    /// @}

    /// Clears the content of the buffer and the vault.
    void reset() { m_buffer.clear(); }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Buffer of untyped Command instances.
    std::vector<Command> m_buffer;
};

// accessors ----------------------------------------------------------------------------------- //

template<>
class Accessor<WidgetDesign, Painterpreter> {
    friend Painterpreter;

    /// The Design's buffer of untyped Command instances.
    static const std::vector<WidgetDesign::Command>& get_buffer(const WidgetDesign& design) { return design.m_buffer; }
};

NOTF_CLOSE_NAMESPACE
