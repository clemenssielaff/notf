#pragma once

#include "./render_manager.hpp"
#include "common/forwards.hpp"
#include "common/id.hpp"
#include "utils/make_smart_enabler.hpp"

namespace notf {

namespace detail {
class RenderTargetDependencies;
} // namespace detail

// ===================================================================================================================//

/// Property id type.
using GraphicsProducerId = IdType<GraphicsProducer, size_t>;

// ===================================================================================================================//

/// Base class for GraphicsProducer.
class GraphicsProducer {

    // types ---------------------------------------------------------------------------------------------------------//
protected:
    /// Token object to make sure that the instance can only be created by a call to `_create`.
    class Token {
        friend class GraphicsProducer;
        Token() = default;
    };

    /// GraphicsProducer subclasses must identify themselves to the RenderManager, so it can try to minimize graphics
    /// state changes when rendering multiple GraphicsProducers in sequence.
    enum class Type {
        PLOTTER,
        PROCEDURAL,
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
    static std::shared_ptr<T> _create(RenderManager& render_manager, Ts&&... args)
    {
        static_assert(std::is_base_of<GraphicsProducer, T>::value, "GraphicsProducer::_create can only create "
                                                                   "instances of subclasses of GraphicsProducer");
        const Token token;
#ifdef NOTF_DEBUG
        auto result = std::shared_ptr<T>(new T(token, render_manager, std::forward<Ts>(args)...));
#else
        auto result = std::make_shared<make_shared_enabler<T>>(token, render_manager, std::forward<Ts>(args)...);
#endif
        RenderManager::Private<GraphicsProducer>(render_manager).register_new(result);
        return result;
    }

public:
    /// Destructor.
    virtual ~GraphicsProducer();

    /// Id of this GraphicsProducer.
    GraphicsProducerId id() const noexcept { return m_id; }

    /// Unique type of this GraphicsProducer subclass.
    virtual Type render_type() const = 0;

    /// Render with the current graphics state.
    virtual void render() const = 0;

    /// Whether the GraphicsProducer is currently dirty or not.
    virtual bool is_dirty() const = 0;

    /// Report all RenderTargets that this Producer depends on.
    /// The default implementation does nothing, it is the subclass' responsibility to add *all* of its dependencies.
    /// @param dependencies     [out] Dependencies to add yours to.
    virtual void report_dependencies(detail::RenderTargetDependencies& /*dependencies*/) const {}

private:
    /// Generate the next available GraphicsProducerId.
    static GraphicsProducerId _next_id();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// GraphicsProducer id.
    const GraphicsProducerId m_id;
};

} // namespace notf
