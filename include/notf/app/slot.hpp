#pragma once

#include "notf/reactive/operator.hpp"

#include "notf/app/node_handle.hpp"

NOTF_OPEN_NAMESPACE

// relay ============================================================================================================ //

/// Base class for all Relays.
using AnyRelay = AnyOperator;

/// A Relay is a typedef for a reactive Operator with the same input- and output-type and only a single Subscriber.
template<class T>
using Relay = Operator<T, T, detail::SinglePublisherPolicy>;

// relay handle ===================================================================================================== //

template<class T>
class RelayHandle {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Property value type.
    using value_t = T;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default (empty) Constructor.
    RelayHandle() = default;

//    /// Value Constructor.
//    /// @param node     Node owning the Relay
//    /// @param relay    Relay to handle.
//    RelayHandle(NodeHandle node, PropertyPtr<T> property) : m_node(std::move(node)), m_property(std::move(property))
//    {}
};

NOTF_CLOSE_NAMESPACE
