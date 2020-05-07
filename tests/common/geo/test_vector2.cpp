#include "catch.hpp"
#include "utils.hpp"

#include "notf/common/geo/vector2.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("Vector2s can be constructed", "[common][vector2]") {
    WHEN("from two values") {
        const float fa = random<float>();
        const float fb = random<float>();
        const int ia = random<int>(-10, 10);
        const int ib = random<int>(-10, 10);

        const V2f vecf(fa, fb);
        const V2i veci(ia, ib);

        THEN("correctly") {
            REQUIRE(is_approx(vecf.x(), fa));
            REQUIRE(is_approx(vecf.y(), fb));
            REQUIRE(veci.x() == ia);
            REQUIRE(veci.y() == ib);
        }
    }

    WHEN("to be zero") {
        const V2f vecf = V2f::zero();
        const V2i veci = V2i::zero();

        THEN("correctly") {
            REQUIRE(is_approx(vecf.x(), 0));
            REQUIRE(is_approx(vecf.y(), 0));
            REQUIRE(veci.x() == 0);
            REQUIRE(veci.y() == 0);
        }
    }

    WHEN("with a fill value") {
        const float f = random<float>();
        const int i = random<int>(-10, 10);

        const V2f vecf = V2f::all(f);
        const V2i veci = V2i::all(i);

        THEN("correctly") {
            REQUIRE(is_approx(vecf.x(), f));
            REQUIRE(is_approx(vecf.y(), f));
            REQUIRE(veci.x() == i);
            REQUIRE(veci.y() == i);
        }
    }

    WHEN("as an axis") {
        const V2f x_axis_f = V2f::x_axis();
        const V2i x_axis_i = V2i::x_axis();

        const V2f y_axis_f = V2f::y_axis();
        const V2i y_axis_i = V2i::y_axis();

        THEN("correctly") {
            REQUIRE(is_approx(x_axis_f.x(), 1));
            REQUIRE(is_approx(x_axis_f.y(), 0));
            REQUIRE(x_axis_i.x() == 1);
            REQUIRE(x_axis_i.y() == 0);

            REQUIRE(is_approx(y_axis_f.x(), 0));
            REQUIRE(is_approx(y_axis_f.y(), 1));
            REQUIRE(y_axis_i.x() == 0);
            REQUIRE(y_axis_i.y() == 1);
        }
    }
}

SCENARIO("Vector2s can be inspected", "[common][vector2]") {
    WHEN("you need to check if a vector2 is zero") {
        const V2f zero_f = V2f::zero();
        const V2i zero_i = V2i::zero();

        const float x_f = random<float>();
        const float y_f = random<float>();
        const int x_i = random<int>();
        const int y_i = random<int>();

        const V2f random_f = V2f(x_f, y_f);
        const V2i random_i = V2i(x_i, y_i);

        THEN("is_zero will work") {
            REQUIRE(zero_f.is_zero());
            REQUIRE(!random_f.is_zero()); // may fail but not very likely

            REQUIRE(zero_i.is_zero());
            REQUIRE(!random_i.is_zero()); // may fail but not very likely
        }
    }

    WHEN("you need to check if a vector2 is zero") {
        const float x_f = random<float>();
        const float y_f = random<float>();
        const int x_i = random<int>();
        const int y_i = random<int>();

        const V2f zero_x_f = V2f(0, x_f);
        const V2f zero_y_f = V2f(x_f, 0);
        const V2f random_f = V2f(x_f, y_f);

        const V2i zero_x_i = V2i(0, x_i);
        const V2i zero_y_i = V2i(x_i, 0);
        const V2i random_i = V2i(x_i, y_i);

        THEN("is_zero will work") {
            REQUIRE(zero_x_f.contains_zero());
            REQUIRE(zero_y_f.contains_zero());
            REQUIRE(!random_f.contains_zero()); // may fail but not very likely

            REQUIRE(zero_x_i.contains_zero());
            REQUIRE(zero_y_i.contains_zero());
            REQUIRE(!random_i.contains_zero()); // may fail but not very likely
        }
    }

    WHEN("you need to check if a vector2 is horizontal or vertical") {
        const V2f zero_f = V2f::zero();
        const V2i zero_i = V2i::zero();

        const V2f x_axis_f = V2f::x_axis();
        const V2f y_axis_f = V2f::y_axis();
        const V2i x_axis_i = V2i::x_axis();
        const V2i y_axis_i = V2i::y_axis();

        const V2f scaled_x_axis_f = V2f::x_axis() * random<float>(1, 100);
        const V2f scaled_y_axis_f = V2f::y_axis() * random<float>(1, 100);
        const V2i scaled_x_axis_i = V2i::x_axis() * random<int>(1, 100);
        const V2i scaled_y_axis_i = V2i::y_axis() * random<int>(1, 100);

        const V2f random_f = V2f(random<float>(), random<float>());
        const V2i random_i = V2i(random<int>(), random<int>());

        THEN("is_horizontal and is_vertical will work") {
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

    WHEN("you need use the []-operator on a vector2") {
        const float x_f = random<float>();
        const float y_f = random<float>();
        const int x_i = random<int>();
        const int y_i = random<int>();

        const V2f const_random_f = V2f(x_f, y_f);
        const V2i const_random_i = V2i(x_i, y_i);

        V2f mutable_random_f = V2f(x_f, y_f);
        V2i mutable_random_i = V2i(x_i, y_i);

        THEN("it will work correctly") {
            REQUIRE(is_approx(const_random_f[0], x_f));
            REQUIRE(is_approx(const_random_f[1], y_f));
            REQUIRE(const_random_i[0] == x_i);
            REQUIRE(const_random_i[1] == y_i);

            mutable_random_f[0] += 1;
            mutable_random_f[1] += 2;
            REQUIRE(is_approx(mutable_random_f[0], x_f + 1));
            REQUIRE(is_approx(mutable_random_f[1], y_f + 2));

            mutable_random_i[0] += 1;
            mutable_random_i[1] += 2;
            REQUIRE(mutable_random_i[0] == x_i + 1);
            REQUIRE(mutable_random_i[1] == y_i + 2);
        }
    }

    WHEN("you need check if two vector2s are parallel") {
        const V2f vecf = V2f(random<float>(), random<float>());
        const V2d vecd = V2d(random<double>(), random<double>());

        THEN("it will work correctly") {
            REQUIRE(vecf.is_parallel_to(vecf));
            REQUIRE(vecf.is_parallel_to(vecf * random<float>(0.1f, 2)));
            REQUIRE(vecf.is_parallel_to(vecf * random<float>(-0.1f, -2)));
            REQUIRE(vecf.is_parallel_to(-vecf));
            REQUIRE(vecf.is_parallel_to(V2f::zero()));

            REQUIRE(!vecf.is_parallel_to(vecf.get_orthogonal()));
            REQUIRE(!vecf.is_parallel_to(V2f(random<float>(), random<float>())));

            REQUIRE(vecd.is_parallel_to(vecd));
            REQUIRE(vecd.is_parallel_to(vecd * random<double>(0.1, 2)));
            REQUIRE(vecd.is_parallel_to(vecd * random<double>(-0.1, -2)));
            REQUIRE(vecd.is_parallel_to(-vecd));
            REQUIRE(vecd.is_parallel_to(V2d::zero()));

            REQUIRE(!vecd.is_parallel_to(V2d(random<double>(), random<double>())));
            REQUIRE(!vecd.is_parallel_to(vecd.get_orthogonal()));

            REQUIRE(V2f::x_axis().is_parallel_to(V2f::x_axis() * random<float>()));
            REQUIRE(!V2f::x_axis().is_parallel_to(random<V2f>()));
            REQUIRE(V2f::y_axis().is_parallel_to(V2f::y_axis() * random<float>()));
            REQUIRE(!V2f::y_axis().is_parallel_to(random<V2f>()));

            REQUIRE(V2d::x_axis().is_parallel_to(V2d::x_axis() * random<double>()));
            REQUIRE(!V2d::x_axis().is_parallel_to(random<V2d>()));
            REQUIRE(V2d::y_axis().is_parallel_to(V2d::y_axis() * random<double>()));
            REQUIRE(!V2d::y_axis().is_parallel_to(random<V2d>()));

            REQUIRE(lowest_tested<V2f>().is_parallel_to(lowest_tested<V2f>()));
            REQUIRE(lowest_tested<V2f>().is_parallel_to(highest_tested<V2f>()));
            REQUIRE(lowest_tested<V2f>().is_parallel_to(V2f(1, 1)));
            REQUIRE(lowest_tested<V2f>().is_parallel_to(V2f(-0.000001f, -0.000001f)));
            REQUIRE(lowest_tested<V2f>().is_parallel_to(V2f::zero()));
            REQUIRE(highest_tested<V2f>().is_parallel_to(highest_tested<V2f>()));
            REQUIRE(highest_tested<V2f>().is_parallel_to(lowest_tested<V2f>()));
            REQUIRE(highest_tested<V2f>().is_parallel_to(V2f::zero()));

            REQUIRE(!lowest_tested<V2f>().is_parallel_to(random<V2f>()));
            REQUIRE(!lowest_tested<V2f>().is_parallel_to(lowest_tested<V2f>().get_orthogonal()));
            REQUIRE(!highest_tested<V2f>().is_parallel_to(random<V2f>()));
            REQUIRE(!highest_tested<V2f>().is_parallel_to(highest_tested<V2f>().get_orthogonal()));

            REQUIRE(lowest_tested<V2d>().is_parallel_to(lowest_tested<V2d>()));
            REQUIRE(lowest_tested<V2d>().is_parallel_to(highest_tested<V2d>()));
            REQUIRE(lowest_tested<V2d>().is_parallel_to(V2d(1, 1)));
            REQUIRE(lowest_tested<V2d>().is_parallel_to(V2d(-0.000001, -0.000001)));
            REQUIRE(lowest_tested<V2d>().is_parallel_to(V2d::zero()));
            REQUIRE(highest_tested<V2d>().is_parallel_to(highest_tested<V2d>()));
            REQUIRE(highest_tested<V2d>().is_parallel_to(lowest_tested<V2d>()));
            REQUIRE(highest_tested<V2d>().is_parallel_to(V2d::zero()));

            REQUIRE(!lowest_tested<V2d>().is_parallel_to(random<V2d>()));
            REQUIRE(!lowest_tested<V2d>().is_parallel_to(lowest_tested<V2d>().get_orthogonal()));
            REQUIRE(!highest_tested<V2d>().is_parallel_to(random<V2d>()));
            REQUIRE(!highest_tested<V2d>().is_parallel_to(highest_tested<V2d>().get_orthogonal()));
        }
    }

    WHEN("you need check if two vector2s are orthogonal") {
        const V2f vecf = random<V2f>();
        const V2d vecd = random<V2d>();

        THEN("it will work correctly") {
            REQUIRE(vecf.is_orthogonal_to(V2f::zero()));
            REQUIRE(vecf.is_orthogonal_to(vecf.get_orthogonal()));
            REQUIRE(vecf.is_orthogonal_to(vecf.get_orthogonal() *= -1));
            REQUIRE(vecf.is_orthogonal_to(vecf.get_orthogonal() * random<float>(0.1f, 2)));

            REQUIRE(!vecf.is_orthogonal_to(vecf));
            REQUIRE(!vecf.is_orthogonal_to(V2f(random<float>(), random<float>())));
            REQUIRE(!vecf.is_orthogonal_to(vecf * random<float>(0.1f, 2)));
            REQUIRE(!vecf.is_orthogonal_to(vecf * random<float>(-0.1f, -2)));
            REQUIRE(!vecf.is_orthogonal_to(-vecf));

            REQUIRE(vecd.is_orthogonal_to(V2d::zero()));
            REQUIRE(vecd.is_orthogonal_to(vecd.get_orthogonal()));
            REQUIRE(vecd.is_orthogonal_to(vecd.get_orthogonal() *= -1));
            REQUIRE(vecd.is_orthogonal_to(vecd.get_orthogonal() * random<double>(0.1, 2)));

            REQUIRE(!vecd.is_orthogonal_to(vecd));
            REQUIRE(!vecd.is_orthogonal_to(V2d(random<double>(), random<double>())));
            REQUIRE(!vecd.is_orthogonal_to(vecd * random<double>(0.1, 2)));
            REQUIRE(!vecd.is_orthogonal_to(vecd * random<double>(-0.1, -2)));
            REQUIRE(!vecd.is_orthogonal_to(-vecd));
        }
    }

    WHEN("you need check if a vector2 is unit") {
        THEN("it will work correctly") {
            for (auto i = 0; i < 10000; ++i) {
                REQUIRE(V2f::x_axis().is_unit());
                REQUIRE(V2f::y_axis().is_unit());
                REQUIRE(V2d::x_axis().is_unit());
                REQUIRE(V2d::y_axis().is_unit());

                REQUIRE(!V2f::zero().is_unit());
                REQUIRE(!V2d::zero().is_unit());

                REQUIRE(!random<V2f>().is_unit());
                REQUIRE(!random<V2d>().is_unit());

                REQUIRE(random<V2f>().normalize().is_unit());
                REQUIRE(random<V2d>().normalize().is_unit());
                REQUIRE(random<V2f>().normalize().is_unit());
                REQUIRE(random<V2d>().normalize().is_unit());

                REQUIRE(!lowest_tested<V2f>().is_unit());
                REQUIRE(lowest_tested<V2f>().normalize().is_unit());
                REQUIRE(lowest_tested<V2f>().normalize().is_unit());
                REQUIRE(!lowest_tested<V2d>().is_unit());
                REQUIRE(lowest_tested<V2d>().normalize().is_unit());
                REQUIRE(lowest_tested<V2d>().normalize().is_unit());

                REQUIRE(!highest_tested<V2f>().is_unit());
                REQUIRE(highest_tested<V2f>().normalize().is_unit());
                REQUIRE(highest_tested<V2f>().normalize().is_unit());
                REQUIRE(!highest_tested<V2d>().is_unit());
                REQUIRE(highest_tested<V2d>().normalize().is_unit());
                REQUIRE(highest_tested<V2d>().normalize().is_unit());
            }
        }
    }

    WHEN("you need check if two vector2s are approximately the same") {
        const V2f vecf = random<V2f>();
        const V2d vecd = random<V2d>();

        THEN("it will work correctly") {
            REQUIRE(vecf.is_approx(vecf));
            REQUIRE(vecd.is_approx(vecd));

            REQUIRE(vecf.is_approx(V2f(vecf.x(), vecf.y() + precision_high<float>())));
            REQUIRE(vecd.is_approx(V2d(vecd.x(), vecd.y() + precision_high<double>())));

            REQUIRE(!random<V2f>().is_approx(random<V2f>()));
            REQUIRE(!random<V2d>().is_approx(random<V2d>()));
        }
    }

    WHEN("you need the magnitude of a vector2") {
        const float factor_f = random<float>(-1, 1);
        const double factor_d = random<double>(-1, 1);

        THEN("it will work correctly") {
            REQUIRE(is_approx(V2f::x_axis().get_magnitude(), 1));
            REQUIRE(is_approx(V2f::y_axis().get_magnitude(), 1));
            REQUIRE(is_approx(V2d::x_axis().get_magnitude(), 1));
            REQUIRE(is_approx(V2d::y_axis().get_magnitude(), 1));

            REQUIRE(is_approx(V2f::zero().get_magnitude(), 0));
            REQUIRE(is_approx(V2d::zero().get_magnitude(), 0));

            REQUIRE(!is_approx(random<V2f>().get_magnitude(), 1));
            REQUIRE(!is_approx(random<V2d>().get_magnitude(), 1));

            REQUIRE(is_approx((random<V2f>().normalize() * factor_f).get_magnitude(), abs(factor_f)));
            REQUIRE(is_approx((random<V2d>().normalize() * factor_d).get_magnitude(), abs(factor_d)));

            REQUIRE(is_approx(random<V2f>().normalize().get_magnitude(), 1));
            REQUIRE(is_approx(random<V2d>().normalize().get_magnitude(), 1));
            REQUIRE(is_approx(random<V2f>().normalize().get_magnitude(), 1));
            REQUIRE(is_approx(random<V2d>().normalize().get_magnitude(), 1));

            REQUIRE(!is_approx(lowest_tested<V2f>().get_magnitude(), 1));
            REQUIRE(is_approx(lowest_tested<V2f>().normalize().get_magnitude(), 1));
            REQUIRE(is_approx(lowest_tested<V2f>().normalize().get_magnitude(), 1));
            REQUIRE(!is_approx(lowest_tested<V2d>().get_magnitude(), 1));
            REQUIRE(is_approx(lowest_tested<V2d>().normalize().get_magnitude(), 1));
            REQUIRE(is_approx(lowest_tested<V2d>().normalize().get_magnitude(), 1));

            REQUIRE(!is_approx(highest_tested<V2f>().get_magnitude(), 1));
            REQUIRE(is_approx(highest_tested<V2f>().normalize().get_magnitude(), 1));
            REQUIRE(is_approx(highest_tested<V2f>().normalize().get_magnitude(), 1));
            REQUIRE(!is_approx(highest_tested<V2d>().get_magnitude(), 1));
            REQUIRE(is_approx(highest_tested<V2d>().normalize().get_magnitude(), 1));
            REQUIRE(is_approx(highest_tested<V2d>().normalize().get_magnitude(), 1));
        }
    }
}

SCENARIO("Vector2s can be modified", "[common][vector2]") {
    WHEN("you need to set a vector2 to zero") {
        const float x_f = random<float>();
        const float y_f = random<float>();
        const int x_i = random<int>();
        const int y_i = random<int>();

        V2f random_f = V2f(x_f, y_f);
        V2i random_i = V2i(x_i, y_i);

        THEN("set_zero will work") {
            REQUIRE(!random_f.is_zero()); // may fail but not very likely
            REQUIRE(!random_i.is_zero()); // may fail but not very likely

            random_f.set_all(0);
            random_i.set_all(0);

            REQUIRE(random_f.is_zero());
            REQUIRE(random_i.is_zero());
        }
    }

    WHEN("you need to invert a vector") {
        const float x_f = random<float>();
        const float y_f = random<float>();
        const int x_i = random<int>();
        const int y_i = random<int>();

        V2f random_f = V2f(x_f, y_f);
        V2i random_i = V2i(x_i, y_i);

        V2f inv_random_f = -random_f;
        V2i inv_random_i = -random_i;

        THEN("invert and inverse will work") {
            REQUIRE(is_approx(inv_random_f.x(), -random_f.x()));
            REQUIRE(is_approx(inv_random_f.y(), -random_f.y()));

            REQUIRE(inv_random_i.x() == -random_i.x());
            REQUIRE(inv_random_i.y() == -random_i.y());

            random_f *= -1;
            random_i *= -1;

            REQUIRE(inv_random_f == random_f);
            REQUIRE(inv_random_i == random_i);
        }
    }

    WHEN("you need to get an orthogonal vector2") {
        const float x_f = random<float>();
        const float y_f = random<float>();
        const int x_i = random<int>();
        const int y_i = random<int>();

        const V2f random_f = V2f(x_f, y_f);
        const V2i random_i = V2i(x_i, y_i);

        const V2f ortho_random_f = random_f.get_orthogonal();
        const V2i ortho_random_i = random_i.get_orthogonal();

        THEN("vector2s will be rotated 90degrees counter-clockwise") {
            REQUIRE(
                is_approx(norm_angle(random_f.get_angle_to(ortho_random_f)), pi<float>() / 2, precision_low<float>()));
            REQUIRE(ortho_random_i.x() == -random_i.y());
            REQUIRE(ortho_random_i.y() == random_i.x());
        }
    }

    WHEN("you want to interpolate between two vectors2") {
        const V2f random_f1 = random<V2f>();
        const V2f random_f2 = random<V2f>();

        THEN("you can lerp them") {
            const V2f full_left = lerp(random_f1, random_f2, 0);
            const V2f full_right = lerp(random_f1, random_f2, 1);
            REQUIRE(full_left.is_approx(random_f1));
            REQUIRE(full_right.is_approx(random_f2));
        }
    }
}
