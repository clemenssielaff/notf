#pragma once

namespace notf {

struct StencilFunc {
    enum Stencil : unsigned char {
        INVALID = 0,
        ALWAYS,
        NEVER,
        LESS,
        LEQUAL,
        GREATER,
        GEQUAL,
        EQUAL,
        NOTEQUAL,
    };

    /** The Stencil function. */
    Stencil function;

    /** Default Constructor. */
    StencilFunc()
        : function(ALWAYS) {}

    /** Value Constructor. */
    StencilFunc(const Stencil function)
        : function(function) {}

    /** Applies the stencil function to OpenGL.
     * A valid OpenGL context must exist before calling this function.
     */
    void apply() const;

    /** Equality operator. */
    bool operator==(const StencilFunc& other) const { return function == other.function; }

    /** Inequality operator. */
    bool operator!=(const StencilFunc& other) const { return function != other.function; }

    /** Checks if the StencilFunc is valid. */
    bool is_valid() const { return function != INVALID; }
};

} // namespace notf
