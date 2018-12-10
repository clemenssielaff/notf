#pragma once

#include "notf/meta/config.hpp"

#include "boost/fiber/all.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

namespace this_fiber = ::boost::this_fiber;
namespace fibers = ::boost::fibers;
using Fiber = fibers::fiber;

NOTF_CLOSE_NAMESPACE
