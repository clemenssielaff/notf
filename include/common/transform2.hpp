#pragma once

#include "assert.h"
#include <array>
#include <iosfwd>

#include "common/real.hpp"
#include "common/vector2.hpp"

namespace signal {

/// \brief A 2D Transformation Matrix with 3x3 components.
/// Only the first two rows are actually stored though, the last row is a static constant.
///
struct Transform2 {

private: // class
    /// \brief Private Row class to enforce bound checks during debug.
    ///
    struct Row {

        friend struct Transform2;

        std::array<Real, 3> values;

        /// \brief Read-access to a value in the Row.
        Real operator[](unsigned char col) const
        {
            assert(col < 3);
            return values[col];
        }

    private: // methods for Transform2
        /// \brief Write access to the Row's data.
        Real& operator[](unsigned char col)
        {
            return values[col];
        }
    };

public: // methods
    //  FIELDS  ///////////////////////////////////////////////////////////////////////////////////////////////////////

    /// \brief Matrix values.
    std::array<Row, 2> rows;

    //  HOUSEHOLD  ////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Do not implement the default methods, so this data structure remains a POD.
    Transform2() = default; // Default Constructor.
    ~Transform2() = default; // Destructor.
    Transform2(const Transform2& other) = default; // Copy Constructor.
    Transform2& operator=(const Transform2& other) = default; // Assignment Operator.

    //  STATIC CONSTRUCTORS  //////////////////////////////////////////////////////////////////////////////////////////

    /// \brief The identity matrix.
    static Transform2 identity()
    {
        return {{{{{{1, 0, 0}}},
                  {{{0, 1, 0}}}}}};
    }

    /// \brief A translation matrix.
    /// \param vector   Translation vector.
    static Transform2 translation(const Vector2& vector)
    {
        return {{{{{{0, 0, vector.x}}},
                  {{{0, 0, vector.y}}}}}};
    }

    /// \brief A rotation matrix.
    /// \param radians  Counter-clockwise rotation in radians.
    static Transform2 rotation(const Real radians)
    {
        const Real sine = sin(radians);
        const Real cosine = cos(radians);
        return {{{{{{cosine, sine, 0}}},
                  {{{-sine, cosine, 0}}}}}};
    }

    /// \brief A uniform scale matrix.
    /// \param factor   Uniform scale factor.
    static Transform2 scale(const Real factor)
    {
        return {{{{{{factor, 0, 0}}},
                  {{{0, factor, 0}}}}}};
    }

    /// \brief A non-uniform scale matrix.
    /// \param vector   Non-uniform scale vector.
    static Transform2 scale(const Vector2& vector)
    {
        return {{{{{{vector.x, 0, 0}}},
                  {{{0, vector.y, 0}}}}}};
    }

    /// \brief A non-uniform skew matrix.
    /// \param vector   Non-uniform skew vector.
    static Transform2 skew(const Vector2& vector)
    {
        return {{{{{{1, tan(vector.x), 0}}},
                  {{{tan(vector.y), 1, 0}}}}}};
    }

    //  INSPECTION  ///////////////////////////////////////////////////////////////////////////////////////////////////

    /// \brief Read access to the Matrix.
    const Row& operator[](unsigned char row) const
    {
        static const Row last_row{{{0, 0, 1}}};
        assert(row < 3);
        if (row == 2) {
            return last_row;
        }
        else {
            return rows[row];
        }
    }

    /// \brief Matrix multiplication of this Matrix with another.
    Transform2 operator*(const Transform2& other) const
    {
        Transform2 result;
        result[0][0] = (rows[0][0] * other[0][0]) + (rows[0][1] * other[1][0]);
        result[0][1] = (rows[0][0] * other[0][1]) + (rows[0][1] * other[1][1]);
        result[0][2] = rows[0][2] + (rows[0][0] * other[0][2]) + (rows[0][1] * other[1][2]);
        result[1][0] = (rows[1][0] * other[0][0]) + (rows[1][1] * other[1][0]);
        result[1][1] = (rows[1][0] * other[0][1]) + (rows[1][1] * other[1][1]);
        result[1][2] = rows[1][2] + (rows[1][0] * other[0][2]) + (rows[1][1] * other[1][2]);
        return result;
    }

    /// \brief In-place Matrix multiplication of this Matrix with another.
    Transform2& operator*=(const Transform2& other)
    {
        Transform2 temp = *this * other;
        std::swap((*this).rows, temp.rows);
        return *this;
    }

private: // methods
    /// \brief Write access to the Matrix.
    Row& operator[](unsigned char row)
    {
        return rows[row];
    }
};

} // namespace signal
