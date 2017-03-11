#include "test/catch.hpp"

#include "test/test_utils.hpp"
using namespace notf;

SCENARIO("Vector2s can be constructed", "[common][vector2]")
{

    WHEN("from two values")
    {
        const float fa = random_number<float>();
        const float fb = random_number<float>();
        const int ia   = random_number<int>(-10, 10);
        const int ib   = random_number<int>(-10, 10);

        const Vector2f vecf(fa, fb);
        const Vector2i veci(ia, ib);

        THEN("correctly")
        {
            REQUIRE(vecf.x == approx(fa));
            REQUIRE(vecf.y == approx(fb));
            REQUIRE(veci.x == ia);
            REQUIRE(veci.y == ib);
        }
    }

    WHEN("to be zero")
    {
        const Vector2f vecf = Vector2f::zero();
        const Vector2i veci = Vector2i::zero();

        THEN("correctly")
        {
            REQUIRE(vecf.x == approx(0));
            REQUIRE(vecf.y == approx(0));
            REQUIRE(veci.x == 0);
            REQUIRE(veci.y == 0);
        }
    }

    WHEN("with a fill value")
    {
        const float f = random_number<float>();
        const int i   = random_number<int>(-10, 10);

        const Vector2f vecf = Vector2f::fill(f);
        const Vector2i veci = Vector2i::fill(i);

        THEN("correctly")
        {
            REQUIRE(vecf.x == approx(f));
            REQUIRE(vecf.y == approx(f));
            REQUIRE(veci.x == i);
            REQUIRE(veci.y == i);
        }
    }

    WHEN("as an axis")
    {
        const Vector2f x_axis_f = Vector2f::x_axis();
        const Vector2i x_axis_i = Vector2i::x_axis();

        const Vector2f y_axis_f = Vector2f::y_axis();
        const Vector2i y_axis_i = Vector2i::y_axis();

        THEN("correctly")
        {
            REQUIRE(x_axis_f.x == approx(1));
            REQUIRE(x_axis_f.y == approx(0));
            REQUIRE(x_axis_i.x == 1);
            REQUIRE(x_axis_i.y == 0);

            REQUIRE(y_axis_f.x == approx(0));
            REQUIRE(y_axis_f.y == approx(1));
            REQUIRE(y_axis_i.x == 0);
            REQUIRE(y_axis_i.y == 1);
        }
    }
}

SCENARIO("Vector2s can be inspected", "[common][vector2]")
{
    WHEN("you need to check if a vector2 is zero")
    {
        const Vector2f zero_f = Vector2f::zero();
        const Vector2i zero_i = Vector2i::zero();

        const float x_f = random_number<float>();
        const float y_f = random_number<float>();
        const int x_i   = random_number<int>();
        const int y_i   = random_number<int>();

        const Vector2f random_f = Vector2f(x_f, y_f);
        const Vector2i random_i = Vector2i(x_i, y_i);

        THEN("is_zero will work")
        {
            REQUIRE(zero_f.is_zero());
            REQUIRE(!random_f.is_zero()); // may fail but not very likely

            REQUIRE(zero_i.is_zero());
            REQUIRE(!random_i.is_zero()); // may fail but not very likely
        }
    }

    WHEN("you need to check if a vector2 is zero")
    {
        const float x_f = random_number<float>();
        const float y_f = random_number<float>();
        const int x_i   = random_number<int>();
        const int y_i   = random_number<int>();

        const Vector2f zero_x_f = Vector2f(0, x_f);
        const Vector2f zero_y_f = Vector2f(x_f, 0);
        const Vector2f random_f = Vector2f(x_f, y_f);

        const Vector2i zero_x_i = Vector2i(0, x_i);
        const Vector2i zero_y_i = Vector2i(x_i, 0);
        const Vector2i random_i = Vector2i(x_i, y_i);

        THEN("is_zero will work")
        {
            REQUIRE(zero_x_f.contains_zero());
            REQUIRE(zero_y_f.contains_zero());
            REQUIRE(!random_f.contains_zero()); // may fail but not very likely

            REQUIRE(zero_x_i.contains_zero());
            REQUIRE(zero_y_i.contains_zero());
            REQUIRE(!random_i.contains_zero()); // may fail but not very likely
        }
    }

    WHEN("you need to check if a vector2 is horizontal or vertical")
    {
        const Vector2f zero_f = Vector2f::zero();
        const Vector2i zero_i = Vector2i::zero();

        const Vector2f x_axis_f = Vector2f::x_axis();
        const Vector2f y_axis_f = Vector2f::y_axis();
        const Vector2i x_axis_i = Vector2i::x_axis();
        const Vector2i y_axis_i = Vector2i::y_axis();

        const Vector2f scaled_x_axis_f = Vector2f::x_axis() * random_number<float>(1, 100);
        const Vector2f scaled_y_axis_f = Vector2f::y_axis() * random_number<float>(1, 100);
        const Vector2i scaled_x_axis_i = Vector2i::x_axis() * random_number<int>(1, 100);
        const Vector2i scaled_y_axis_i = Vector2i::y_axis() * random_number<int>(1, 100);

        const Vector2f random_f = Vector2f(random_number<float>(), random_number<float>());
        const Vector2i random_i = Vector2i(random_number<int>(), random_number<int>());

        THEN("is_horizontal and is_vertical will work")
        {
            REQUIRE(zero_f.is_horizontal());
            REQUIRE(zero_f.is_vertical());
            REQUIRE(x_axis_f.is_horizontal());
            REQUIRE(!x_axis_f.is_vertical());
            REQUIRE(!y_axis_f.is_horizontal());
            REQUIRE(y_axis_f.is_vertical());
            REQUIRE(scaled_x_axis_f.is_horizontal());
            REQUIRE(!scaled_x_axis_f.is_vertical());
            REQUIRE(!scaled_y_axis_f.is_horizontal());
            REQUIRE(scaled_y_axis_f.is_vertical());
            REQUIRE(!random_f.is_horizontal()); // may fail but not very likely
            REQUIRE(!random_f.is_vertical());   //

            REQUIRE(zero_i.is_horizontal());
            REQUIRE(zero_i.is_vertical());
            REQUIRE(x_axis_i.is_horizontal());
            REQUIRE(!x_axis_i.is_vertical());
            REQUIRE(!y_axis_i.is_horizontal());
            REQUIRE(y_axis_i.is_vertical());
            REQUIRE(scaled_x_axis_i.is_horizontal());
            REQUIRE(!scaled_x_axis_i.is_vertical());
            REQUIRE(!scaled_y_axis_i.is_horizontal());
            REQUIRE(scaled_y_axis_i.is_vertical());
            REQUIRE(!random_i.is_horizontal()); // may fail but not very likely
            REQUIRE(!random_i.is_vertical());   //
        }
    }

    WHEN("you need use the []-operator on a vector2")
    {
        const float x_f = random_number<float>();
        const float y_f = random_number<float>();
        const int x_i   = random_number<int>();
        const int y_i   = random_number<int>();

        const Vector2f const_random_f = Vector2f(x_f, y_f);
        const Vector2i const_random_i = Vector2i(x_i, y_i);

        Vector2f mutable_random_f = Vector2f(x_f, y_f);
        Vector2i mutable_random_i = Vector2i(x_i, y_i);

        THEN("it will work correctly")
        {
            REQUIRE(const_random_f[0] == approx(x_f));
            REQUIRE(const_random_f[1] == approx(y_f));
            REQUIRE(const_random_i[0] == x_i);
            REQUIRE(const_random_i[1] == y_i);

            mutable_random_f[0] += 1;
            mutable_random_f[1] += 2;
            REQUIRE(mutable_random_f[0] == approx(x_f + 1));
            REQUIRE(mutable_random_f[1] == approx(y_f + 2));

            mutable_random_i[0] += 1;
            mutable_random_i[1] += 2;
            REQUIRE(mutable_random_i[0] == x_i + 1);
            REQUIRE(mutable_random_i[1] == y_i + 2);
        }
    }

    WHEN("you need check if two vector2s are parallel")
    {
        const Vector2f vecf = Vector2f(random_number<float>(), random_number<float>());
        const Vector2d vecd = Vector2d(random_number<double>(), random_number<double>());

        THEN("it will work correctly")
        {
            REQUIRE(vecf.is_parallel_to(vecf));
            REQUIRE(vecf.is_parallel_to(vecf * random_number<float>(0.1f, 2)));
            REQUIRE(vecf.is_parallel_to(vecf * random_number<float>(-0.1f, -2)));
            REQUIRE(vecf.is_parallel_to(vecf.inverse()));
            REQUIRE(vecf.is_parallel_to(Vector2f::zero()));

            REQUIRE(!vecf.is_parallel_to(vecf.orthogonal()));
            REQUIRE(!vecf.is_parallel_to(Vector2f(random_number<float>(), random_number<float>())));

            REQUIRE(vecd.is_parallel_to(vecd));
            REQUIRE(vecd.is_parallel_to(vecd * random_number<double>(0.1, 2)));
            REQUIRE(vecd.is_parallel_to(vecd * random_number<double>(-0.1, -2)));
            REQUIRE(vecd.is_parallel_to(vecd.inverse()));
            REQUIRE(vecd.is_parallel_to(Vector2d::zero()));

            REQUIRE(!vecd.is_parallel_to(Vector2d(random_number<double>(), random_number<double>())));
            REQUIRE(!vecd.is_parallel_to(vecd.orthogonal()));

            REQUIRE(Vector2f::x_axis().is_parallel_to(Vector2f::x_axis() * random_number<float>()));
            REQUIRE(!Vector2f::x_axis().is_parallel_to(random_vector<float>()));
            REQUIRE(Vector2f::y_axis().is_parallel_to(Vector2f::y_axis() * random_number<float>()));
            REQUIRE(!Vector2f::y_axis().is_parallel_to(random_vector<float>()));

            REQUIRE(Vector2d::x_axis().is_parallel_to(Vector2d::x_axis() * random_number<double>()));
            REQUIRE(!Vector2d::x_axis().is_parallel_to(random_vector<double>()));
            REQUIRE(Vector2d::y_axis().is_parallel_to(Vector2d::y_axis() * random_number<double>()));
            REQUIRE(!Vector2d::y_axis().is_parallel_to(random_vector<double>()));

            REQUIRE(lowest_vector<float>().is_parallel_to(lowest_vector<float>()));
            REQUIRE(lowest_vector<float>().is_parallel_to(highest_vector<float>()));
            REQUIRE(lowest_vector<float>().is_parallel_to(Vector2f(1, 1)));
            REQUIRE(lowest_vector<float>().is_parallel_to(Vector2f(-0.000001f, -0.000001f)));
            REQUIRE(lowest_vector<float>().is_parallel_to(Vector2f::zero()));
            REQUIRE(highest_vector<float>().is_parallel_to(highest_vector<float>()));
            REQUIRE(highest_vector<float>().is_parallel_to(lowest_vector<float>()));
            REQUIRE(highest_vector<float>().is_parallel_to(Vector2f::zero()));

            REQUIRE(!lowest_vector<float>().is_parallel_to(random_vector<float>()));
            REQUIRE(!lowest_vector<float>().is_parallel_to(lowest_vector<float>().orthogonal()));
            REQUIRE(!highest_vector<float>().is_parallel_to(random_vector<float>()));
            REQUIRE(!highest_vector<float>().is_parallel_to(highest_vector<float>().orthogonal()));

            REQUIRE(lowest_vector<double>().is_parallel_to(lowest_vector<double>()));
            REQUIRE(lowest_vector<double>().is_parallel_to(highest_vector<double>()));
            REQUIRE(lowest_vector<double>().is_parallel_to(Vector2d(1, 1)));
            REQUIRE(lowest_vector<double>().is_parallel_to(Vector2d(-0.000001, -0.000001)));
            REQUIRE(lowest_vector<double>().is_parallel_to(Vector2d::zero()));
            REQUIRE(highest_vector<double>().is_parallel_to(highest_vector<double>()));
            REQUIRE(highest_vector<double>().is_parallel_to(lowest_vector<double>()));
            REQUIRE(highest_vector<double>().is_parallel_to(Vector2d::zero()));

            REQUIRE(!lowest_vector<double>().is_parallel_to(random_vector<double>()));
            REQUIRE(!lowest_vector<double>().is_parallel_to(lowest_vector<double>().orthogonal()));
            REQUIRE(!highest_vector<double>().is_parallel_to(random_vector<double>()));
            REQUIRE(!highest_vector<double>().is_parallel_to(highest_vector<double>().orthogonal()));
        }
    }

    WHEN("you need check if two vector2s are orthogonal")
    {
        const Vector2f vecf = random_vector<float>();
        const Vector2d vecd = random_vector<double>();

        THEN("it will work correctly")
        {
            REQUIRE(vecf.is_orthogonal_to(Vector2f::zero()));
            REQUIRE(vecf.is_orthogonal_to(vecf.orthogonal()));
            REQUIRE(vecf.is_orthogonal_to(vecf.orthogonal().invert()));
            REQUIRE(vecf.is_orthogonal_to(vecf.orthogonal() * random_number<float>(0.1f, 2)));

            REQUIRE(!vecf.is_orthogonal_to(vecf));
            REQUIRE(!vecf.is_orthogonal_to(Vector2f(random_number<float>(), random_number<float>())));
            REQUIRE(!vecf.is_orthogonal_to(vecf * random_number<float>(0.1f, 2)));
            REQUIRE(!vecf.is_orthogonal_to(vecf * random_number<float>(-0.1f, -2)));
            REQUIRE(!vecf.is_orthogonal_to(vecf.inverse()));

            REQUIRE(vecd.is_orthogonal_to(Vector2d::zero()));
            REQUIRE(vecd.is_orthogonal_to(vecd.orthogonal()));
            REQUIRE(vecd.is_orthogonal_to(vecd.orthogonal().invert()));
            REQUIRE(vecd.is_orthogonal_to(vecd.orthogonal() * random_number<double>(0.1, 2)));

            REQUIRE(!vecd.is_orthogonal_to(vecd));
            REQUIRE(!vecd.is_orthogonal_to(Vector2d(random_number<double>(), random_number<double>())));
            REQUIRE(!vecd.is_orthogonal_to(vecd * random_number<double>(0.1, 2)));
            REQUIRE(!vecd.is_orthogonal_to(vecd * random_number<double>(-0.1, -2)));
            REQUIRE(!vecd.is_orthogonal_to(vecd.inverse()));
        }
    }

    WHEN("you need check if a vector2 is unit")
    {
        THEN("it will work correctly")
        {
            for (auto i = 0; i < 10000; ++i) {
                REQUIRE(Vector2f::x_axis().is_unit());
                REQUIRE(Vector2f::y_axis().is_unit());
                REQUIRE(Vector2d::x_axis().is_unit());
                REQUIRE(Vector2d::y_axis().is_unit());

                REQUIRE(!Vector2f::zero().is_unit());
                REQUIRE(!Vector2d::zero().is_unit());

                REQUIRE(!random_vector<float>().is_unit());
                REQUIRE(!random_vector<double>().is_unit());

                REQUIRE(random_vector<float>().normalized().is_unit());
                REQUIRE(random_vector<double>().normalized().is_unit());
                REQUIRE(random_vector<float>().normalize().is_unit());
                REQUIRE(random_vector<double>().normalize().is_unit());

                REQUIRE(!lowest_vector<float>().is_unit());
                REQUIRE(lowest_vector<float>().normalize().is_unit());
                REQUIRE(lowest_vector<float>().normalized().is_unit());
                REQUIRE(!lowest_vector<double>().is_unit());
                REQUIRE(lowest_vector<double>().normalize().is_unit());
                REQUIRE(lowest_vector<double>().normalized().is_unit());

                REQUIRE(!highest_vector<float>().is_unit());
                REQUIRE(highest_vector<float>().normalize().is_unit());
                REQUIRE(highest_vector<float>().normalized().is_unit());
                REQUIRE(!highest_vector<double>().is_unit());
                REQUIRE(highest_vector<double>().normalize().is_unit());
                REQUIRE(highest_vector<double>().normalized().is_unit());
            }
        }
    }

    WHEN("you need check if two vector2s are approximately the same")
    {
        const Vector2f vecf = random_vector<float>();
        const Vector2d vecd = random_vector<double>();

        THEN("it will work correctly")
        {
            REQUIRE(vecf.is_approx(vecf));
            REQUIRE(vecd.is_approx(vecd));

            REQUIRE(vecf.is_approx(Vector2f(vecf.x, vecf.y + precision_high<float>())));
            REQUIRE(vecd.is_approx(Vector2d(vecd.x, vecd.y + precision_high<double>())));

            REQUIRE(!random_vector<float>().is_approx(random_vector<float>()));
            REQUIRE(!random_vector<double>().is_approx(random_vector<double>()));
        }
    }

    WHEN("you need the magnitude of a vector2")
    {
        const float factor_f  = random_number<float>(-1, 1);
        const double factor_d = random_number<double>(-1, 1);

        THEN("it will work correctly")
        {
            REQUIRE(Vector2f::x_axis().magnitude() == approx(1));
            REQUIRE(Vector2f::y_axis().magnitude() == approx(1));
            REQUIRE(Vector2d::x_axis().magnitude() == approx(1));
            REQUIRE(Vector2d::y_axis().magnitude() == approx(1));

            REQUIRE(Vector2f::zero().magnitude() == approx(0));
            REQUIRE(Vector2d::zero().magnitude() == approx(0));

            REQUIRE(random_vector<float>().magnitude() != approx(1));
            REQUIRE(random_vector<double>().magnitude() != approx(1));

            REQUIRE((random_vector<float>().normalized() * factor_f).magnitude() == approx(abs(factor_f)));
            REQUIRE((random_vector<double>().normalized() * factor_d).magnitude() == approx(abs(factor_d)));

            REQUIRE(random_vector<float>().normalized().magnitude() == approx(1));
            REQUIRE(random_vector<double>().normalized().magnitude() == approx(1));
            REQUIRE(random_vector<float>().normalize().magnitude() == approx(1));
            REQUIRE(random_vector<double>().normalize().magnitude() == approx(1));

            REQUIRE(lowest_vector<float>().magnitude() != approx(1));
            REQUIRE(lowest_vector<float>().normalize().magnitude() == approx(1));
            REQUIRE(lowest_vector<float>().normalized().magnitude() == approx(1));
            REQUIRE(lowest_vector<double>().magnitude() != approx(1));
            REQUIRE(lowest_vector<double>().normalize().magnitude() == approx(1));
            REQUIRE(lowest_vector<double>().normalized().magnitude() == approx(1));

            REQUIRE(highest_vector<float>().magnitude() != approx(1));
            REQUIRE(highest_vector<float>().normalize().magnitude() == approx(1));
            REQUIRE(highest_vector<float>().normalized().magnitude() == approx(1));
            REQUIRE(highest_vector<double>().magnitude() != approx(1));
            REQUIRE(highest_vector<double>().normalize().magnitude() == approx(1));
            REQUIRE(highest_vector<double>().normalized().magnitude() == approx(1));
        }
    }
}

SCENARIO("Vector2s can be modified", "[common][vector2]")
{
    WHEN("you need to set a vector2 to zero")
    {
        const float x_f = random_number<float>();
        const float y_f = random_number<float>();
        const int x_i   = random_number<int>();
        const int y_i   = random_number<int>();

        Vector2f random_f = Vector2f(x_f, y_f);
        Vector2i random_i = Vector2i(x_i, y_i);

        THEN("set_zero will work")
        {
            REQUIRE(!random_f.is_zero()); // may fail but not very likely
            REQUIRE(!random_i.is_zero()); // may fail but not very likely

            random_f.set_zero();
            random_i.set_zero();

            REQUIRE(random_f.is_zero());
            REQUIRE(random_i.is_zero());
        }
    }

    WHEN("you need to invert a vector")
    {
        const float x_f = random_number<float>();
        const float y_f = random_number<float>();
        const int x_i   = random_number<int>();
        const int y_i   = random_number<int>();

        Vector2f random_f = Vector2f(x_f, y_f);
        Vector2i random_i = Vector2i(x_i, y_i);

        Vector2f inv_random_f = random_f.inverse();
        Vector2i inv_random_i = random_i.inverse();

        THEN("invert and inverse will work")
        {
            REQUIRE(inv_random_f.x == approx(-random_f.x));
            REQUIRE(inv_random_f.y == approx(-random_f.y));

            REQUIRE(inv_random_i.x == -random_i.x);
            REQUIRE(inv_random_i.y == -random_i.y);

            random_f.invert();
            random_i.invert();

            REQUIRE(inv_random_f == random_f);
            REQUIRE(inv_random_i == random_i);
        }
    }

    WHEN("you need to get an orthogonal vector2")
    {
        const float x_f = random_number<float>();
        const float y_f = random_number<float>();
        const int x_i   = random_number<int>();
        const int y_i   = random_number<int>();

        const Vector2f random_f = Vector2f(x_f, y_f);
        const Vector2i random_i = Vector2i(x_i, y_i);

        const Vector2f ortho_random_f = random_f.orthogonal();
        const Vector2i ortho_random_i = random_i.orthogonal();

        THEN("vector2s will be rotated 90degrees counter-clockwise")
        {
            REQUIRE(norm_angle(random_f.angle() + static_cast<float>(HALF_PI)) == approx(ortho_random_f.angle(), precision_low<float>()));

            REQUIRE(ortho_random_i.x == -random_i.y);
            REQUIRE(ortho_random_i.y == random_i.x);
        }
    }
}