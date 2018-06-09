#pragma once

#include "app/node_property.hpp"

NOTF_OPEN_NAMESPACE

/// Test Accessor providing test-related access functions to a PropertyGraph.
/// The test::Harness accessors subvert about any safety guards in place and are only to be used for testing under
/// controlled circumstances (and only from a single thread)!
template<>
class access::_PropertyGraph<test::Harness> {
public:
    /// Number of properties in the graph.
    static size_t size() { return PropertyGraph::s_body_count; }
};

template<>
class access::_NodeProperty<test::Harness> {
public:
    /// Current NodeProperty value.
    /// @param property     Property to operate on.
    /// @param thread_id    (pretend) Id of this thread. Is exposed so it can be overridden by tests.
    template<class T>
    static const T& get(const TypedNodeProperty<T>& property, const std::thread::id thread_id)
    {
        return property._get(thread_id);
    }

    /// Extracts and returns a reference from a PropertyHandle.
    template<class T>
    static TypedNodeProperty<T>& raw(PropertyHandle<T>& handle)
    {
        TypedNodePropertyPtr<T> property = handle.m_property.lock();
        NOTF_ASSERT(property);
        return *property;
    }
};

NOTF_CLOSE_NAMESPACE
