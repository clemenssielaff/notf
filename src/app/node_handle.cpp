#include "notf/app/node_handle.hpp"

#include "notf/app/node.hpp"

NOTF_OPEN_NAMESPACE

// node handle ====================================================================================================== //

const Uuid& NodeHandle::get_uuid() const { return _get_node()->get_uuid(); }

std::string_view NodeHandle::get_name() const { return _get_node()->get_name(); }

NOTF_CLOSE_NAMESPACE
