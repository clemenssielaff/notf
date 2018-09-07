#pragma once

#include "./config.hpp"

// meta namespace =================================================================================================== //

/// Name of the meta namespace
#define NOTF_META_NAMESPACE_NAME meta

/// Opens the meta namespace.
#define NOTF_OPEN_META_NAMESPACE NOTF_OPEN_NAMESPACE namespace NOTF_META_NAMESPACE_NAME {

/// For visual balance with NOTF_OPEN_META_NAMESPACE.
#define NOTF_CLOSE_META_NAMESPACE NOTF_CLOSE_NAMESPACE }

/// Use the versioned namespace.
#define NOTF_USING_META_NAMESPACE using namespace NOTF_NAMESPACE_NAME::NOTF_META_NAMESPACE_NAME

// short unsigned integer names ===================================================================================== //

using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
