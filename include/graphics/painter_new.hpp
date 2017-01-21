#pragma once

#include "common/color.hpp"
#include "common/size2f.hpp"
#include "common/transform2.hpp"
#include "utils/enum_to_number.hpp"

namespace notf {

struct BlendMode {

    enum Mode : unsigned char {
        SOURCE_OVER,
        SOURCE_IN,
        SOURCE_OUT,
        SOURCE_ATOP,
        DESTINATION_OVER,
        DESTINATION_IN,
        DESTINATION_OUT,
        DESTINATION_ATOP,
        LIGHTER,
        COPY,
        XOR,
    };

    BlendMode()
        : rgb(SOURCE_OVER)
        , alpha(SOURCE_OVER)
    {
    }

    BlendMode(const Mode mode)
        : rgb(mode)
        , alpha(std::move(mode))
    {
    }

    BlendMode(const Mode color, const Mode alpha)
        : rgb(std::move(color))
        , alpha(std::move(alpha))
    {
    }

    const Mode rgb;
    const Mode alpha;
};

class Painter {

public:
    enum class LineCap : unsigned char {
        BUTT,
        ROUND,
        SQUARE,
    };

    enum class LineJoin : unsigned char {
        MITER,
        ROUND,
        BEVEL,
    };

public: // classes
    struct Paint {
        Paint(Color color = Color())
            : xform(Transform2::identity())
            , extent()
            , radius(0)
            , feather(1)
            , innerColor(color)
            , outerColor(color)
            , image(0)
        {
        }

        void set_color(const Color color)
        {
            innerColor = std::move(color);
            outerColor = innerColor;
        }

        Transform2 xform;
        Size2f extent;
        float radius;
        float feather;
        Color innerColor;
        Color outerColor;
        int image;
    };

    struct Scissor {
        void reset()
        {
            xform = Transform2::identity();
            extend = {-1, -1 };
        }

        Transform2 xform = Transform2::identity();
        Size2f extend    = {-1, -1};
    };
};

} // namespace notf
