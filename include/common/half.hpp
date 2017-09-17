#pragma once

namespace notf {

/**
 * The `half` type is only used to store the result of floating point operations.
 * No mathematical operations are defined for half
 */
struct half {
    /** Default constructor. */
    half() = default;

    /** Value constructor. */
    half(const float input);

    /** Converts the half back to a float. */
    explicit operator float() const;

    /** Half value. */
    short value;
};

} // namespace notf
