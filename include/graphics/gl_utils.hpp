#pragma once

namespace signal {

/**
 * @brief Nicer way to provide a buffer offset to glVertexAttribPointer.
 *
 * Adapted from https://stackoverflow.com/a/23177754/3444217
 *
 * @param offset    Buffer offset.
 */
constexpr auto buffer_offset(int offset) { return static_cast<char*>(0) + offset; }

} // namespace signal
