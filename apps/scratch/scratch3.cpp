#include <array>
#include <iostream>
#include <iomanip>
#include <cmath>

static const uint print_precision = 4;

struct v2;
struct mat2x3;
v2 operator*(const v2& lhs, const mat2x3& rhs);
mat2x3 operator*(const mat2x3& lhs, const mat2x3& rhs);

double to_print(const double value){
    static constexpr double precision = 1e-4;
    double result = std::round(value / precision) * precision;
    if(std::abs(result) < precision){ result = 0; } // no negative zero
    return result;
}

/// https://en.wikipedia.org/wiki/Small-angle_approximation#Error_of_the_approximations
template<class T>
T fast_cos(const T& radians){
    if(radians < 0.664){
        return T(1) - (radians * radians) / T(2);
    } else {
        return std::cos(radians);
    }
}

template<class T>
T fast_sin(const T& radians){
    if(radians < 0.24){
        return radians;
    } else {
        return std::sin(radians);
    }
}

template<class T>
T fast_tan(const T& radians){
    if(radians < 0.176){
        return radians;
    } else {
        return std::tan(radians);
    }
}

struct v2 {
    v2& normalize(){
        const double length_sq = x*x + y*y;
        if(std::abs(length_sq - 1) > 1e-8){
            const double length = std::sqrt(length_sq);
            x /= length;
            y /= length;
        }
        return *this;
    }

    bool operator==(const v2& other) const { return other.x == x && other.y == y; }   /// Comparison
    bool operator!=(const v2& other) const { return other.x != x || other.y != y; }   ///

    double x;   /// Fields
    double y;   ///
};
std::ostream& operator<<(std::ostream& out, const v2& vec) {
    return out << std::setprecision(print_precision+1) << "v2(" << to_print(vec.x) << ", " << to_print(vec.y) << ")";
}

/// V2 subtraction.
v2 operator-(const v2& lhs, const v2& rhs){
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}
/// V2 scalar multiplication.
v2 operator*(const v2& vec, const double factor){
    return {vec.x * factor, vec.y * factor};
}


struct mat2x3 {
    static mat2x3 diagonal(const double value){ return {v2{value, 0}, v2{0, value}, v2{0, 0}}; }
    static mat2x3 diagonal(const double first, const double second){ return {v2{first, 0}, v2{0, second}, v2{0, 0}}; }

    /// The identity matrix.
    static mat2x3 identity(){ return diagonal(1); }

    /// Matrix with each element set to zero.
    static mat2x3 zero() { return diagonal(0); }

    /// 2D translation matrix.
    /// @param vec		Translation vector.
    static mat2x3 translation(const v2& vec){ return {v2{1, 0}, v2{0, 1}, vec}; }
    static mat2x3 translation(const double x, const double y){ return {v2{1, 0}, v2{0, 1}, v2{x, y}}; }

    /// Matrix representing a counterclockwise 2D rotation around the origin.
    /// See https://en.wikipedia.org/wiki/Transformation_matrix#Rotation
    /// @param radian   Angle to rotate counterclockwise in radian.
    static mat2x3 rotation(const double radian){
        const double sin = fast_sin(radian);
        const double cos = fast_cos(radian);
        return {v2{cos, sin}, v2{-sin, cos}, v2{0, 0}};
    }

    /// Matrix representing a counterclockwise 2D rotation around an arbitrary pivot point
    /// This is a concatenation of the following:
    /// 1. Translate the coordinates so that `pivot` is at the origin.
    /// 2. Rotate.
    /// 3. Translate back.
    /// @param radian   Angle to rotate counterclockwise in radian.
    /// @param pivot	Pivot point to rotate around.
    static mat2x3 rotation(const double radian, const v2& pivot){
        //return translation(v2{pivot}) * rotation(radian) * translation(v2{-pivot.x, -pivot.y});
        const double sin = fast_sin(radian);
        const double cos = fast_cos(radian);
        return {v2{cos, sin}, v2{-sin, cos}, v2{pivot.x - cos*pivot.x + sin*pivot.y,
                                                pivot.y - sin*pivot.x - cos*pivot.y}};
    }

    /// @{
    /// Scale transformation, in one or both directions.
    ///	See https://en.wikipedia.org/wiki/Transformation_matrix#Stretching
    /// @param factor	Scale factor.
    static mat2x3 scale_x(const double factor) { return diagonal(factor, 1); }
    static mat2x3 scale_y(const double factor) { return diagonal(1, factor); }
    static mat2x3 scale(const double factor) { return diagonal(factor); }
    /// @}

    /// Squeeze transformation.
    /// See https://en.wikipedia.org/wiki/Transformation_matrix#Squeezing
    /// @param factor	Squeeze factor.
    static mat2x3 squeeze(const double factor) {
        if(factor == 0) { return zero(); }
        else { return diagonal(factor, 1/factor); }
    }

    /// @{
    /// Shear transformation.
    /// See https://en.wikipedia.org/wiki/Transformation_matrix#Shearing
    /// @param distance	Shear distance.
    static mat2x3 shear_x(const double distance) { return {v2{1, 0}, v2{distance, 1}, v2{0, 0}}; }
    static mat2x3 shear_y(const double distance) { return {v2{1, distance}, v2{0, 1}, v2{0, 0}}; }
    /// @}

    /// Reflection over a line that passes through the origin at the given angle in radian.
    /// @param angle	Counterclockwise angle of the reflection line in radian.
    static mat2x3 reflection(const double angle) {
        const double sin = fast_sin(2*angle);
        const double cos = fast_cos(2*angle);
        return {v2{cos, sin}, v2{sin, -cos}, v2{0, 0}};
    }

    /// Reflection over a line defined by two points.
    /// @param start		First point on the reflection line.
    /// @param direction 	Second point on the reflection line.
    /// This is a concatenation of the following:
    /// 1. Translate the coordinates so that `start` is at the origin.
    /// 2. Rotate so that `direction-start` aligns with the x-axis.
    /// 3. Reflect about the x-axis.
    /// 4. Rotate back.
    /// 5. Translate back.
    static mat2x3 reflection(const v2& start, v2 direction) {
        direction = direction-start;
        if(direction.y == 0) { return scale_x(-1); } // line is horizontal
        if(direction.x == 0) { return scale_y(-1); } // line is vertical
        {
            const double mag_sq = direction.x * direction.x + direction.y * direction.y;
            if(mag_sq < 1e-8){ return identity(); }
            if(std::abs(mag_sq - 1) > 1e-8){
                const double mag = std::sqrt(mag_sq);
                direction.x /= mag;
                direction.y /= mag;
            }
        }
        const double u = (direction.x * direction.x) - (direction.y * direction.y);
        const double v = (direction.x * direction.y) + (direction.x * direction.y);
        return {v2{u, v}, v2{v, -u}, v2{start.x - u*start.x - v*start.y,
                                        start.y + u*start.y - v*start.x}};
    }
    static mat2x3 reflection(v2 direction) {
        return reflection(v2{0, 0}, std::move(direction));
    }

    /// Determinant of an affine 2D transformation matrix:
    /// |a c x|
    ///	|b d y| => a*d*1 + c*y*0 + x*b*0 - 0*d*x - 0*y*a - 1*b*c => a*d - b*c
    /// |0 0 1|
    double get_determinant() const { return data[0].x * data[1].y - data[0].y * data[1].x; }

    /// A 2D transformation preserves the area of a polygon if its determinant is +/-1.
    bool is_preserving_area() const { return std::abs(get_determinant()  - 1) < 1e-8; }


    bool operator==(const mat2x3& other) const { return other.data == data; } /// Comparison
    bool operator!=(const mat2x3& other) const { return other.data != data; } ///

    v2& operator[](size_t i){ return data[i]; }               /// Access
    const v2& operator[](size_t i) const { return data[i]; }  ///

    std::array<v2, 3> data; /// Fields
};
std::ostream& operator<<(std::ostream& out, const mat2x3& mat) {
    return out << std::setprecision(print_precision+1)
               << "mat2x3((" << to_print(mat[0].x) << ", " << to_print(mat[0].y)
               << "), (" << to_print(mat[1].x) << ", " << to_print(mat[1].y)
               << "), (" << to_print(mat[2].x) << ", " << to_print(mat[2].y) << "))";
}

/// Applies the right-hand side matrix transformation to the left-hand side matrix and returns the result.
/// Inputs:
///     lhs = |a c e|, rhs = |u w y|
///           |b d f|        |v x z|
///           |0 0 1|        |0 0 1| // last row is implicit
/// Operation:
///           |u w y         h = a*u + c*v + e*0
///           |v x z         i = b*u + d*v + f*0
///           |0 0 1   with  j = a*w + c*x + e*0
///     ------+------        k = b*w + d*x + f*0
///     a c e |h j l         l = a*y + c*z + e*1
///     b d f |i k m         m = b*y + d*z + f*1
///     0 0 1 |0 0 1
///
mat2x3 operator*(const mat2x3& lhs, const mat2x3& rhs) {
    mat2x3 mat;
    mat[0].x = lhs[0].x * rhs[0].x + lhs[1].x * rhs[0].y;
    mat[0].y = lhs[0].y * rhs[0].x + lhs[1].y * rhs[0].y;

    mat[1].x = lhs[0].x * rhs[1].x + lhs[1].x * rhs[1].y;
    mat[1].y = lhs[0].y * rhs[1].x + lhs[1].y * rhs[1].y;

    mat[2].x = lhs[0].x * rhs[2].x + lhs[1].x * rhs[2].y + lhs[2].x;
    mat[2].y = lhs[0].y * rhs[2].x + lhs[1].y * rhs[2].y + lhs[2].y;
    return mat;
}

/// Applies the right-hand side matrix transformation to the left-hand side vector and returns the result.
///
/// Note that in mathematical notation, the matrix would be on the right of the vector.
/// However, since matrices usually represent transformations and the order of transformations in mathematical notation runs
/// counterintuitive to the order as written in code (A*B*C would mean that marix `C` is transformed first by `B` and then by `C`),
/// we flip the order of arguments. This way, `v*A*B*C` means (vector `v` transformed first by `A`, then by `B` and eventually by `C`).
///
/// Inputs:
///     lhs = |a|, rhs = |p r t|
///           |b|        |q s u|
///           |1|        |0 0 1| // last row is implicit
/// Operation:
///           |a
///           |b
///           |1   with  x = p*a + r*b + t
///     ------+--        y = q*a + s*b + u
///     p r t |x
///     q s u |y
///     0 0 1 |1
///
v2 operator*(const v2& lhs, const mat2x3& rhs) {
    v2 vec;
    vec.x = rhs[0].x * lhs.x + rhs[1].x * lhs.y + rhs[2].x;
    vec.y = rhs[0].y * lhs.x + rhs[1].y * lhs.y + rhs[2].y;
    return vec;
}

int main(){
    v2 pos {0, 3};
    std::cout << pos * mat2x3::reflection(v2{1,1}) << std::endl;
    return 0;
}
