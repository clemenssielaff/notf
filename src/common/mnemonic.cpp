/// This is a C++ port of a C implementation by John Mettraux (https://github.com/flon-io/mnemo),
/// which was in turn a C port from Ruby of its own code (https://github.com/jmettraux/rufus-mnemo).
///
#include "notf/common/mnemonic.hpp"

#include <array>

#include "notf/common/string.hpp"
#include "notf/meta/numeric.hpp"
#include "notf/meta/types.hpp"

namespace {
NOTF_USING_NAMESPACE;

constexpr std::array<const char*, 100> g_syllables{
    "ba",  "bi",  "bu",  "be",  "bo",  //
    "cha", "chi", "chu", "che", "cho", //
    "da",  "di",  "du",  "de",  "do",  //
    "fa",  "fi",  "fu",  "fe",  "fo",  //
    "ga",  "gi",  "gu",  "ge",  "go",  //
    "ha",  "hi",  "hu",  "he",  "ho",  //
    "ja",  "ji",  "ju",  "je",  "jo",  //
    "ka",  "ki",  "ku",  "ke",  "ko",  //
    "la",  "li",  "lu",  "le",  "lo",  //
    "ma",  "mi",  "mu",  "me",  "mo",  //
    "na",  "ni",  "nu",  "ne",  "no",  //
    "pa",  "pi",  "pu",  "pe",  "po",  //
    "ra",  "ri",  "ru",  "re",  "ro",  //
    "sa",  "si",  "su",  "se",  "so",  //
    "sha", "shi", "shu", "she", "sho", //
    "ta",  "ti",  "tu",  "te",  "to",  //
    "tsa", "tsi", "tsu", "tse", "tso", //
    "wa",  "wi",  "wu",  "we",  "wo",  //
    "ya",  "yi",  "yu",  "ye",  "yo",  //
    "za",  "zi",  "zu",  "ze",  "zo",  //
};

constexpr auto g_syllable_sizes = []() {
    std::array<uchar, g_syllables.size()> result{};
    for (uchar i = 0; i < g_syllables.size(); ++i) {
        result[i] = static_cast<uchar>(cstring_length(g_syllables[i]));
    }
    return result;
}();

} // namespace

// mnemonic ========================================================================================================= //

NOTF_OPEN_NAMESPACE

std::string number_to_mnemonic(size_t number, uint max_syllables)
{
    if (max_syllables > 0) { number %= exp(g_syllables.size(), max_syllables); }

    std::string result;
    result.reserve(((count_digits(number) / 2) + 1) * 3);

    result.append(g_syllables[number % g_syllables.size()]);
    for (number /= g_syllables.size(); number != 0; number /= g_syllables.size()) {
        result.append(g_syllables[number % g_syllables.size()]);
    }

    result.shrink_to_fit();
    return result;
}

std::optional<size_t> mnemonic_to_number(std::string_view mnemonic)
{
    size_t result = 0;
    while (!mnemonic.empty()) {
        const size_t size_before = mnemonic.size();
        if (size_before < 2) { return {}; } // no syllable will match a string of less than 2 characters
        for (size_t i = 0; i < g_syllables.size(); ++i) {
            const uchar syllable_size = g_syllable_sizes[i];
            if (mnemonic.size() >= syllable_size && mnemonic.compare(0, syllable_size, g_syllables[i]) == 0) {
                result = (g_syllables.size() * result) + i;
                mnemonic.remove_prefix(syllable_size);
                break;
            }
        }
        if (mnemonic.size() == size_before) {
            return {}; // no syllable matched
        }
    }
    return result;
}

NOTF_CLOSE_NAMESPACE
