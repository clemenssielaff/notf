#pragma once

#include "common/vector2.hpp"
#include "common/vector3.hpp"
#include "common/vector4.hpp"

namespace notf {

//====================================================================================================================//

namespace detail {

/// @brief Transforms the given input and returns a new value.
template<typename MATRIX4, typename INPUT>
INPUT transform3(const MATRIX4&, const INPUT&);

} // namespace detail

//====================================================================================================================//

/// @brief A 3D Transformation matrix with 4x4 components.
///
/// [a, e, i, m
///  b, f, j, n
///  c, g, k, o
///  d, h, l, p]
///
/// Matrix layout is equivalent to glm's layout, which in turn is equivalent to GLSL's matrix layout for easy
/// compatiblity with OpenGL.

template<typename REAL>
struct _Matrix4 : public detail::Arithmetic<_Matrix4<REAL>, _RealVector4<REAL>, 4> {

	/// @brief Element type.
	using element_t = REAL;

	/// @brief Component type.
	using component_t = _RealVector4<element_t>;

	/// @brief Arithmetic base type.
	using super_t = detail::Arithmetic<_Matrix4<element_t>, component_t, 4>;

	// fields --------------------------------------------------------------------------------------------------------//
	using super_t::data;

	// methods -------------------------------------------------------------------------------------------------------//
	/// @brief Default constructor.
	_Matrix4() = default;

	/// @brief Value constructor defining the diagonal of the matrix.
	/// @param a    Value to put into the diagonal.
	_Matrix4(const element_t a)
	    : super_t{component_t(a, 0, 0, 0), component_t(0, a, 0, 0), component_t(0, 0, a, 0), component_t(0, 0, 0, a)} {}

	/// @brief Column-wise constructor of the matrix.
	/// @param a    First column.
	/// @param b    Second column.
	/// @param c    Third column.
	/// @param d    Fourth column.
	_Matrix4(const component_t a, const component_t b, const component_t c, const component_t d)
	    : super_t{std::move(a), std::move(b), std::move(c), std::move(d)} {}

	/// @brief Element-wise constructor.
	_Matrix4(const element_t a, const element_t b, const element_t c, const element_t d, const element_t e,
	         const element_t f, const element_t g, const element_t h, const element_t i, const element_t j,
	         const element_t k, const element_t l, const element_t m, const element_t n, const element_t o,
	         const element_t p)
	    : super_t{component_t(a, b, c, d), component_t(e, f, g, h), component_t(i, j, k, l), component_t(m, n, o, p)} {}

	/// @brief The identity matrix.
	static _Matrix4 identity() { return _Matrix4(1); }

	/// @brief An element-wise translation matrix.
	/// @param x    X transformation.
	/// @param y    Y transformation.
	/// @param z    Z transformation (default to 0).
	static _Matrix4 translation(const element_t x, const element_t y, const element_t z = 0) {
		return _Matrix4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1);
	}

	/// @brief A 2d translation matrix.
	/// @param translation  2D translation vector.
	static _Matrix4 translation(const _RealVector2<element_t>& t) { return _Matrix4::translation(t.x(), t.y()); }

	/// @brief A 3d translation matrix.
	/// @param translation  3D translation vector.
	static _Matrix4 translation(const _RealVector3<element_t>& t) { return _Matrix4::translation(t.x(), t.y(), t.z()); }

	/// @brief A rotation matrix.
	/// @param axis     Rotation axis.
	/// @param radians  Rotation in radians.
	static _Matrix4 rotation(const _RealVector3<element_t> axis, const element_t radians) {
		return identity().rotate(std::move(axis), radians);
	}

	/// @brief A uniform scale matrix.
	/// @param factor   Uniform scale factor.
	static _Matrix4 scaling(const element_t s) { return _Matrix4(s, 0, 0, 0, 0, s, 0, 0, 0, 0, s, 0, 0, 0, 0, 1); }

	/// @brief A non-uniform scale matrix.
	/// @param x    X component of the scale vector.
	/// @param y    Y component of the scale vector.
	/// @param z    Z component of the scale vector.
	static _Matrix4 scaling(const element_t x, const element_t y, const element_t z = 1) {
		return _Matrix4(x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, 0, 0, 0, 0, 1);
	}

	/// @brief Creates a perspective transformation.
	/// @param fov       Horizontal field of view in radians.
	/// @param aspect    Aspect ratio (width / height)
	/// @param znear     Distance to the near plane in z direction.
	/// @param zfar      Distance to the far plane in z direction.
	static _Matrix4 perspective(const element_t fov, const element_t aspect, element_t near, element_t far) {
		// near and far planes must be >= 1
		near = max(near, 1);
		far  = max(near, far);

		_Matrix4 result = _Matrix4::zero();
		if (std::abs(aspect) <= precision_high<element_t>() || std::abs(far - near) <= precision_high<element_t>()) {
			return result;
		}

		const element_t tan_half_fov = tan(fov / 2);

		result[0][0] = 1 / (aspect * tan_half_fov);
		result[1][1] = 1 / tan_half_fov;
		result[2][3] = -1;
		result[2][2] = -(far + near) / (far - near);
		result[3][2] = -(2 * far * near) / (far - near);

		return result;
	}

	/// @brief Creates an orthographic transformation matrix.
	/// @param left     Vertical distance from screen zero to the left edge of the projection.
	/// @param right    Vertical distance from screen zero to the right edge of the projection.
	/// @param bottom   Vertical distance from screen zero to the bottom edge of the projection.
	/// @param top      Vertical distance from screen zero to the top edge of the projection.
	/// @param znear    Distance to the near plane in z direction.
	/// @param zfar     Distance to the far plane in z direction.
	static _Matrix4 orthographic(const element_t left, const element_t right, const element_t bottom,
	                             const element_t top, element_t near, element_t far) {
		// near and far planes must be >= 1
		near = max(near, 1);
		far  = max(near, far);

		const element_t width  = right - left;
		const element_t height = top - bottom;
		const element_t depth  = far - near;

		_Matrix4 result = identity();
		if (std::abs(width) <= precision_high<element_t>() || std::abs(height) <= precision_high<element_t>()
		    || std::abs(depth) <= precision_high<element_t>()) {
			return result;
		}

		result[0][0] = 2 / width;
		result[1][1] = 2 / height;
		result[3][0] = -(right + left) / width;
		result[3][1] = -(top + bottom) / height;
		result[2][2] = -2 / depth;
		result[3][2] = -(near + far) / depth;

		return result;
	}

	/// @brief The translation part of this matrix.
	const component_t& translation() const { return data[3]; }

	/// @brief Concatenation of two transformation matrices.
	/// @param other    Transformation to concatenate.
	_Matrix4 operator*(const _Matrix4& other) const {
		_Matrix4 result;
		result[0] = data[0] * other[0][0] + data[1] * other[0][1] + data[2] * other[0][2] + data[3] * other[0][3];
		result[1] = data[0] * other[1][0] + data[1] * other[1][1] + data[2] * other[1][2] + data[3] * other[1][3];
		result[2] = data[0] * other[2][0] + data[1] * other[2][1] + data[2] * other[2][2] + data[3] * other[2][3];
		result[3] = data[0] * other[3][0] + data[1] * other[3][1] + data[2] * other[3][2] + data[3] * other[3][3];
		return result;
	}

	/// @brief Concatenation of another transformation matrix to this one in-place.
	/// @param other    Transformation to concatenate.
	_Matrix4& operator*=(const _Matrix4& other) {
		*this = *this * other;
		return *this;
	}

	/// @brief Concatenate this to another another transformation matrix.
	/// @param other    Transformation to concatenate to.
	_Matrix4& premult(const _Matrix4& other) {
		*this = other * *this;
		return *this;
	}

	/// @brief Translates this transformation by a given delta vector.
	/// @param delta    Delta translation.
	_Matrix4 translate(const component_t& delta) const {
		_Matrix4 result;
		result[0] = data[0];
		result[1] = data[1];
		result[2] = data[2];
		result[3] = data[0] * delta[0] + data[1] * delta[1] + data[2] * delta[2] + data[3];
		return result;
	}

	/// @brief Rotates the transformation by a given angle in radians over a given axis.
	/// @param axis     Rotation axis.
	/// @param radians  Rotation in radians.
	_Matrix4 rotate(_RealVector3<element_t> axis, const element_t radian) const {
		const element_t cos_angle = cos(radian);
		const element_t sin_angle = sin(radian);

		axis.normalize();
		const _RealVector3<element_t> temp = axis * (1 - cos_angle);

		_Matrix4 rotation;
		rotation[0][0] = cos_angle + temp[0] * axis[0];
		rotation[0][1] = temp[0] * axis[1] + sin_angle * axis[2];
		rotation[0][2] = temp[0] * axis[2] - sin_angle * axis[1];

		rotation[1][0] = temp[1] * axis[0] - sin_angle * axis[2];
		rotation[1][1] = cos_angle + temp[1] * axis[1];
		rotation[1][2] = temp[1] * axis[2] + sin_angle * axis[0];

		rotation[2][0] = temp[2] * axis[0] + sin_angle * axis[1];
		rotation[2][1] = temp[2] * axis[1] - sin_angle * axis[0];
		rotation[2][2] = cos_angle + temp[2] * axis[2];

		_Matrix4 result;
		result[0] = data[0] * rotation[0][0] + data[1] * rotation[0][1] + data[2] * rotation[0][2];
		result[1] = data[0] * rotation[1][0] + data[1] * rotation[1][1] + data[2] * rotation[1][2];
		result[2] = data[0] * rotation[2][0] + data[1] * rotation[2][1] + data[2] * rotation[2][2];
		result[3] = data[3];

		return result;
	}

	/// @brief Applies a non-uniform scaling to this matrix.
	/// @brief scaling  Scaling vector.
	_Matrix4 scale(const component_t& scaling) const {
		_Matrix4 result = *this;
		result[0] *= scaling[0];
		result[1] *= scaling[1];
		result[2] *= scaling[2];
		return result;
	}

	/// @brief Applies an uniform scaling to this matrix.
	/// @brief scaling  Scale factor.
	_Matrix4 scale(const element_t factor) const {
		_Matrix4 result = *this;
		result[0] *= factor;
		result[1] *= factor;
		result[2] *= factor;
		return result;
	}

	/// @brief Returns the inverse of this matrix.
	_Matrix4 get_inverse() const {
		element_t coef00 = data[2][2] * data[3][3] - data[3][2] * data[2][3];
		element_t coef02 = data[1][2] * data[3][3] - data[3][2] * data[1][3];
		element_t coef03 = data[1][2] * data[2][3] - data[2][2] * data[1][3];

		element_t coef04 = data[2][1] * data[3][3] - data[3][1] * data[2][3];
		element_t coef06 = data[1][1] * data[3][3] - data[3][1] * data[1][3];
		element_t coef07 = data[1][1] * data[2][3] - data[2][1] * data[1][3];

		element_t coef08 = data[2][1] * data[3][2] - data[3][1] * data[2][2];
		element_t coef10 = data[1][1] * data[3][2] - data[3][1] * data[1][2];
		element_t coef11 = data[1][1] * data[2][2] - data[2][1] * data[1][2];

		element_t coef12 = data[2][0] * data[3][3] - data[3][0] * data[2][3];
		element_t coef14 = data[1][0] * data[3][3] - data[3][0] * data[1][3];
		element_t coef15 = data[1][0] * data[2][3] - data[2][0] * data[1][3];

		element_t coef16 = data[2][0] * data[3][2] - data[3][0] * data[2][2];
		element_t coef18 = data[1][0] * data[3][2] - data[3][0] * data[1][2];
		element_t coef19 = data[1][0] * data[2][2] - data[2][0] * data[1][2];

		element_t coef20 = data[2][0] * data[3][1] - data[3][0] * data[2][1];
		element_t coef22 = data[1][0] * data[3][1] - data[3][0] * data[1][1];
		element_t coef23 = data[1][0] * data[2][1] - data[2][0] * data[1][1];

		component_t fac0(coef00, coef00, coef02, coef03);
		component_t fac1(coef04, coef04, coef06, coef07);
		component_t fac2(coef08, coef08, coef10, coef11);
		component_t fac3(coef12, coef12, coef14, coef15);
		component_t fac4(coef16, coef16, coef18, coef19);
		component_t fac5(coef20, coef20, coef22, coef23);

		component_t vec0(data[1][0], data[0][0], data[0][0], data[0][0]);
		component_t vec1(data[1][1], data[0][1], data[0][1], data[0][1]);
		component_t vec2(data[1][2], data[0][2], data[0][2], data[0][2]);
		component_t vec3(data[1][3], data[0][3], data[0][3], data[0][3]);

		component_t inv0(vec1 * fac0 - vec2 * fac1 + vec3 * fac2);
		component_t inv1(vec0 * fac0 - vec2 * fac3 + vec3 * fac4);
		component_t inv2(vec0 * fac1 - vec1 * fac3 + vec3 * fac5);
		component_t inv3(vec0 * fac2 - vec1 * fac4 + vec2 * fac5);

		component_t signA(+1, -1, +1, -1);
		component_t signB(-1, +1, -1, +1);
		_Matrix4 inverse(inv0 * signA, inv1 * signB, inv2 * signA, inv3 * signB);

		component_t row0(inverse[0][0], inverse[1][0], inverse[2][0], inverse[3][0]);
		component_t dot0(data[0] * row0);
		element_t dot1 = (dot0[0] + dot0[1]) + (dot0[2] + dot0[3]);

		return inverse / dot1;
	}

	/// @brief Returns the transformed copy of a given vector.
	/// @param vector   Vector to transform.
	_RealVector4<element_t> transform(const _RealVector4<element_t>& vector) const {
		// this is the operation matrix * vector
		const _RealVector4<element_t> mul0 = data[0] * _RealVector4<element_t>::fill(vector[0]);
		const _RealVector4<element_t> mul1 = data[1] * _RealVector4<element_t>::fill(vector[1]);
		const _RealVector4<element_t> mul2 = data[2] * _RealVector4<element_t>::fill(vector[2]);
		const _RealVector4<element_t> mul3 = data[3] * _RealVector4<element_t>::fill(vector[3]);
		return (mul0 + mul1) + (mul2 + mul3);
	}

	/// @brief Returns the transformed copy of a given vector.
	/// @param vector   Vector to transform.
	_RealVector2<element_t> transform(const _RealVector2<element_t>& vector) const {
		const _RealVector4<element_t> mul0   = data[0] * _RealVector4<element_t>::fill(vector[0]);
		const _RealVector4<element_t> mul1   = data[1] * _RealVector4<element_t>::fill(vector[1]);
		const _RealVector4<element_t> result = (mul0 + mul1) + data[3];
		return _RealVector2<element_t>(result.x(), result.y());
	}

	/// @brief Return the transformed copy of the value.
	/// @param value    Value to transform.
	template<typename T>
	T transform(const T& value) const {
		return detail::transform3(*this, value);
	}
};

//====================================================================================================================//

using Matrix4f = _Matrix4<float>;
using Matrix4d = _Matrix4<double>;

//====================================================================================================================//

/// @brief Prints the contents of a matrix into a std::ostream.
/// @param out      Output stream, implicitly passed with the << operator.
/// @param matrix   Matrix to print.
/// @return Output stream for further output.
template<typename REAL>
std::ostream& operator<<(std::ostream& out, const notf::_Matrix4<REAL>& matrix);

} // namespace notf

//====================================================================================================================//

namespace std {

/// @brief std::hash specialization for notf::_Matrix4.
template<typename Real>
struct hash<notf::_Matrix4<Real>> {
	size_t operator()(const notf::_Matrix4<Real>& matrix) const {
		return notf::hash(matrix[0], matrix[1], matrix[2], matrix[3]);
	}
};
} // namespace std

//#ifndef NOTF_NO_SIMD
//#include "common/simd/simd_xform3.hpp"
//#endif