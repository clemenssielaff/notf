#pragma once

// symbol import/export ============================================================================================= //

/// Generic helper definitions for shared library support.
/// (from https://gcc.gnu.org/wiki/Visibility)
#if defined _WIN32 || defined __CYGWIN__
#define NOTF_DLL_IMPORT __declspec(dllimport)
#define NOTF_DLL_EXPORT __declspec(dllexport)
#define NOTF_DLL_LOCAL
#else
#define NOTF_DLL_IMPORT __attribute__((visibility("default")))
#define NOTF_DLL_EXPORT __attribute__((visibility("default")))
#define NOTF_DLL_LOCAL __attribute__((visibility("hidden")))
#endif

#define NOTF_C_API extern "C"
