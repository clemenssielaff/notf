#pragma once

#include <memory>

namespace notf {

// https://stackoverflow.com/a/8147213/3444217 and https://stackoverflow.com/a/25069711/3444217
template<typename T>
class MakeSmartEnabler : public T { // TODO: get rid of MakeSmartEnabler wherever possible
public:
    template<typename... Args>
    MakeSmartEnabler(Args&&... args) : T(std::forward<Args>(args)...)
    {
    }
};

} // namespace notf
