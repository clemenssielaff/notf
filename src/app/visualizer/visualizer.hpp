#pragma once

#include <vector>

#include "app/forwards.hpp"
#include "common/exception.hpp"
#include "common/pointer.hpp"

NOTF_OPEN_NAMESPACE

namespace access { // forwards
template<class>
class _Visualizer;
} // namespace access

// ================================================================================================================== //

/// Base class for Visualizer.
class Visualizer {

    friend class access::_Visualizer<Layer>;
    friend class access::_Visualizer<Plate>;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    template<class T>
    using Access = access::_Visualizer<T>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Destructor.
    virtual ~Visualizer();

private:
    /// Report all Plates that this Visualizer depends on.
    /// The default implementation does nothing, it is the subclass' responsibility to add *all* of its dependencies.
    /// @param dependencies     [out] Dependencies to add yours to.
    virtual void _collect_dependencies(std::vector<Plate*>& /*dependencies*/) const {}

    /// Subclass-defined visualization implementation.
    /// @param scene    Scene to visualize.
    virtual void _visualize(valid_ptr<Scene*> scene) const = 0;
};

// ================================================================================================================== //

template<>
class access::_Visualizer<Layer> {
    friend class notf::Layer;

    /// Invokes the Visualizer.
    /// @param scene    Scene to visualize.
    static void visualize(Visualizer& visualizer, valid_ptr<Scene*> scene) { visualizer._visualize(std::move(scene)); }
};

template<>
class access::_Visualizer<Plate> {
    friend class notf::Plate;

    /// Invokes the Visualizer.
    /// @param scene    Scene to visualize.
    static void visualize(Visualizer& visualizer, valid_ptr<Scene*> scene) { visualizer._visualize(std::move(scene)); }
};

NOTF_CLOSE_NAMESPACE
