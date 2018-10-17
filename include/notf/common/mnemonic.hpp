#pragma once

#include <optional>
#include <string>

#include "notf/meta/config.hpp"

NOTF_OPEN_NAMESPACE

// mnemonic ========================================================================================================= //

/// Converts a number to a random word, that should be reasonably easy to remember and to communicate.
/// @param number           Number identifiying the Mnemonic.
/// @param max_syllables    Maximum number of syllables to be generated, 0 means no limit.
std::string number_to_mnemonic(size_t number, uint max_syllables = 0);

/// Converts a mnemonic word back to a number
/// @param mnemonic Mnemonic generated with `number_to_mnemonic`.
/// @returns        Optional value is empty, if the given string is not a mnemonic.
std::optional<size_t> mnemonic_to_number(std::string_view mnemonic);

NOTF_CLOSE_NAMESPACE
