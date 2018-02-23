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

    /// Report all RenderTargets that this Producer depends on.
    /// The default implementation does nothing, it is the subclass' responsibility to add *all* of its dependencies.
    /// @param dependencies     [out] Dependencies to add yours to.
    virtual void report_dependencies(detail::RenderDag& /*dependencies*/) const {}

private:
    /// Renders the GraphicsProducer, if it is dirty.
    /// Is only callable from the RenderManager.
    void render() const;

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
};

// ===================================================================================================================//

template<>
class GraphicsProducer::Private<Layer> {
    friend class Layer;

    /// Constructor.
    /// @param producer     The GraphicsProducer to access.
    Private(GraphicsProducer& producer) : m_producer(producer) {}

    /// Renders the GraphicsProducer, if it is dirty.
    void render() { m_producer.render(); }

    /// The GraphicsProducer to access.
    GraphicsProducer& m_producer;
};

template<>
class GraphicsProducer::Private<RenderTarget> {
    friend class RenderTarget;

    /// Constructor.
    /// @param producer     The GraphicsProducer to access.
    Private(GraphicsProducer& producer) : m_producer(producer) {}

    /// Renders the GraphicsProducer, if it is dirty.
    void render() { m_producer.render(); }

    /// The GraphicsProducer to access.
    GraphicsProducer& m_producer;
};

} // namespace notf
