#pragma once

#include <functional>
#include <set>

namespace notf {

/** Erases all elements from a set, that fulfill a certain condition.
 * From http://stackoverflow.com/a/16013546/3444217
 *
 * Call as follows:
 * 	auto foo = std::set<int>{0, 1, 2, 3, 4, 5, 6, 7 };
 *  erase_conditional(foo, [](const int& value) -> bool {
 *      return value % 2 == 0;
 *  });
 *  // foo is now {1, 3, 5, 7 }
 */
template<typename T, typename Func>
void erase_conditional(std::set<T>& set, Func condition)
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

} // namespace notf
