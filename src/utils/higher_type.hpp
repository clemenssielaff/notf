#include "common/meta.hpp"

#include <limits>

NOTF_OPEN_NAMESPACE

/// Trait containing the type that has a higher numeric limits.
template<typename LEFT, typename RIGHT>
struct higher_type {
    using type = typename std::conditional<std::numeric_limits<LEFT>::max() <= std::numeric_limits<RIGHT>::max(), LEFT,
                                           RIGHT>::type;
};

NOTF_CLOSE_NAMESPACE
