#pragma once

#include "app/properties.hpp"

NOTF_OPEN_NAMESPACE

/// Test Accessor providing test-related access functions to a PropertyGraph.
/// The test::Harness accessors subvert about any safety guards in place and are only to be used for testing under
/// controlled circumstances (and only from a single thread)!
template<>
class PropertyGraph::Access<test::Harness> {
public:
    /// Constructor.
    /// @param graph    PropertyGraph to access.
    Access(PropertyGraph& graph) : m_graph(graph) {}

    /// Number of properties in the graph.
    size_t size() const { return m_graph.m_properties.size(); }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// PropertyGraph to access.
    PropertyGraph& m_graph;
};

NOTF_CLOSE_NAMESPACE
