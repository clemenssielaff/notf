#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "graphics/painter.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

void produce_painter(pybind11::module& module)
{
    py::class_<Painter> Py_Painter(module, "Painter");
    py::class_<NVGpaint>(module, "Paint");

    py::enum_<Painter::Winding>(Py_Painter, "Winding")
        .value("CCW", Painter::Winding::CCW)
        .value("CW", Painter::Winding::CW);

    py::enum_<Painter::LineCap>(Py_Painter, "LineCap")
        .value("BUTT", Painter::LineCap::BUTT)
        .value("ROUND", Painter::LineCap::ROUND)
        .value("SQUARE", Painter::LineCap::SQUARE);

    py::enum_<Painter::LineJoin>(Py_Painter, "LineJoin")
        .value("ROUND", Painter::LineJoin::ROUND)
        .value("BEVEL", Painter::LineJoin::BEVEL)
        .value("MITER", Painter::LineJoin::MITER);

    py::enum_<Painter::Align>(Py_Painter, "Align")
        .value("LEFT", Painter::Align::LEFT)
        .value("CENTER", Painter::Align::CENTER)
        .value("RIGHT", Painter::Align::RIGHT)
        .value("TOP", Painter::Align::TOP)
        .value("MIDDLE", Painter::Align::MIDDLE)
        .value("BOTTOM", Painter::Align::BOTTOM)
        .value("BASELINE", Painter::Align::BASELINE);

    py::enum_<Painter::Composite>(Py_Painter, "Composite")
        .value("SOURCE_OVER", Painter::Composite::SOURCE_OVER)
        .value("SOURCE_IN", Painter::Composite::SOURCE_IN)
        .value("SOURCE_OUT", Painter::Composite::SOURCE_OUT)
        .value("ATOP", Painter::Composite::ATOP)
        .value("DESTINATION_OVER", Painter::Composite::DESTINATION_OVER)
        .value("DESTINATION_IN", Painter::Composite::DESTINATION_IN)
        .value("DESTINATION_OUT", Painter::Composite::DESTINATION_OUT)
        .value("DESTINATION_ATOP", Painter::Composite::DESTINATION_ATOP)
        .value("LIGHTER", Painter::Composite::LIGHTER)
        .value("COPY", Painter::Composite::COPY)
        .value("XOR", Painter::Composite::XOR);

    py::enum_<Painter::ImageFlags>(Py_Painter, "ImageFlags")
        .value("GENERATE_MIPMAPS", Painter::ImageFlags::GENERATE_MIPMAPS)
        .value("REPEATX", Painter::ImageFlags::REPEATX)
        .value("REPEATY", Painter::ImageFlags::REPEATY)
        .value("FLIPY", Painter::ImageFlags::FLIPY)
        .value("PREMULTIPLIED", Painter::ImageFlags::PREMULTIPLIED);

    Py_Painter.def("save_state", &Painter::save_state, "Saves the current render state onto a stack.");
    Py_Painter.def("restore_state", &Painter::restore_state, "Pops and restores current render state.");
    Py_Painter.def("reset_state", &Painter::save_state, "Resets current render state to default values. Does not affect the render state stack.");

    Py_Painter.def("set_composite", &Painter::set_composite, "Determines how incoming (source) pixels are combined with existing (destination) pixels.", py::arg("composite"));
    Py_Painter.def("set_alpha", &Painter::set_alpha, "Sets the global transparency of all rendered shapes.", py::arg("alpha"));

    Py_Painter.def("set_stroke", (void (Painter::*)(const Color&)) & Painter::set_stroke, "Sets the current stroke style to a solid color.", py::arg("color"));
    Py_Painter.def("set_stroke", (void (Painter::*)(NVGpaint)) & Painter::set_stroke, "Sets the current stroke style to a paint.", py::arg("paint"));
    Py_Painter.def("set_fill", (void (Painter::*)(const Color&)) & Painter::set_fill, "Sets the current fill style to a solid color.", py::arg("color"));
    Py_Painter.def("set_fill", (void (Painter::*)(NVGpaint)) & Painter::set_fill, "Sets the current fill style to a paint.", py::arg("paint"));
    Py_Painter.def("set_stroke_width", &Painter::set_stroke_width, "Sets the width of the stroke.", py::arg("width"));
    Py_Painter.def("set_line_cap", &Painter::set_line_cap, "Sets how the end of the line (cap) is drawn - default is LineCap::BUTT.", py::arg("cap"));
    Py_Painter.def("set_line_join", &Painter::set_line_join, "Sets how sharp path corners are drawn - default is LineJoin::MITER.", py::arg("join"));
    Py_Painter.def("set_miter_limit", &Painter::set_miter_limit, "Sets the miter limit of the stroke.", py::arg("limit"));

    Py_Painter.def("LinearGradient", (NVGpaint(Painter::*)(float, float, float, float, const Color&, const Color&)) & Painter::LinearGradient, "Creates a linear gradient paint.", py::arg("sx"), py::arg("sy"), py::arg("ex"), py::arg("ey"), py::arg("start_color"), py::arg("end_color"));
    Py_Painter.def("LinearGradient", (NVGpaint(Painter::*)(const Vector2&, const Vector2&, const Color&, const Color&)) & Painter::LinearGradient, "Creates a linear gradient paint.", py::arg("start_pos"), py::arg("end_pos"), py::arg("start_color"), py::arg("end_color"));
    Py_Painter.def("BoxGradient", (NVGpaint(Painter::*)(float, float, float, float, float, float, const Color&, const Color&)) & Painter::BoxGradient, "Creates a box gradient paint.", py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"), py::arg("radius"), py::arg("feather"), py::arg("inner_color"), py::arg("outer_color"));
    Py_Painter.def("BoxGradient", (NVGpaint(Painter::*)(const Aabr&, float, float, const Color&, const Color&)) & Painter::BoxGradient, "Creates a box gradient paint.", py::arg("box"), py::arg("radius"), py::arg("feather"), py::arg("inner_color"), py::arg("outer_color"));
    Py_Painter.def("RadialGradient", (NVGpaint(Painter::*)(float, float, float, float, const Color&, const Color&)) & Painter::RadialGradient, "Creates a radial gradient paint.", py::arg("cx"), py::arg("cy"), py::arg("inr"), py::arg("outr"), py::arg("inner_color"), py::arg("outer_color"));

    Py_Painter.def("reset_transform", &Painter::reset_transform, "Resets the coordinate system to its identity.");
    Py_Painter.def("translate", (void (Painter::*)(float, float)) & Painter::translate, "Translates the coordinate system.", py::arg("x"), py::arg("y"));
    Py_Painter.def("rotate", &Painter::rotate, "Rotates the coordinate system `angle` radians in a clockwise direction.", py::arg("angle"));
    Py_Painter.def("scale", (void (Painter::*)(float)) & Painter::scale, "Scales the coordinate system uniformly in both x- and y.", py::arg("factor"));
    Py_Painter.def("scale", (void (Painter::*)(float, float)) & Painter::scale, "Scales the coorindate system in x- and y independently.", py::arg("x"), py::arg("x"));
    Py_Painter.def("skew_x", &Painter::skew_x, "Skews the coordinate system along x for `angle` radians.", py::arg("angle"));
    Py_Painter.def("skew_y", &Painter::skew_y, "Skews the coordinate system along y for `angle` radians.", py::arg("angle"));

    Py_Painter.def("set_scissor", (void (Painter::*)(float, float, float, float)) & Painter::set_scissor, "Limits all painting to the inside of the given (transformed) rectangle.", py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"));
    Py_Painter.def("intersect_scissor", (void (Painter::*)(float, float, float, float)) & Painter::intersect_scissor, "Intersects the current scissor with the given rectangle, both in the same (transformed) coordinate system.", py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"));
    Py_Painter.def("reset_scissor", &Painter::reset_scissor, "Resets the scissor rectangle and disables scissoring.");

    Py_Painter.def("start_path", &Painter::start_path, "Clears the current path and sub-paths.");
    Py_Painter.def("set_winding", &Painter::set_winding, "Sets the current sub-path winding.", py::arg("winding"));
    Py_Painter.def("move_to", (void (Painter::*)(float, float)) & Painter::move_to, "Starts new sub-path with specified point as first point.", py::arg("x"), py::arg("x"));
    Py_Painter.def("line_to", (void (Painter::*)(float, float)) & Painter::line_to, "Adds line segment from the last point in the path to the specified point.", py::arg("x"), py::arg("x"));
    Py_Painter.def("bezier_to", (void (Painter::*)(float, float, float, float, float, float)) & Painter::bezier_to, "Adds cubic bezier segment from last point in the path via two control points to the specified point.", py::arg("c1x"), py::arg("c1y"), py::arg("c2x"), py::arg("c2y"), py::arg("x"), py::arg("y"));
    Py_Painter.def("quad_to", (void (Painter::*)(float, float, float, float)) & Painter::quad_to, "Adds quadratic bezier segment from last point in the path via a control point to the specified point.", py::arg("c1x"), py::arg("c1y"), py::arg("x"), py::arg("y"));
    Py_Painter.def("arc_to", (void (Painter::*)(float, float, float, float, float)) & Painter::arc_to, "Adds an arc segment at the corner defined by the last path point, and two specified points.", py::arg("x1"), py::arg("y1"), py::arg("x2"), py::arg("y2"), py::arg("radius"));
    Py_Painter.def("arc", (void (Painter::*)(float, float, float, float, float, Painter::Winding)) & Painter::arc, "Creates new circle arc shaped sub-path.", py::arg("cx"), py::arg("cy"), py::arg("r"), py::arg("a0"), py::arg("a1"), py::arg("winding"));
    Py_Painter.def("rect", (void (Painter::*)(float, float, float, float)) & Painter::rect, "Creates new rectangle shaped sub-path.", py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"));
    Py_Painter.def("rounded_rect", (void (Painter::*)(float, float, float, float, float, float, float, float)) & Painter::rounded_rect, "Creates new rounded rectangle shaped sub-path.", py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"), py::arg("rad_nw"), py::arg("rad_ne"), py::arg("rad_se"), py::arg("rad_sw"));
    Py_Painter.def("rounded_rect", (void (Painter::*)(float, float, float, float, float)) & Painter::rounded_rect, "Creates new rounded rectangle shaped sub-path.", py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"), py::arg("radius"));
    Py_Painter.def("ellipse", (void (Painter::*)(float, float, float, float)) & Painter::ellipse, "Creates new ellipse shaped sub-path.", py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"));
    Py_Painter.def("circle", (void (Painter::*)(float, float, float)) & Painter::circle, "Creates new circle shaped sub-path.", py::arg("x"), py::arg("y"), py::arg("radius"));
    Py_Painter.def("close_path", &Painter::close_path, "Closes current sub-path with a line segment.");
    Py_Painter.def("fill", &Painter::fill, "Fills the current path with current fill style.");
    Py_Painter.def("stroke", &Painter::stroke, "Fills the current path with current stroke style.");
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
