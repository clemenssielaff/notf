#include "catch.hpp"
#include "test_utils.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

using namespace notf;
using namespace glm;

namespace { // anonymous

inline void compare_mat4(const notf::Xform4f& my, const mat4& their)
{
    for (size_t col = 0; col < 4; ++col) {
        for (size_t row = 0; row < 4; ++row) {
            REQUIRE(std::abs(my[col][row] - their[static_cast<int>(col)][static_cast<int>(row)]) < precision_high<float>());
        }
    }
}

inline void compare_vec4(const notf::Vector4f& my, const vec4& their)
{
    for (size_t col = 0; col < 4; ++col) {
        REQUIRE(std::abs(my[col] - their[static_cast<int>(col)]) < precision_high<float>());
    }
}

glm::mat4 to_glm_mat4(const notf::Xform4f& matrix)
{
    glm::mat4 result;
    for (size_t col = 0; col < 4; ++col) {
        for (size_t row = 0; row < 4; ++row) {
            result[static_cast<int>(col)][static_cast<int>(row)] = matrix[col][row];
        }
    }
    return result;
}

} // namespace anonymous

SCENARIO("Xform4 base tests", "[common][xform4]")
{
    WHEN("you want to create a Xform4")
    {
        THEN("you can use the element-wise constructor")
        {
            const Xform4f matrix(1.f, 2.f, 3.f, 4.f,
                                 5.f, 6.f, 7.f, 8.f,
                                 9.f, 10.f, 11.f, 12.f,
                                 13.f, 14.f, 15.f, 16.f);
            REQUIRE(matrix[0][0] == approx(1.));
            REQUIRE(matrix[0][1] == approx(2.));
            REQUIRE(matrix[0][2] == approx(3.));
            REQUIRE(matrix[0][3] == approx(4.));
            REQUIRE(matrix[1][0] == approx(5.));
            REQUIRE(matrix[1][1] == approx(6.));
            REQUIRE(matrix[1][2] == approx(7.));
            REQUIRE(matrix[1][3] == approx(8.));
            REQUIRE(matrix[2][0] == approx(9.));
            REQUIRE(matrix[2][1] == approx(10.));
            REQUIRE(matrix[2][2] == approx(11.));
            REQUIRE(matrix[2][3] == approx(12.));
            REQUIRE(matrix[3][0] == approx(13.));
            REQUIRE(matrix[3][1] == approx(14.));
            REQUIRE(matrix[3][2] == approx(15.));
            REQUIRE(matrix[3][3] == approx(16.));
        }
        THEN("you can use the element-wise initializer list")
        {
            const Xform4f matrix{1.f, 2.f, 3.f, 4.f,
                                 5.f, 6.f, 7.f, 8.f,
                                 9.f, 10.f, 11.f, 12.f,
                                 13.f, 14.f, 15.f, 16.f};
            REQUIRE(matrix[0][0] == approx(1.));
            REQUIRE(matrix[0][1] == approx(2.));
            REQUIRE(matrix[0][2] == approx(3.));
            REQUIRE(matrix[0][3] == approx(4.));
            REQUIRE(matrix[1][0] == approx(5.));
            REQUIRE(matrix[1][1] == approx(6.));
            REQUIRE(matrix[1][2] == approx(7.));
            REQUIRE(matrix[1][3] == approx(8.));
            REQUIRE(matrix[2][0] == approx(9.));
            REQUIRE(matrix[2][1] == approx(10.));
            REQUIRE(matrix[2][2] == approx(11.));
            REQUIRE(matrix[2][3] == approx(12.));
            REQUIRE(matrix[3][0] == approx(13.));
            REQUIRE(matrix[3][1] == approx(14.));
            REQUIRE(matrix[3][2] == approx(15.));
            REQUIRE(matrix[3][3] == approx(16.));
        }
        THEN("you can use the element-wise constructor with mixed scalar types")
        {
            const Xform4f matrix(1.f, 2.f, 3.f, 4,
                                 5.f, 6.f, 7.f, 8.,
                                 9.f, 10.f, 11.f, 12.l,
                                 13.f, 14.f, 15.f, 16.f);
            REQUIRE(matrix[0][0] == approx(1.));
            REQUIRE(matrix[0][1] == approx(2.));
            REQUIRE(matrix[0][2] == approx(3.));
            REQUIRE(matrix[0][3] == approx(4.));
            REQUIRE(matrix[1][0] == approx(5.));
            REQUIRE(matrix[1][1] == approx(6.));
            REQUIRE(matrix[1][2] == approx(7.));
            REQUIRE(matrix[1][3] == approx(8.));
            REQUIRE(matrix[2][0] == approx(9.));
            REQUIRE(matrix[2][1] == approx(10.));
            REQUIRE(matrix[2][2] == approx(11.));
            REQUIRE(matrix[2][3] == approx(12.));
            REQUIRE(matrix[3][0] == approx(13.));
            REQUIRE(matrix[3][1] == approx(14.));
            REQUIRE(matrix[3][2] == approx(15.));
            REQUIRE(matrix[3][3] == approx(16.));
        }
        THEN("you can create an identity matrix")
        {
            const Xform4f matrix = Xform4f::identity();
            REQUIRE(matrix[0][0] == approx(1.));
            REQUIRE(matrix[0][1] == approx(0.));
            REQUIRE(matrix[0][2] == approx(0.));
            REQUIRE(matrix[0][3] == approx(0.));
            REQUIRE(matrix[1][0] == approx(0.));
            REQUIRE(matrix[1][1] == approx(1.));
            REQUIRE(matrix[1][2] == approx(0.));
            REQUIRE(matrix[1][3] == approx(0.));
            REQUIRE(matrix[2][0] == approx(0.));
            REQUIRE(matrix[2][1] == approx(0.));
            REQUIRE(matrix[2][2] == approx(1.));
            REQUIRE(matrix[2][3] == approx(0.));
            REQUIRE(matrix[3][0] == approx(0.));
            REQUIRE(matrix[3][1] == approx(0.));
            REQUIRE(matrix[3][2] == approx(0.));
            REQUIRE(matrix[3][3] == approx(1.));
        }
        THEN("you can create an translation matrix")
        {
            const Vector4f translation = random_vector<Vector4f>();
            const Xform4f matrix       = Xform4f::translation(translation);
            mat4 their                 = translate(mat4(1.0f), vec3(translation.x(), translation.y(), translation.z()));
            compare_mat4(matrix, their);
        }
        THEN("you can create a rotation matrix")
        {
            const Vector4f axis  = random_vector<Vector4f>();
            const float angle    = random_radian<float>();
            const Xform4f matrix = Xform4f::rotation(angle, axis);
            mat4 their           = rotate(mat4(1.0f), angle, vec3(axis.x(), axis.y(), axis.z()));
            compare_mat4(matrix, their);
        }
        THEN("you can create a uniform scale matrix")
        {
            const float factor   = random_number(0.0001f, 1000.f);
            const Xform4f matrix = Xform4f::scaling(factor);
            mat4 their           = scale(mat4(1.0f), vec3(factor, factor, factor));
            compare_mat4(matrix, their);
        }
        THEN("you can create a non-uniform scale matrix")
        {
            const Vector4f factor = random_vector<Vector4f>();
            const Xform4f matrix  = Xform4f::scaling(factor);
            mat4 their            = scale(mat4(1.0f), vec3(factor.x(), factor.y(), factor.z()));
            compare_mat4(matrix, their);
        }
    }

    WHEN("you want to work with multiple Xform4s")
    {
        THEN("you can concatenate them by multiplication")
        {
            const Xform4f a    = random_matrix<Xform4f>(-10, 10);
            const Xform4f b    = random_matrix<Xform4f>(-10, 10);
            const Xform4f mine = a * b;
            mat4 theirs        = to_glm_mat4(a) * to_glm_mat4(b);
            compare_mat4(mine, theirs);
        }
    }

    WHEN("you want to transform with an Xform4")
    {
        THEN("you can transform a vector")
        {
            const Vector4f vec  = random_vector<Vector4f>();
            const Xform4f xform = random_matrix<Xform4f>(-10, 10);

            const Vector4f mine = xform.transform(vec);
            glm::vec4 theirs = glm::vec4(vec.x(), vec.y(), vec.z(), vec.w()) * to_glm_mat4(xform) ;
            compare_vec4(mine, theirs);
        }
    }
}
