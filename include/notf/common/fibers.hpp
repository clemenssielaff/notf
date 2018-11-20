#pragma once

#include "notf/meta/config.hpp"

#include "boost/fiber/all.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

using Fiber = ::boost::fibers::fiber;
namespace this_fiber = ::boost::this_fiber;

NOTF_CLOSE_NAMESPACE
