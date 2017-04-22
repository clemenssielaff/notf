#include "test/catch.hpp"

#include "common/xform2.hpp"
#include "test/test_utils.hpp"
using namespace notf;

SCENARIO("Working with 2D transformations", "[common][xform2]")
{
    WHEN("you transform any vector with an identity transform")
    {
        const Vector2f vec             = random_vector<float>();
        const Vector2f transformed_vec = Xform2f::identity().transform(vec);
        THEN("nothing happens")
        {
            REQUIRE(vec.is_approx(transformed_vec));
        }
    }

    WHEN("you stack multiple transforms")
    {
        const Xform2f trans_xform    = Xform2f::translation(Vector2f(100, 0));
        const Xform2f rotation_xform = Xform2f::rotation(static_cast<float>(PI / 2));
        const Xform2f scale_xform    = Xform2f::scale(2);

        Xform2f total_xform = trans_xform;
        total_xform *= rotation_xform;
        total_xform *= scale_xform;

        Xform2f inline_total = trans_xform * rotation_xform * scale_xform;

        THEN("they are applied from left to right")
        {
            const Vector2f result = total_xform.transform(Vector2f::zero());
            REQUIRE(result.is_approx(Vector2f(0, 200), precision_low<float>()));

            const Vector2f inline_result = inline_total.transform(Vector2f::zero());
            REQUIRE(inline_result.is_approx(Vector2f(0, 200), precision_low<float>()));
        }
    }

    WHEN("you pre-multiply a transform")
    {
        const Xform2f left_xform  = random_xform2<float>(0, 1000, 0.01f, 2);
        const Xform2f right_xform = random_xform2<float>(0, 1000, 0.01f, 2);

        const Xform2f normal  = right_xform * left_xform;
        const Xform2f premult = left_xform.get_premult(right_xform);

        THEN("it behaves just as if you multiplied and assigned regularly")
        {
            REQUIRE(premult.is_approx(normal));
        }
    }

    WHEN("a random transform is applied to a random vector")
    {
        const Xform2f xform = random_xform2<float>(0, 1000, 0.01f, 2);
        const Vector2f vec  = random_vector<float>(-1000, 1000);

        const Vector2f transformed_vec = xform.transform(vec);

        THEN("the inverse of the transform will bring the vector back into its original position")
        {
            const Vector2f inversed_vec = xform.get_inverse().transform(transformed_vec);

            REQUIRE(vec.is_approx(inversed_vec, 0.0002f)); // very imprecise
        }
    }
}
