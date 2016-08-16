#pragma once

namespace signal {

/**@brief Nicer way to provide a buffer offset to glVertexAttribPointer.
 *
 * @param offset    Buffer offset.
 */
template <typename T>
constexpr auto buffer_offset(int offset) { return reinterpret_cast<void*>(offset * sizeof(T)); }

} // namespace signal
