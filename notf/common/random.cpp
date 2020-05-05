#include "notf/common/random.hpp"

NOTF_OPEN_NAMESPACE

// random number generator ========================================================================================== //

namespace { // anonymous

pcg32 create_global_rng() {
    pcg_extras::seed_seq_from<std::random_device> seed_source;
    return pcg32(seed_source);
}

} // namespace

namespace detail {

pcg32& global_rng() {
    static pcg32 rng = create_global_rng();
    return rng;
}

} // namespace detail

NOTF_CLOSE_NAMESPACE
