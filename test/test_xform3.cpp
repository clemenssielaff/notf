#include "catch.hpp"
#include "glm_utils.hpp"
#include "test_utils.hpp"

using namespace notf;

SCENARIO("Xform3 base tests", "[common][xform3]")
{
    WHEN("you want to create a Xform3")
    {
        THEN("you can use the element-wise constructor")
        {
            const Xform3f matrix(1.f, 2.f, 3.f, 4.f,
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
            const Xform3f matrix{1.f, 2.f, 3.f, 4.f,
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
            const Xform3f matrix(1.f, 2.f, 3.f, 4,
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
            const Xform3f matrix = Xform3f::identity();
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
            const Xform3f matrix       = Xform3f::translation(translation);
            glm::mat4 their                 = translate(glm::mat4(1.0f), glm::vec3(translation.x(), translation.y(), translation.z()));
            compare_mat4(matrix, their);
        }
        THEN("you can create a rotation matrix")
        {
            const Vector4f axis  = random_vector<Vector4f>();
            const float angle    = random_radian<float>();
            const Xform3f matrix = Xform3f::rotation(axis, angle);
            glm::mat4 their           = rotate(glm::mat4(1.0f), angle, glm::vec3(axis.x(), axis.y(), axis.z()));
            compare_mat4(matrix, their);
        }
        THEN("you can create a uniform scale matrix")
        {
            const float factor   = random_number(0.0001f, 1000.f);
            const Xform3f matrix = Xform3f::scaling(factor);
            glm::mat4 their           = scale(glm::mat4(1.0f), glm::vec3(factor, factor, factor));
            compare_mat4(matrix, their);
        }
        THEN("you can create a non-uniform scale matrix")
        {
            const Vector4f factor = random_vector<Vector4f>();
            const Xform3f matrix  = Xform3f::scaling(factor);
            glm::mat4 their            = scale(glm::mat4(1.0f), glm::vec3(factor.x(), factor.y(), factor.z()));
            compare_mat4(matrix, their);
        }
    }

    WHEN("you want to work with multiple Xform3s")
    {
        THEN("you can concatenate them by multiplication")
        {
            const Xform3f a    = random_matrix<Xform3f>(-10, 10);
            const Xform3f b    = random_matrix<Xform3f>(-10, 10);
            const Xform3f mine = a * b;
            glm::mat4 theirs        = to_glm_mat4(a) * to_glm_mat4(b);
            compare_mat4(mine, theirs);
        }
    }

    WHEN("you want to transform with an Xform3")
    {
        THEN("you can rotate a known vector along a know axis")
        {
            const Vector4f axis(0, 1, 0, 0);
            const Xform3f xform = Xform3f::rotation(axis, pi<float>() / 2);
            const Vector4f vector(1, 1, 0, 1);
            const Vector4f result = xform.transform(vector);
            REQUIRE(result.is_approx(Vector4f(0, 1, -1, 1)));
        }
        THEN("you can transform a random vector")
        {
            const Vector4f vec  = random_vector<Vector4f>();
            const Xform3f xform = random_matrix<Xform3f>(-10, 10);

            const Vector4f mine = xform.transform(vec);
            glm::vec4 theirs    = to_glm_mat4(xform) * glm::vec4(vec.x(), vec.y(), vec.z(), vec.w());
            compare_vec4(mine, theirs);
        }
    }
}