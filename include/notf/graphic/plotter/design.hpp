#pragma once

#include "notf/meta/concept.hpp"

#include "notf/common/variant.hpp"

#include "notf/graphic/plotter/plotter.hpp"

NOTF_OPEN_NAMESPACE

// widget design ==================================================================================================== //

class PlotterDesign {

    friend Accessor<PlotterDesign, Painter>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(PlotterDesign);

    // commands ---------------------------------------------------------------

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
            Path2Ptr path;
        };
        std::unique_ptr<Data> data;
    };
    struct SetClip {
        struct Data {
            Aabrf clip;
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
        Plotter::CapStyle cap;
    };
    struct SetLineJoin {
        Plotter::JointStyle join;
    };
    struct Fill {};
    struct Stroke {};
    struct Write {
        struct Data {
            std::string text;
        };
        std::unique_ptr<Data> data;
    };
    using Command = std::variant<ResetState, PushState, PopState, SetXform, SetPaint, SetPath, SetClip, SetFont,
                                 SetAlpha, SetStrokeWidth, SetBlendMode, SetLineCap, SetLineJoin, Fill, Stroke, Write>;

private:
    NOTF_CREATE_FIELD_DETECTOR(data);
    static_assert(sizeof(Command) == (sizeof(std::unique_ptr<void>) * 2),
                  "Make sure to wrap supplementary data of your Command type in a unique_ptr<>, "
                  "so it doesn't inflate the size of the Command variant");

    // methods --------------------------------------------------------------------------------- //
public:
    /// Whether the Design is dirty or not.
    bool is_dirty() const { return m_is_dirty.load(std::memory_order_acquire); }

    /// Marks this Design as dirty.
    void set_dirty() { m_is_dirty.store(true, std::memory_order_release); }

    /// The Design's buffer of untyped Command instances.
    const std::vector<Command>& get_buffer() const { return m_buffer; }

private:
    /// Clears the content of the buffer and the vault.
    void _reset() { m_buffer.clear(); }

    /// @{
    /// Pushes a new Command onto the buffer.
    /// @param args Arguments used to initialize the data of the given Command.
    template<class T, class... Args>
    std::enable_if_t<_has_data_v<T>> _add_command(Args&&... args) {
        m_buffer.emplace_back(T{make_unique_aggregate<typename T::Data>(std::forward<Args>(args)...)});
    }
    template<class T, class... Args>
    std::enable_if_t<!_has_data_v<T>> _add_command(Args&&... args) {
        m_buffer.emplace_back(T{std::forward<Args>(args)...});
    }
    /// @}

    /// Finishes and performs basic optimization on the buffer.
    /// The performed optimizations do not affect the Design, only remove unnecessary Commands.
    /// Do not add any more Commands to the Design after calling the method before resetting it first.
    void _complete();

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Buffer of untyped Command instances.
    std::vector<Command> m_buffer;

    /// Whether or not the Design needs to be re-parsed or not.
    std::atomic_bool m_is_dirty = false;
};

// accessors ======================================================================================================== //

template<>
class Accessor<PlotterDesign, Painter> {
    friend Painter;

    static void reset(PlotterDesign& design) { design._reset(); }

    template<class T, class... Args>
    static void add_command(PlotterDesign& design, Args&&... args) {
        design._add_command<T>(std::forward<Args>(args)...);
    }

    static void complete(PlotterDesign& design) { design._complete(); }
};

NOTF_CLOSE_NAMESPACE
