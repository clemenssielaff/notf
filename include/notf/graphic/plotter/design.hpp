#pragma once

#include "notf/meta/concept.hpp"

#include "notf/common/variant.hpp"

#include "notf/graphic/plotter/plotter.hpp"

NOTF_OPEN_NAMESPACE

// widget design ==================================================================================================== //

class PlotterDesign {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Resets the Plotter's State to its default values.
    struct ResetState {};

    struct PushState {};

    struct PopState {};

    struct SetXform {
        struct Data {
            M3f transformation;
        };
        std::unique_ptr<Data> data;
    };

    struct SetPaint {
        struct Data {
            Plotter::Paint paint;
        };
        std::unique_ptr<Data> data;
    };

    struct SetPath {
        struct Data {
            Path2 path;
        };
        std::unique_ptr<Data> data;
    };

    struct SetStencil {
        struct Data {
            Path2 path;
        };
        std::unique_ptr<Data> data;
    };

    struct SetFont {
        struct Data {
            FontPtr font;
        };
        std::unique_ptr<Data> data;
    };

    struct SetAlpha {
        float alpha;
    };

    struct SetStrokeWidth {
        float stroke_width;
    };

    struct SetBlendMode {
        BlendMode mode;
    };

    struct SetLineCap {
        Plotter::LineCap cap;
    };

    struct SetLineJoin {
        Plotter::LineJoin join;
    };

    struct Fill {};

    struct Stroke {};

    struct Write {
        struct Data {
            std::string text;
        };
        std::unique_ptr<Data> data;
    };

    using Command = std::variant<ResetState, PushState, PopState, SetXform, SetPaint, SetPath, SetStencil, SetFont,
                                 SetAlpha, SetStrokeWidth, SetBlendMode, SetLineCap, SetLineJoin, Fill, Stroke, Write>;

private:
    NOTF_CREATE_FIELD_DETECTOR(data);
    static_assert(sizeof(Command) == (sizeof(std::unique_ptr<void>) * 2),
                  "Make sure to wrap supplementary data of your Command type in a unique_ptr<>, "
                  "so it doesn't inflate the size of the Command variant");

    // methods --------------------------------------------------------------------------------- //
public:
    /// @{
    /// Pushes a new Command onto the buffer.
    /// @param args Arguments used to initialize the data of the given Command.
    template<class T, class... Args>
    std::enable_if_t<_has_data_v<T>> add_command(Args&&... args) {
        m_buffer.emplace_back(T{make_unique_aggregate<typename T::Data>(std::forward<Args>(args)...)});
    }
    template<class T, class... Args>
    std::enable_if_t<!_has_data_v<T>> add_command(Args&&... args) {
        m_buffer.emplace_back(T{std::forward<Args>(args)...});
    }
    /// @}

    /// The Design's buffer of untyped Command instances.
    const std::vector<Command>& get_buffer() const { return m_buffer; }

    /// Clears the content of the buffer and the vault.
    void reset() { m_buffer.clear(); }

    /// Finishes and performs basic optimization on the buffer.
    /// The performed optimizations do not affect the Design, only remove unnecessary Commands.
    /// Do not add any more Commands to the Design after calling the method before resetting it first.
    void complete();

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Buffer of untyped Command instances.
    std::vector<Command> m_buffer;
};

NOTF_CLOSE_NAMESPACE
