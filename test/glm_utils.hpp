#include "catch.hpp"

#include "common/matrix3.hpp"
#include "common/matrix4.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat3x2.hpp>
#include <glm/mat4x4.hpp>

inline void compare_mat4(const notf::Matrix4f& my, const glm::mat4& their) {
	for (size_t col = 0; col < 4; ++col) {
		for (size_t row = 0; row < 4; ++row) {
			REQUIRE(std::abs(my[col][row] - their[static_cast<int>(col)][static_cast<int>(row)])
			        < notf::precision_high<float>());
		}
	}
}

inline void compare_mat2(const notf::Matrix3f& my, const glm::mat4& their) {
	for (size_t col = 0; col < 2; ++col) {
		REQUIRE(std::abs(my[col][0] - their[static_cast<int>(col)][static_cast<int>(0)])
		        < notf::precision_high<float>());
		REQUIRE(std::abs(my[col][1] - their[static_cast<int>(col)][static_cast<int>(1)])
		        < notf::precision_high<float>());
		REQUIRE(std::abs(their[static_cast<int>(col)][static_cast<int>(2)]) < notf::precision_high<float>());
		REQUIRE(std::abs(their[static_cast<int>(col)][static_cast<int>(3)]) < notf::precision_high<float>());
	}

	REQUIRE(std::abs(their[static_cast<int>(2)][static_cast<int>(0)]) < notf::precision_high<float>());
	REQUIRE(std::abs(their[static_cast<int>(2)][static_cast<int>(1)]) < notf::precision_high<float>());
	REQUIRE(std::abs(1 - their[static_cast<int>(2)][static_cast<int>(2)]) < notf::precision_high<float>());
	REQUIRE(std::abs(their[static_cast<int>(2)][static_cast<int>(3)]) < notf::precision_high<float>());

	REQUIRE(std::abs(my[2][0] - their[static_cast<int>(3)][static_cast<int>(0)]) < notf::precision_high<float>());
	REQUIRE(std::abs(my[2][1] - their[static_cast<int>(3)][static_cast<int>(1)]) < notf::precision_high<float>());
	REQUIRE(std::abs(their[static_cast<int>(3)][static_cast<int>(2)]) < notf::precision_high<float>());
	REQUIRE(std::abs(1 - their[static_cast<int>(3)][static_cast<int>(3)]) < notf::precision_high<float>());
}

inline void compare_vec4(const notf::Vector4f& my, const glm::vec4& their) {
	for (size_t col = 0; col < 4; ++col) {
		REQUIRE(std::abs(my[col] - their[static_cast<int>(col)]) < notf::precision_high<float>());
	}
}

inline glm::mat4 to_glm_mat4(const notf::Matrix4f& matrix) {
	glm::mat4 result;
	for (size_t col = 0; col < 4; ++col) {
		for (size_t row = 0; row < 4; ++row) {
			result[static_cast<int>(col)][static_cast<int>(row)] = matrix[col][row];
		}
	}
	return result;
}
