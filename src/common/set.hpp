#pragma once

#include <functional>
#include <unordered_set>

#include "common/meta.hpp"

NOTF_OPEN_NAMESPACE

/// Pops a single element from a std::set and places it into the output parameter.
/// @return True iff an element was popped from the set.
template<typename T, typename U, typename = std::enable_if_t<std::is_convertible<T, U>::value>>
bool pop_one(std::unordered_set<T>& set, U& result)
{
    if (set.empty()) {
        return false;
    }

    auto it = set.begin();
    result = *it;
    set.erase(it);
    return true;
}

/// Erases all elements from a set, that fulfill a certain condition.
/// From http://stackoverflow.com/a/16013546/3444217
///
/// Call as follows:
/// 	auto foo = std::set<int>{0, 1, 2, 3, 4, 5, 6, 7 };
///  erase_conditional(foo, [](const int& value) -> bool {
///      return value % 2 == 0;
///  });
///  // foo is now {1, 3, 5, 7 }
///
template<typename T, typename Func>
void erase_conditional(std::unordered_set<T>& set, Func condition)
{
    for (auto it = set.begin(); it != set.end();) {
        if (condition(*it)) {
            it = set.erase(it);
        }
        else {
            ++it;
        }
    }
}

NOTF_CLOSE_NAMESPACE
