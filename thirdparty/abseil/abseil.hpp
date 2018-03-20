#pragma once

namespace absl {

struct in_place_t {};

template<typename T>
struct in_place_type_t {};

static constexpr in_place_t in_place = {};

} // namespace absl
