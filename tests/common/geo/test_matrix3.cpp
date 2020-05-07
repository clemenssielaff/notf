#include "catch.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat3x2.hpp>
#include <glm/mat4x4.hpp>

#include "utils.hpp"

#include "notf/common/geo/matrix3.hpp"

NOTF_USING_NAMESPACE;

inline void compare_mat2(const M3f& my, const glm::mat4& their)
{
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


SCENARIO("Working with 2D transformations", "[common][xform2]") {
    WHEN("you want to create a XForm2") {
        THEN("you can use the element-wise constructor") {
            const M3f matrix(1.f, 2.f, 3.f, 4.f, 5.f, 6.f);
            REQUIRE(is_approx(matrix[0][0], 1.));
            REQUIRE(is_approx(matrix[0][1], 2.));
            REQUIRE(is_approx(matrix[1][0], 3.));
            REQUIRE(is_approx(matrix[1][1], 4.));
            REQUIRE(is_approx(matrix[2][0], 5.));
            REQUIRE(is_approx(matrix[2][1], 6.));
        }
        THEN("you can use the element-wise initializer list") {
            const M3f matrix{1.f, 2.f, 3.f, 4.f, 5.f, 6.f};
            REQUIRE(is_approx(matrix[0][0], 1.));
            REQUIRE(is_approx(matrix[0][1], 2.));
            REQUIRE(is_approx(matrix[1][0], 3.));
            REQUIRE(is_approx(matrix[1][1], 4.));
            REQUIRE(is_approx(matrix[2][0], 5.));
            REQUIRE(is_approx(matrix[2][1], 6.));
        }
        THEN("you can use the element-wise constructor with mixed scalar types") {
            const M3f matrix(1.f, 2., 3, 4u, 5.l, char(6));
            REQUIRE(is_approx(matrix[0][0], 1.));
            REQUIRE(is_approx(matrix[0][1], 2.));
            REQUIRE(is_approx(matrix[1][0], 3.));
            REQUIRE(is_approx(matrix[1][1], 4.));
            REQUIRE(is_approx(matrix[2][0], 5.));
            REQUIRE(is_approx(matrix[2][1], 6.));
        }
        THEN("you can create an identity matrix") {
            const M3f matrix = M3f::identity();
            REQUIRE(is_approx(matrix[0][0], 1.));
            REQUIRE(is_approx(matrix[0][1], 0.));
            REQUIRE(is_approx(matrix[1][0], 0.));
            REQUIRE(is_approx(matrix[1][1], 1.));
            REQUIRE(is_approx(matrix[2][0], 0.));
            REQUIRE(is_approx(matrix[2][1], 0.));
        }
    }

    WHEN("you create an translation matrix") {
        THEN("it is equal to glm transformation matrix") {
            const V2f translation = notf::random<V2f>();
            const M3f matrix = M3f::translation(translation);
            glm::mat4 their = translate(glm::mat4(1.0f), glm::vec3(translation.x(), translation.y(), 0));
            compare_mat2(matrix, their);
        }
        THEN("its 'get_translation' method will return the same translation you put in") {
            const V2f translation = random<V2f>();
            const M3f matrix = M3f::translation(translation);
            REQUIRE(transform_by(V2f(0, 0), matrix).is_approx(translation));
        }
    }

    WHEN("you create a rotation matrix") {
        THEN("it is equal to glm rotation matrix") {
            const float angle = random_radian<float>();
            const M3f matrix = M3f::rotation(angle);
            glm::mat4 their = rotate(glm::mat4(1.0f), angle, glm::vec3(0, 0, 1));
            compare_mat2(matrix, their);
        }
        THEN("its 'rotation' method will return the same angle you put in") {
            const float angle = random_radian<float>();
            const M3f matrix = M3f::rotation(angle);
            REQUIRE(matrix.is_rotation());
            REQUIRE(is_approx(matrix.rotation(), angle));
        }
    }

    WHEN("you create an uniform scaling matrix") {
        THEN("it is equal to glm scaling matrix") {
            const float factor = random(0.0001f, 1000.f);
            const M3f matrix = M3f::scaling(factor);
            glm::mat4 their = scale(glm::mat4(1.0f), glm::vec3(factor, factor, 1));
            compare_mat2(matrix, their);
        }
        THEN("its 'get_scale_*' methods will return the same factor you put in") {
            const float factor = random(0.0001f, 1000.f);
            const M3f matrix = M3f::scaling(factor);
            REQUIRE(is_approx(matrix.scale_x(), factor));
            REQUIRE(is_approx(matrix.scale_y(), factor));
        }
    }

    WHEN("you create a non-uniform scaling matrix") {
        THEN("it is equal to glm scaling matrix") {
            const V2f factor = random<V2f>(0.0001f, 1000.f);
            const M3f matrix = M3f::scaling(factor);
            glm::mat4 their = scale(glm::mat4(1.0f), glm::vec3(factor.x(), factor.y(), 1));
            compare_mat2(matrix, their);
        }
        THEN("its 'get_scale_*' methods will return the same factors you put in") {
            const V2f factor = random<V2f>(0.0001f, 1000.f);
            const M3f matrix = M3f::scaling(factor);
            REQUIRE(is_approx(matrix.scale_x(), factor.x()));
            REQUIRE(is_approx(matrix.scale_y(), factor.y()));
        }
    }

    WHEN("you transform any vector with an identity transform") {
        V2f vec = random<V2f>();
        THEN("nothing happens") { REQUIRE(transform_by(vec, M3f::identity()).is_approx(vec)); }
    }

    WHEN("you stack multiple transforms") {
        const M3f trans_xform = M3f::translation(V2f(1, 0));
        const M3f rotation_xform = M3f::rotation(pi<float>() / 2);
        const M3f scale_xform = M3f::scaling(2);

        M3f total_xform = scale_xform;
        total_xform *= rotation_xform;
        total_xform *= trans_xform;

        const M3f inline_total = scale_xform * rotation_xform * trans_xform;

        const M3f premult_total = trans_xform.premult(rotation_xform).premult(scale_xform);

        THEN("they are applied from right to left") {
            V2f result = V2f::zero();
            total_xform.transform(result);
            REQUIRE(result.is_approx(V2f(0, 2)));

            V2f inline_result = V2f::zero();
            inline_total.transform(inline_result);
            REQUIRE(inline_result.is_approx(V2f(0, 2)));

            V2f premult_result = V2f::zero();
            inline_total.transform(premult_result);
            REQUIRE(premult_result.is_approx(V2f(0, 2)));
        }
    }

    WHEN("you pre-multiply a transform") {
        const M3f left_xform = random<M3f>(0, 1000, 0.01f, 2);
        const M3f right_xform = random<M3f>(0, 1000, 0.01f, 2);

        const M3f normal = right_xform * left_xform;
        const M3f premult = left_xform.premult(right_xform);

        THEN("it behaves just as if you multiplied and assigned regularly") { REQUIRE(premult.is_approx(normal)); }
    }

    WHEN("a random transform is applied to a random vector") {
        const M3f xform = random<M3f>(0, 1, 0.0001f, 2);
        const V2f vec = random<V2f>(-1, 1);

        V2f transformed_vec = vec;
        xform.transform(transformed_vec);

        THEN("the inverse of the transform will bring the vector back into its original position") {
            V2f inversed_vec = transformed_vec;
            xform.inverse().transform(inversed_vec);
            REQUIRE(vec.is_approx(inversed_vec));
        }
    }
}
