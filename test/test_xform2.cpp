#include "catch.hpp"
#include "glm_utils.hpp"
#include "test_utils.hpp"

using namespace notf;

SCENARIO("Working with 2D transformations", "[common][xform2]")
{
    WHEN("you want to create a XForm2")
    {
        THEN("you can use the element-wise constructor")
        {
            const Xform2f matrix(1.f, 2.f,
                                 3.f, 4.f,
                                 5.f, 6.f);
            REQUIRE(matrix[0][0] == approx(1.));
            REQUIRE(matrix[0][1] == approx(2.));
            REQUIRE(matrix[1][0] == approx(3.));
            REQUIRE(matrix[1][1] == approx(4.));
            REQUIRE(matrix[2][0] == approx(5.));
            REQUIRE(matrix[2][1] == approx(6.));
        }
        THEN("you can use the element-wise initializer list")
        {
            const Xform2f matrix{1.f, 2.f,
                                 3.f, 4.f,
                                 5.f, 6.f};
            REQUIRE(matrix[0][0] == approx(1.));
            REQUIRE(matrix[0][1] == approx(2.));
            REQUIRE(matrix[1][0] == approx(3.));
            REQUIRE(matrix[1][1] == approx(4.));
            REQUIRE(matrix[2][0] == approx(5.));
            REQUIRE(matrix[2][1] == approx(6.));
        }
        THEN("you can use the element-wise constructor with mixed scalar types")
        {
            const Xform2f matrix(1.f, 2.,
                                 3, 4u,
                                 5.l, char(6));
            REQUIRE(matrix[0][0] == approx(1.));
            REQUIRE(matrix[0][1] == approx(2.));
            REQUIRE(matrix[1][0] == approx(3.));
            REQUIRE(matrix[1][1] == approx(4.));
            REQUIRE(matrix[2][0] == approx(5.));
            REQUIRE(matrix[2][1] == approx(6.));
        }
        THEN("you can create an identity matrix")
        {
            const Xform2f matrix = Xform2f::identity();
            REQUIRE(matrix[0][0] == approx(1.));
            REQUIRE(matrix[0][1] == approx(0.));
            REQUIRE(matrix[1][0] == approx(0.));
            REQUIRE(matrix[1][1] == approx(1.));
            REQUIRE(matrix[2][0] == approx(0.));
            REQUIRE(matrix[2][1] == approx(0.));
        }
    }

    WHEN("you create an translation matrix")
    {
        THEN("it is equal to glm transformation matrix")
        {
            const Vector2f translation = random_vector<Vector2f>();
            const Xform2f matrix       = Xform2f::translation(translation);
            glm::mat4 their            = translate(glm::mat4(1.0f), glm::vec3(translation.x(), translation.y(), 0));
            compare_mat2(matrix, their);
        }
        THEN("its 'get_translation' method will return the same translation you put in")
        {
            const Vector2f translation = random_vector<Vector2f>();
            const Xform2f matrix       = Xform2f::translation(translation);
            REQUIRE(matrix.get_translation().is_approx(translation));
        }
    }

    WHEN("you create a rotation matrix")
    {
        THEN("it is equal to glm rotation matrix")
        {
            const float angle    = random_radian<float>();
            const Xform2f matrix = Xform2f::rotation(angle);
            glm::mat4 their      = rotate(glm::mat4(1.0f), angle, glm::vec3(0, 0, 1));
            compare_mat2(matrix, their);
        }
        THEN("its 'get_rotation' method will return the same angle you put in")
        {
            const float angle    = random_radian<float>();
            const Xform2f matrix = Xform2f::rotation(angle);
            REQUIRE(matrix.is_rotation());
            REQUIRE(matrix.get_rotation() == approx(angle));
        }
    }

    WHEN("you create an uniform scaling matrix")
    {
        THEN("it is equal to glm scaling matrix")
        {
            const float factor   = random_number(0.0001f, 1000.f);
            const Xform2f matrix = Xform2f::scaling(factor);
            glm::mat4 their      = scale(glm::mat4(1.0f), glm::vec3(factor, factor, 1));
            compare_mat2(matrix, their);
        }
        THEN("its 'get_scale_*' methods will return the same factor you put in")
        {
            const float factor   = random_number(0.0001f, 1000.f);
            const Xform2f matrix = Xform2f::scaling(factor);
            REQUIRE(matrix.get_scale_x() == approx(factor));
            REQUIRE(matrix.get_scale_y() == approx(factor));
        }
    }

    WHEN("you create a non-uniform scaling matrix")
    {
        THEN("it is equal to glm scaling matrix")
        {
            const Vector2f factor = random_vector<Vector2f>(0.0001f, 1000.f);
            const Xform2f matrix  = Xform2f::scaling(factor);
            glm::mat4 their       = scale(glm::mat4(1.0f), glm::vec3(factor.x(), factor.y(), 1));
            compare_mat2(matrix, their);
        }
        THEN("its 'get_scale_*' methods will return the same factors you put in")
        {
            const Vector2f factor = random_vector<Vector2f>(0.0001f, 1000.f);
            const Xform2f matrix  = Xform2f::scaling(factor);
            REQUIRE(matrix.get_scale_x() == approx(factor.x()));
            REQUIRE(matrix.get_scale_y() == approx(factor.y()));
        }
    }

    WHEN("you transform any vector with an identity transform")
    {
        Vector2f vec            = random_vector<Vector2f>();
        const Vector2f vec_copy = vec;
        Xform2f::identity().transform(vec);
        THEN("nothing happens")
        {
            REQUIRE(vec.is_approx(vec_copy));
        }
    }

    WHEN("you stack multiple transforms")
    {
        const Xform2f trans_xform    = Xform2f::translation(Vector2f(1, 0));
        const Xform2f rotation_xform = Xform2f::rotation(pi<float>() / 2);
        const Xform2f scale_xform    = Xform2f::scaling(2);

        Xform2f total_xform = scale_xform;
        total_xform *= rotation_xform;
        total_xform *= trans_xform;

        const Xform2f inline_total = scale_xform * rotation_xform * trans_xform;

        const Xform2f premult_total = trans_xform.premult(rotation_xform).premult(scale_xform);

        THEN("they are applied from right to left")
        {
            Vector2f result = Vector2f::zero();
            total_xform.transform(result);
            REQUIRE(result.is_approx(Vector2f(0, 2)));

            Vector2f inline_result = Vector2f::zero();
            inline_total.transform(inline_result);
            REQUIRE(inline_result.is_approx(Vector2f(0, 2)));

            Vector2f premult_result = Vector2f::zero();
            inline_total.transform(premult_result);
            REQUIRE(premult_result.is_approx(Vector2f(0, 2)));
        }
    }

    WHEN("you pre-multiply a transform")
    {
        const Xform2f left_xform  = random_xform2<float>(0, 1000, 0.01f, 2);
        const Xform2f right_xform = random_xform2<float>(0, 1000, 0.01f, 2);

        const Xform2f normal  = right_xform * left_xform;
        const Xform2f premult = left_xform.premult(right_xform);

        THEN("it behaves just as if you multiplied and assigned regularly")
        {
            REQUIRE(premult.is_approx(normal));
        }
    }

    WHEN("a random transform is applied to a random vector")
    {
        const Xform2f xform = random_xform2<float>(0, 1, 0.0001f, 2);
        const Vector2f vec  = random_vector<Vector2f>(-1, 1);

        Vector2f transformed_vec = vec;
        xform.transform(transformed_vec);

        THEN("the inverse of the transform will bring the vector back into its original position")
        {
            Vector2f inversed_vec = transformed_vec;
            xform.invert().transform(inversed_vec);
            REQUIRE(vec.is_approx(inversed_vec));
        }
    }
}
