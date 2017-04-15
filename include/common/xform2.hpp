#pragma once

#include <array>
#include <assert.h>
#include <iosfwd>

#include "common/float.hpp"
#include "common/hash.hpp"
#include "common/vector2.hpp"
#include "utils/sfinae.hpp"

namespace notf {

//*********************************************************************************************************************/

/** A 2D Transformation Matrix with 3x3 components.
 * Only the first two columns are actually stored though, the last column is implicit.
 * [[a, b, 0]
 *  [c, d, 0]
 *  [e, f, 1]]
 */
template <typename Real, ENABLE_IF_REAL(Real)>
struct _Xform2 {

    /* Types **********************************************************************************************************/

    using Value_t = Real;

    using Vector_t = _RealVector2<Value_t>;

    /* Fields *********************************************************************************************************/

    /** First two columns of each Matrix row. */
    std::array<Vector_t, 3> rows;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    _Xform2() = default;

    /* Static Constructors ********************************************************************************************/

    /** The identity matrix. */
    static _Xform2 identity()
    {
        return _Xform2{{{{1, 0},
                         {0, 1},
                         {0, 0}}}};
    }

    /** A translation matrix. */
    static _Xform2 translation(Vector_t translation)
    {
        return _Xform2{{{{1, 0},
                         {0, 1},
                         std::move(translation)}}};
    }

    /** A rotation matrix.
     * @param radians   Counter-clockwise rotation in radians.
     */
    static _Xform2 rotation(const Value_t radians)
    {
        const Value_t sine   = sin(radians);
        const Value_t cosine = cos(radians);
        return _Xform2{{{{cosine, sine},
                         {-sine, cosine},
                         {0, 0}}}};
    }

    /** A uniform scale matrix.
     * @param factor   Uniform scale factor.
     */
    static _Xform2 scale(const Value_t factor)
    {
        return _Xform2{{{{factor, 0},
                         {0, factor},
                         {0, 0}}}};
    }

    /** A non-uniform scale matrix.
     * @param vector   Non-uniform scale vector.
     */
    static _Xform2 scale(const Vector_t& vec)
    {
        return _Xform2{{{{vec.x, 0},
                         {0, vec.y},
                         {0, 0}}}};
    }

    /** A non-uniform skew matrix.
     * @param vector   Non-uniform skew vector.
     */
    static _Xform2 skew(const Vector_t& vec)
    {
        return _Xform2{{{{1, tan(vec.y)},
                         {tan(vec.x), 1},
                         {0, 0}}}};
    }

    /*  Inspection  ***************************************************************************************************/

    /** Returns the translation part of this Xform. */
    const Vector_t& get_translation() const { return rows[2]; }

    /** Scale factor along the x-axis. */
    Value_t scale_factor_x() const { return sqrt(rows[0][0] * rows[0][0] + rows[1][0] * rows[1][0]); }

    /** Scale factor along the y-axis. */
    Value_t scale_factor_y() const { return sqrt(rows[0][1] * rows[0][1] + rows[1][1] * rows[1][1]); }

    /** Read-only reference to a row of the Xform2's internal matrix. */
    template <typename Row, ENABLE_IF_INT(Row)>
    const Vector_t& operator[](const Row row) const
    {
        if (0 > row || row > 2) {
            throw std::range_error("Index requested via Xform2 [] operator is out of bounds");
        }
        return rows[row];
    }

    /** Read-only pointer to the Xform2's internal storage. */
    const Value_t* as_ptr() const { return &rows[0][0]; }

    /** Operators *****************************************************************************************************/

    /** Tests whether two Xform2s are equal. */
    bool operator==(const _Xform2& other) const { return rows == other.rows; }

    /** Tests whether two Xform2s are not equal. */
    bool operator!=(const _Xform2& other) const { return rows != other.rows; }

    /** Modifiers *****************************************************************************************************/

    /** Multiplication of this Matrix with another. */
    _Xform2 operator*(const _Xform2& other) const
    {
        _Xform2 result = *this;
        return result *= other;
    }

    /** Applies another Xform to this one in-place. */
    _Xform2& operator*=(const _Xform2& other)
    {
        Value_t* this_array        = this->as_ptr();
        const Value_t* other_array = other.as_ptr();
        const Value_t t0           = this_array[0] * other_array[0] + this_array[1] * other_array[2];
        const Value_t t2           = this_array[2] * other_array[0] + this_array[3] * other_array[2];
        const Value_t t4           = this_array[4] * other_array[0] + this_array[5] * other_array[2] + other_array[4];
        this_array[1]              = this_array[0] * other_array[1] + this_array[1] * other_array[3];
        this_array[3]              = this_array[2] * other_array[1] + this_array[3] * other_array[3];
        this_array[5]              = this_array[4] * other_array[1] + this_array[5] * other_array[3] + other_array[5];
        this_array[0]              = t0;
        this_array[2]              = t2;
        this_array[4]              = t4;
        return *this;
    }

    /** Inverts this Xform2 in-place. */
    _Xform2& invert() { *this = (*this).get_inverse(); } // TODO: test Xform2::invert

    /** Returns the inverse of this Xform2. */
    _Xform2 get_inverse() const
    {
        const Value_t det = rows[0][0] * rows[1][1] - rows[1][0] * rows[0][1];
        if (abs(det) <= precision_high<Value_t>()) {
            return _Xform2::identity();
        }
        const Value_t invdet = 1 / det;
        _Xform2 result;
        result[0][0] = rows[1][1] * invdet;
        result[1][0] = -rows[1][0] * invdet;
        result[2][0] = (rows[1][0] * rows[2][1] - rows[1][1] * rows[2][0]) * invdet;
        result[0][1] = -rows[0][1] * invdet;
        result[1][1] = rows[0][0] * invdet;
        result[2][1] = (rows[0][1] * rows[2][0] - rows[0][0] * rows[2][1]) * invdet;
        return result;
    }

    /** Returns a transformed Vector2. */
    Vector_t transform(const Vector_t& vector) const
    {
        return {
            vector.x * rows[0][0] + vector.y * rows[1][0] + rows[2][0],
            vector.x * rows[0][1] + vector.y * rows[1][1] + rows[2][1],
        };
    }

    /** Read-write reference to a row of the Xform2's internal matrix. */
    template <typename Row, ENABLE_IF_INT(Row)>
    Vector_t& operator[](const Row row)
    {
        if (0 > row || row > 2) {
            throw std::range_error("Index requested via Xform2 [] operator is out of bounds");
        }
        return rows[row];
    }

    /** Read-write pointer to the Xform2's internal storage. */
    Value_t* as_ptr() { return &rows[0][0]; }
};

//*********************************************************************************************************************/

using Xform2f = _Xform2<float>;

/* Free Functions *****************************************************************************************************/

/** Prints the contents of this Aabr into a std::ostream.
 * @param out   Output stream, implicitly passed with the << operator.
 * @param aabr  Aabr to print.
 * @return      Output stream for further output.
 */
template <typename Real>
std::ostream& operator<<(std::ostream& out, const notf::_Xform2<Real>& aabr);

} // namespace notf

/* std::hash **********************************************************************************************************/

namespace std {

/** std::hash specialization for notf::_Aabr. */
template <typename Real>
struct hash<notf::_Xform2<Real>> {
    size_t operator()(const notf::_Xform2<Real>& xform2) const { return notf::hash(xform2[0], xform2[1], xform2[2]); }
};
}
