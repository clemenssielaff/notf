#pragma once

#include <vector>

#include "app/forwards.hpp"
#include "common/pointer.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// Base class for Visualizer.
class Visualizer {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Destructor.
    virtual ~Visualizer();

    /// Subclass-defined visualization implementation.
    /// @param scene    Scene to visualize.
    virtual void visualize(valid_ptr<Scene*> scene) const = 0;

private:
    /// Report all Plates that this Visualizer depends on.
    /// The default implementation does nothing, it is the subclass' responsibility to add *all* of its dependencies.
    /// @param dependencies     [out] Dependencies to add yours to.
    virtual void _collect_dependencies(std::vector<Plate*>& /*dependencies*/) const {}
};

NOTF_CLOSE_NAMESPACE
