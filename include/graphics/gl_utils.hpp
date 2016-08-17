#pragma once

namespace signal {

/**@brief Nicer way to provide a buffer offset to glVertexAttribPointer.
 *
 * @param offset    Buffer offset.
 */
template <typename T>
constexpr auto buffer_offset(int offset) { return reinterpret_cast<void*>(offset * sizeof(T)); }


/*!
 * @brief Helper class to make sure that a Vertex Array Object (VAO) is always unbound after a function exits.
 */
struct VaoBindRAII {
    VaoBindRAII(GLuint vao) { glBindVertexArray(vao); }
    ~VaoBindRAII() { glBindVertexArray(0); }
};

} // namespace signal
