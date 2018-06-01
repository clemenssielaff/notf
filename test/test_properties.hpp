#pragma once

#include "app/property_graph.hpp"

NOTF_OPEN_NAMESPACE

/// Test Accessor providing test-related access functions to a PropertyGraph.
/// The test::Harness accessors subvert about any safety guards in place and are only to be used for testing under
/// controlled circumstances (and only from a single thread)!
template<>
class access::_PropertyGraph<test::Harness> {
public:
    /// Number of properties in the graph.
    /// /// @param graph    PropertyGraph to operate on.
    static size_t size() { return PropertyGraph::s_body_count; }
};

NOTF_CLOSE_NAMESPACE
