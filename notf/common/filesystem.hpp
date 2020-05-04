#pragma once

#include <string>

#include "notf/meta/config.hpp"

NOTF_OPEN_NAMESPACE

// filesystem ======================================================================================================= //

/// Reads the contents of a file into a string.
/// @param file_path        Path (relative to cwd) of the file to read.
/// @throws ResourceError   If the file failed to load.
/// @return                 Contents of the file.
std::string load_file(const std::string& file_path);

NOTF_CLOSE_NAMESPACE
