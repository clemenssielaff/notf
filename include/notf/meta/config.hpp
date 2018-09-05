#pragma once

// ================================================================================================================== //

/// Version of the notf code.
#define NOTF_VERSION_MAJOR 0
#define NOTF_VERSION_MINOR 4

/// Name of the notf namespace.
/// This macro can be used to implement namespace versioning.
#define NOTF_NAMESPACE_NAME notf

/// Opens the notf namespace.
#define NOTF_OPEN_NAMESPACE namespace NOTF_NAMESPACE_NAME {

/// For visual balance with NOTF_OPEN_NAMESPACE.
#define NOTF_CLOSE_NAMESPACE }

/// Use the notf namespace.
#define NOTF_USING_NAMESPACE using namespace NOTF_NAMESPACE_NAME
