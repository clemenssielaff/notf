#pragma once

#include "app/event_manager.hpp"

NOTF_OPEN_NAMESPACE

/// Test Accessor providing test-related access functions to the EventManager.
/// The test::Harness accessors subvert about any safety guards in place and are only to be used for testing under
/// controlled circumstances (and only from a single thread)!
template<>
class EventManager::Access<test::Harness> {

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    /// @param manager    EventManager to access.
    Access(EventManager& manager) : m_manager(manager) {}

    /// Returns the number of items in the EventManager's backlog.
    size_t backlog_size() const { return m_manager.m_backlog.size(); }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// EventManager to access.
    EventManager& m_manager;
};

NOTF_CLOSE_NAMESPACE
