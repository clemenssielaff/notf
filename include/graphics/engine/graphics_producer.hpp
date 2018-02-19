#pragma once

#include "./render_manager.hpp"
#include "common/forwards.hpp"
#include "common/id.hpp"
#include "utils/make_smart_enabler.hpp"

namespace notf {

namespace detail {
class RenderDag;
} // namespace detail

// ===================================================================================================================//

/// Property id type.
using GraphicsProducerId = IdType<GraphicsProducer, size_t>;

// ===================================================================================================================//

/// Base class for GraphicsProducer.
class GraphicsProducer {

    // types ---------------------------------------------------------------------------------------------------------//
protected:
    /// Token object to make sure that object instances can only be created by a call to `_create`.
    class Token {
        friend class GraphicsProducer;
        Token() = default;
    };

    /// Instead of a simple flag, the "dirtyness" of a GraphicsProducer is a little more nuanced.
    /// While the producer is allowed to decide that it is no longer "programatically" dirty after (for example) all of
    /// its inputs have been set back as they were when the last frame was rendered, the only way to clean "user"
    /// dirtyness is by calling `render`.
    enum class DirtynessLevel {
        CLEAN,        ///< not dirty
        PROGRAMMATIC, ///< dirty by "choice"
        USER,         ///< dirty because the user requests a redraw
    };

    /// GraphicsProducer subclasses must identify themselves to the RenderManager, so it can try to minimize graphics
    /// state changes when rendering multiple GraphicsProducers in sequence.
    enum class Type {
        PLOTTER,
        PROCEDURAL,
    };

public:
    /// Private access type template.
    /// Used for finer grained friend control and is compiled away completely (if you should worry).
    template<typename T>
    class Private {
        static_assert(always_false_t<T>{}, "No Private access for requested type");
    };

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// @param token    Token to make sure that the instance can only be created by a call to `_create`.
    GraphicsProducer(const Token&) : m_id(_next_id()) {}

    /// Factory method for this type and all of its children.
    /// You need to call this function from your own factory in order to get a Token instance.
    /// This method will in turn register the new instance with the RenderManager.
    template<typename T, typename... Ts>
    static std::shared_ptr<T> _create(RenderManagerPtr& render_manager, Ts&&... args)
    {
        static_assert(std::is_base_of<GraphicsProducer, T>::value, "GraphicsProducer::_create can only create "
                                                                   "instances of subclasses of GraphicsProducer");
        const Token token;
#ifdef NOTF_DEBUG
        auto result = std::shared_ptr<T>(new T(token, render_manager, std::forward<Ts>(args)...));
#else
        auto result = std::make_shared<make_shared_enabler<T>>(token, render_manager, std::forward<Ts>(args)...);
#endif
        RenderManager::Private<GraphicsProducer>(*render_manager).register_new(result);
        return result;
    }

public:
    /// Destructor.
    virtual ~GraphicsProducer();

    /// Id of this GraphicsProducer.
    GraphicsProducerId id() const noexcept { return m_id; }

    /// Human-readable name of this GraphicsProducer.
    const std::string& name() const { return m_name; }

    /// Whether the GraphicsProducer is currently dirty or not.
    bool is_dirty() const { return m_dirtyness != DirtynessLevel::CLEAN; }

    /// Makes the GraphicsProducer dirty and requires a call to `render` to clean again.
    /// Do not (necessarily) call this method from subclasses, as you'll have finer-grained control over the dirtyness
    /// level with the protected `_set_dirty` and _set_clean` methods.
    void set_dirty() { m_dirtyness = DirtynessLevel::USER; }

    /// Unique type of this GraphicsProducer subclass.
    virtual Type render_type() const = 0;

    /// Report all RenderTargets that this Producer depends on.
    /// The default implementation does nothing, it is the subclass' responsibility to add *all* of its dependencies.
    /// @param dependencies     [out] Dependencies to add yours to.
    virtual void report_dependencies(detail::RenderDag& /*dependencies*/) const {}

private:
    /// Renders the GraphicsProducer, if it is dirty.
    /// Is only callable from the RenderManager.
    void render() const;

protected:
    /// The dirtyness level of this GraphicsProducer.
    DirtynessLevel _dirtyness() const { return m_dirtyness; }

    /// Sets the GraphicsProducer "programmatically" dirty - but only if it is not already "user" dirty.
    void _set_dirty()
    {
        if (m_dirtyness == DirtynessLevel::CLEAN) {
            m_dirtyness = DirtynessLevel::PROGRAMMATIC;
        }
    }

    /// Sets the GraphicsProducer clean, but only if it was just "programmatically" dirty.
    /// This allows the producer finer-grained control over its own dirtyness.
    /// For example, if a producer has a single boolean input, it may decide that it is dirty whenever the input
    /// changed, but clean again when it is set back before a frame was actually rendered.
    /// If however anybody calls `set_dirty` via the public interface, the GraphicsProducer cannot "clean" itself
    /// without invoking "render".
    void _set_clean()
    {
        if (m_dirtyness == DirtynessLevel::PROGRAMMATIC) {
            m_dirtyness = DirtynessLevel::CLEAN;
        }
    }

private:
    /// Subclass-defined implementation of the GraphicsProducer's rendering.
    virtual void _render() const = 0;

    /// Generate the next available GraphicsProducerId.
    static GraphicsProducerId _next_id();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// GraphicsProducer id.
    const GraphicsProducerId m_id;

    /// Human readable name of this GraphicsProducer.
    const std::string m_name;

    /// Producer's dirtyness level.
    mutable DirtynessLevel m_dirtyness;
};

// ===================================================================================================================//

template<>
class GraphicsProducer::Private<RenderManager> {
    friend class RenderManager;

    /// Constructor.
    /// @param producer     The GraphicsProducer to access.
    Private(GraphicsProducer& producer) : m_producer(producer) {}

    /// Renders the GraphicsProducer, if it is dirty.
    void render() { m_producer.render(); }

    /// The GraphicsProducer to access.
    GraphicsProducer& m_producer;
};

} // namespace notf
