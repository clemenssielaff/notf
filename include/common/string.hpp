#pragma once

#include <memory>
#include <string>
#include <vector>

namespace notf {

/** Tokenizes a string.
 * @param input         Input string.
 * @param delimiter     Delimiter character (is removed from the tokens).
 * @return              String tokens.
 */
std::vector<std::string> tokenize(const std::string& input, const char delimiter);

/** tokenize() overload to deal with (potentially nullptr) c-style character arrays.
 * @param input         Input string.
 * @param delimiter     Delimiter character.
 * @return              String tokens.
 */
inline std::vector<std::string> tokenize(const char* input, const char delimiter) {
	if (!input) {
		return {};
	}
	return tokenize(std::string(input), delimiter);
}

/** Tests if a string ends starts a given prefix.
 * @param input     Input string.
 * @param prefix    Prefix to test for.
 * @return True if the first n input string characters match the prefix of size n.
 */
inline bool starts_with(const std::string& input, const std::string& prefix) {
	return (input.compare(0, prefix.size(), prefix) == 0);
}

/** Case insensitive test if a string starts with a given prefix.
 * @param input     Input string.
 * @param prefix    Prefix to test for.
 * @return True iff the first n input string characters match the prefix of size n.
 */
bool istarts_with(const std::string& input, const std::string& prefix);

/** Tests if a string ends with a given postfix.
 * @param input     Input string.
 * @param postfix   Postfix to test for.
 * @return True if the last n input string characters match the postfix of size n.
 */
bool ends_with(const std::string& input, const std::string& postfix);

/** Case insensitive test if a string ends with a given postfix.
 * @param input     Input string.
 * @param postfix   Postfix to test for.
 * @return True iff the last n input string characters match the postfix of size n.
 */
bool iends_with(const std::string& input, const std::string& postfix);

/** Case-insensitive string comparison.
 * @return  True iff both strings are the same in lower case letters.
 */
bool icompare(const std::string& left, const std::string& right);

/** String Formatting function using std::snprinftf.
 * For details on the format string see: http://en.cppreference.com/w/cpp/io/c/fprintf
 * Adapted from: http://stackoverflow.com/a/26221725/3444217
 */
template<typename... Args>
std::string string_format(const std::string& format, Args... args) {
	size_t size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
	std::unique_ptr<char[]> buffer(new char[size]);
	std::snprintf(buffer.get(), size, format.c_str(), args...);
	return std::string(buffer.get(), buffer.get() + size - 1); // We don't want the '\0' inside
}

/** Levenshtein "string distance" algorithm.
 *
 * Originally from
 * https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C.2B.2B
 * @param s1    First string.
 * @param s2    Second string.
 * @return      Number of additions, modifications or removals to get from s1 to s2.
 */
size_t levenshtein_distance(const std::string& s1, const std::string& s2);

/** Extracts the last part of a pathname at compile time.
 * @param input     Path to investigate.
 * @param delimiter Delimiter used to separate path elements.
 * @return          Only the last part of the path, e.g. basename("/path/to/some/file.cpp", '/') would return
 * "file.cpp".
 */
constexpr const char* basename(const char* input, const char delimiter = '/') {
	size_t last_occurrence = 0;
	for (size_t offset = 0; input[offset]; ++offset) {
		if (input[offset] == delimiter) {
			last_occurrence = offset + 1;
		}
	}
	return &input[last_occurrence];
}

} // namespace notf
