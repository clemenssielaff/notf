#pragma once

#include "../meta/config.hpp"

// common namespace ================================================================================================= //

/// Name of the common namespace
#define NOTF_COMMON_NAMESPACE_NAME common

/// Opens the common namespace.
#define NOTF_OPEN_COMMON_NAMESPACE NOTF_OPEN_NAMESPACE namespace NOTF_COMMON_NAMESPACE_NAME {

/// For visual balance with NOTF_OPEN_COMMON_NAMESPACE.
#define NOTF_CLOSE_COMMON_NAMESPACE NOTF_CLOSE_NAMESPACE }

/// Use the versioned namespace.
#define NOTF_USING_COMMON_NAMESPACE using namespace ::NOTF_NAMESPACE_NAME::NOTF_COMMON_NAMESPACE_NAME
