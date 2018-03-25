#pragma once

#include "app/ids.hpp"
#include "app/scene_manager.hpp"
#include "app/window.hpp"

NOTF_OPEN_NAMESPACE

// ===================================================================================================================//

/// Base class for GraphicsProducer.
///
/// We need to make sure that the GraphicsProducer is properly registered with its SceneManager. For that, we delegate
/// the construction of *all* GraphicsProducer instances to the baseclass. In order to make this work, subclasses need
/// to follow a certain structure:
///
/// First, they need to befriend their GraphicsProduer baseclass:
///
/// ```
///     class MyProducer : public GraphicsProducer {
///         friend class GraphicsProducer;
///     ...
/// ```
///
/// Next, they need to declare a *protected* constructor that has a const `Token` reference as first parameter and a
/// mutable SceneManagerPtr reference as the second:
///
/// ```
///     MyProducer(const Token& token, SceneManagerPtr& manager, bool myVeryOwnParameter, ...);
/// ```
///
/// The remaining parameters can be whatever.
///
/// The user creates instances via a factory method, that requires  the SceneManager as the first argument and
/// forwards all additional arguments to the subclass' constructor.
///
/// ```
///     static std::shared_ptr<MyProducer> create(SceneManagerPtr& manager, bool myVeryOwnParameter, ...)
///     {
///         return _create<MyProducer>(manager, myVeryOwnParameter, ...);
///     }
/// ```
///
/// And finally, you need to implement the pure virtual method `_render`.
///
class GraphicsProducer {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    NOTF_ACCESS_TYPES(Layer, RenderTarget)

protected:
    /// Token object to make sure that object instances can only be created by a call to `_create`.
    class Token {
        friend class GraphicsProducer;
        Token() {} // not "= default", otherwise you could construct a Token via `Token{}`.
    };

    // methods -------------------------------------------------------------------------------------------------------//
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE

    /// Constructor.
    /// @param token    Token to make sure that the instance can only be created by a call to `_create`.
    GraphicsProducer(const Token&) : m_id(_next_id()) {}

protected:
    /// Factory method for this type and all of its children.
    /// You need to call this function from your own factory in order to get a Token instance.
    /// This method will in turn register the new instance with the SceneManager.
    template<typename T, typename... Ts>
    static std::shared_ptr<T> _create(SceneManagerPtr& render_manager, Ts&&... args)
    {
        static_assert(std::is_base_of<GraphicsProducer, T>::value, "GraphicsProducer::_create can only create "
                                                                   "instances of GraphicsProducer subclasses");
        const Token token;
        auto result = NOTF_MAKE_SHARED_FROM_PRIVATE(T, token, render_manager, std::forward<Ts>(args)...);
        SceneManager::Access<GraphicsProducer>(*render_manager).register_new(result);
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
    /// Is only callable from the SceneManager.
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
class GraphicsProducer::Access<Layer> {
    friend class Layer;

    /// Constructor.
    /// @param producer     The GraphicsProducer to access.
    Access(GraphicsProducer& producer) : m_producer(producer) {}

    /// Renders the GraphicsProducer, if it is dirty.
    void render() { m_producer.render(); }

    /// The GraphicsProducer to access.
    GraphicsProducer& m_producer;
};

template<>
class GraphicsProducer::Access<RenderTarget> {
    friend class RenderTarget;

    /// Constructor.
    /// @param producer     The GraphicsProducer to access.
    Access(GraphicsProducer& producer) : m_producer(producer) {}

    /// Renders the GraphicsProducer, if it is dirty.
    void render() { m_producer.render(); }

    /// The GraphicsProducer to access.
    GraphicsProducer& m_producer;
};

NOTF_CLOSE_NAMESPACE
