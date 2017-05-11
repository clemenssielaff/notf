#pragma once

#include <array>
#include <assert.h>
#include <iosfwd>

#include "common/aabr.hpp"
#include "common/float.hpp"
#include "common/hash.hpp"
#include "common/template.hpp"
#include "common/vector2.hpp"

namespace notf {

//*********************************************************************************************************************/

/** A 2D (row-major) transformation matrix with 3x3 components.
 * Only the first two columns are actually stored though, the last column is implicit.
 * [[a, b, 0]
 *  [c, d, 0]
 *  [e, f, 1]]
 */
template <typename Real, ENABLE_IF_REAL(Real)>
struct _Xform2 {

    /* Types **********************************************************************************************************/

    using value_t = Real;

    using vector_t = _RealVector2<value_t>;

    /* Fields *********************************************************************************************************/

    /** First two columns of each Matrix row. */
    std::array<vector_t, 3> rows;

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
    static _Xform2 translation(vector_t translation)
    {
        return _Xform2{{{{1, 0},
                         {0, 1},
                         std::move(translation)}}};
    }

    /** A rotation matrix.
     * @param radians   Counter-clockwise rotation in radians.
     */
    static _Xform2 rotation(const value_t radians)
    {
        const value_t sine   = sin(radians);
        const value_t cosine = cos(radians);
        return _Xform2{{{{cosine, sine},
                         {-sine, cosine},
                         {0, 0}}}};
    }

    /** A uniform scale matrix.
     * @param factor   Uniform scale factor.
     */
    static _Xform2 scale(const value_t factor)
    {
        return _Xform2{{{{factor, 0},
                         {0, factor},
                         {0, 0}}}};
    }

    /** A non-uniform scale matrix.
     * You can also achieve reflection by passing (-1, 1) for a reflection over the vertical axis, (1, -1) for over the
     * horizontal axis or (-1, -1) for a point-reflection with respect to the origin.
     * @param vector   Non-uniform scale vector.
     */
    static _Xform2 scale(const vector_t& vec)
    {
        return _Xform2{{{{vec.x, 0},
                         {0, vec.y},
                         {0, 0}}}};
    }

    /** A non-uniform skew matrix.
     * @param vector   Non-uniform skew vector.
     */
    static _Xform2 skew(const vector_t& vec)
    {
        return _Xform2{{{{1, tan(vec.y)},
                         {tan(vec.x), 1},
                         {0, 0}}}};
    }

    /*  Inspection  ***************************************************************************************************/

    /** Returns the translation part of this Xform. */
    const vector_t& get_translation() const { return rows[2]; }

    /** Scale factor along the x-axis. */
    value_t get_scale_x() const { return sqrt(rows[0][0] * rows[0][0] + rows[1][0] * rows[1][0]); }

    /** Scale factor along the y-axis. */
    value_t get_scale_y() const { return sqrt(rows[0][1] * rows[0][1] + rows[1][1] * rows[1][1]); }

    /** Calculates the determinant of the transformation matrix. */
    value_t get_determinant() const { return (rows[0][0] * rows[1][1]) - (rows[1][0] * rows[0][1]); }

    /** Read-only reference to a row of the Xform2's internal matrix. */
    template <typename Row, ENABLE_IF_INT(Row)>
    const vector_t& operator[](const Row row) const
    {
        assert(0 <= row && row <= 2);
        return rows[row];
    }

    /** Read-only pointer to the Xform2's internal storage. */
    const value_t* as_ptr() const { return &rows[0][0]; }

    /** Operators *****************************************************************************************************/

    /** Tests whether two Xform2s are equal. */
    bool operator==(const _Xform2& other) const { return rows == other.rows; }

    /** Tests whether two Xform2s are not equal. */
    bool operator!=(const _Xform2& other) const { return rows != other.rows; }

    /** Returns True, if other and self are approximately the same Xform2.
     * @param other     Xform2 to test against.
     * @param epsilon   Maximal allowed distance between the individual entries in the transform matrix.
     */
    bool is_approx(const _Xform2& other, const value_t epsilon = precision_high<value_t>()) const
    {
        for (size_t i = 0; i < 6; ++i) {
            if (abs(as_ptr()[i] - other.as_ptr()[i]) > epsilon) {
                return false;
            }
        }
        return true;
    }

    /** Modifiers *****************************************************************************************************/

    /** Translates the transformation in-place by a given delta vector. */
    _Xform2& translate(const vector_t& delta) { return premult(_Xform2::translation(delta)); }

    /** Rotates the transformation in-place by a given angle in radians. */
    _Xform2& rotate(const value_t radians) { return premult(_Xform2::rotation(radians)); }

    /** Multiplication of this Matrix with another. */
    _Xform2 operator*(const _Xform2& other) const
    {
        _Xform2 result = *this;
        return result *= other;
    }

    /** Applies another Xform to this one in-place. */
    _Xform2& operator*=(const _Xform2& other)
    {
        value_t* this_array        = this->as_ptr();
        const value_t* other_array = other.as_ptr();

        const value_t t0 = this_array[0] * other_array[0] + this_array[1] * other_array[2];
        const value_t t2 = this_array[2] * other_array[0] + this_array[3] * other_array[2];
        const value_t t4 = this_array[4] * other_array[0] + this_array[5] * other_array[2] + other_array[4];

        this_array[1] = this_array[0] * other_array[1] + this_array[1] * other_array[3];
        this_array[3] = this_array[2] * other_array[1] + this_array[3] * other_array[3];
        this_array[5] = this_array[4] * other_array[1] + this_array[5] * other_array[3] + other_array[5];
        this_array[0] = t0;
        this_array[2] = t2;
        this_array[4] = t4;
        return *this;
    }

    /** Premultiplies the other Xform with this one in-place (same as `*this = other * this`). */
    _Xform2& premult(const _Xform2& other)
    {
        value_t* this_array        = this->as_ptr();
        const value_t* other_array = other.as_ptr();

        const value_t t0 = other_array[0] * this_array[0] + other_array[1] * this_array[2];
        const value_t t1 = other_array[0] * this_array[1] + other_array[1] * this_array[3];
        const value_t t3 = other_array[2] * this_array[1] + other_array[3] * this_array[3];

        this_array[4] = other_array[4] * this_array[0] + other_array[5] * this_array[2] + this_array[4];
        this_array[5] = other_array[4] * this_array[1] + other_array[5] * this_array[3] + this_array[5];
        this_array[2] = other_array[2] * this_array[0] + other_array[3] * this_array[2];
        this_array[0] = t0;
        this_array[1] = t1;
        this_array[3] = t3;
        return *this;
    }

    /** Premultiplies the other Xform with this one (same as `*this = other * this`). */
    _Xform2 get_premult(const _Xform2& other) const
    {
        _Xform2 result = *this;
        return result.premult(other);
    }

    /** Inverts this Xform2 in-place. */
    _Xform2& invert()
    {
        *this = (*this).get_inverse();
        return *this;
    }

    /** Returns the inverse of this Xform2. */
    _Xform2 get_inverse() const
    {
        const value_t det = get_determinant();
        if (abs(det) <= precision_high<value_t>()) {
            return _Xform2::identity();
        }
        const value_t invdet = 1 / det;
        _Xform2 result;
        result[0][0] = +(rows[1][1]) * invdet;
        result[0][1] = -(rows[0][1]) * invdet;
        result[1][0] = -(rows[1][0]) * invdet;
        result[1][1] = +(rows[0][0]) * invdet;
        result[2][0] = +(rows[1][0] * rows[2][1] - rows[1][1] * rows[2][0]) * invdet;
        result[2][1] = -(rows[0][0] * rows[2][1] - rows[0][1] * rows[2][0]) * invdet;
        return result;
    }

    /** Transforms the given Vector2 in-place. */
    void transform(vector_t& vector) const
    {
        vector = {
            vector.x * rows[0][0] + vector.y * rows[1][0] + rows[2][0],
            vector.x * rows[0][1] + vector.y * rows[1][1] + rows[2][1],
        };
    }

    /** Transforms the given AABR in-place. */
    void transform(_Aabr<vector_t>& aabr) const
    {
        vector_t d0 = aabr._min;
        vector_t d1 = aabr._max;
        vector_t d2 = {aabr._min.x, aabr._max.y};
        vector_t d3 = {aabr._max.x, aabr._min.y};
        transform(d0);
        transform(d1);
        transform(d2);
        transform(d3);
        aabr._min.x = min(d0.x, d1.x, d2.x, d3.x);
        aabr._min.y = min(d0.y, d1.y, d2.y, d3.y);
        aabr._max.x = max(d0.x, d1.x, d2.x, d3.x);
        aabr._max.y = max(d0.y, d1.y, d2.y, d3.y);
    }

    /** Read-write reference to a row of the Xform2's internal matrix. */
    template <typename Row, ENABLE_IF_INT(Row)>
    vector_t& operator[](const Row row)
    {
        assert(0 <= row && row <= 2);
        return rows[row];
    }

    /** Read-write pointer to the Xform2's internal storage. */
    value_t* as_ptr() { return &rows[0][0]; }
};

//*********************************************************************************************************************/

using Xform2f = _Xform2<float>;
using Xform2d = _Xform2<double>;

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
