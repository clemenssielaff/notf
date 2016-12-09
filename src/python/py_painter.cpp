#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "graphics/painter.hpp"
#include "python/docstr.hpp"
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

    Py_Painter.def("get_widget_size", &Painter::get_widget_size, DOCSTR("Returns the size of the Widget in local coordinates."));
    Py_Painter.def("get_buffer_size", &Painter::get_buffer_size, DOCSTR("Returns the size of the Window's framebuffer in pixels."));
    Py_Painter.def("get_mouse_pos", &Painter::get_mouse_pos, DOCSTR("Returns the mouse position in the Widget's coordinate system."));
    Py_Painter.def("get_time", &Painter::get_time, DOCSTR("Returns the time since Application start in seconds."));

    Py_Painter.def("save_state", &Painter::save_state, DOCSTR("Saves the current render state onto a stack."));
    Py_Painter.def("restore_state", &Painter::restore_state, DOCSTR("Pops and restores current render state."));
    Py_Painter.def("reset_state", &Painter::save_state, DOCSTR("Resets current render state to default values. Does not affect the render state stack."));

    Py_Painter.def("set_composite", &Painter::set_composite, DOCSTR("Determines how incoming (source) pixels are combined with existing (destination) pixels."), py::arg("composite"));
    Py_Painter.def("set_alpha", &Painter::set_alpha, DOCSTR("Sets the global transparency of all rendered shapes."), py::arg("alpha"));

    Py_Painter.def("set_stroke", (void (Painter::*)(const Color&)) & Painter::set_stroke, DOCSTR("Sets the current stroke style to a solid color."), py::arg("color"));
    Py_Painter.def("set_stroke", (void (Painter::*)(NVGpaint)) & Painter::set_stroke, DOCSTR("Sets the current stroke style to a paint."), py::arg("paint"));
    Py_Painter.def("set_fill", (void (Painter::*)(const Color&)) & Painter::set_fill, DOCSTR("Sets the current fill style to a solid color."), py::arg("color"));
    Py_Painter.def("set_fill", (void (Painter::*)(NVGpaint)) & Painter::set_fill, DOCSTR("Sets the current fill style to a paint."), py::arg("paint"));
    Py_Painter.def("set_stroke_width", &Painter::set_stroke_width, DOCSTR("Sets the width of the stroke."), py::arg("width"));
    Py_Painter.def("set_line_cap", &Painter::set_line_cap, DOCSTR("Sets how the end of the line (cap) is drawn - default is LineCap::BUTT."), py::arg("cap"));
    Py_Painter.def("set_line_join", &Painter::set_line_join, DOCSTR("Sets how sharp path corners are drawn - default is LineJoin::MITER."), py::arg("join"));
    Py_Painter.def("set_miter_limit", &Painter::set_miter_limit, DOCSTR("Sets the miter limit of the stroke."), py::arg("limit"));

    Py_Painter.def("LinearGradient", (NVGpaint(Painter::*)(float, float, float, float, const Color&, const Color&)) & Painter::LinearGradient, DOCSTR("Creates a linear gradient paint."), py::arg("sx"), py::arg("sy"), py::arg("ex"), py::arg("ey"), py::arg("start_color"), py::arg("end_color"));
    Py_Painter.def("LinearGradient", (NVGpaint(Painter::*)(const Vector2&, const Vector2&, const Color&, const Color&)) & Painter::LinearGradient, DOCSTR("Creates a linear gradient paint."), py::arg("start_pos"), py::arg("end_pos"), py::arg("start_color"), py::arg("end_color"));
    Py_Painter.def("BoxGradient", (NVGpaint(Painter::*)(float, float, float, float, float, float, const Color&, const Color&)) & Painter::BoxGradient, DOCSTR("Creates a box gradient paint."), py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"), py::arg("radius"), py::arg("feather"), py::arg("inner_color"), py::arg("outer_color"));
    Py_Painter.def("BoxGradient", (NVGpaint(Painter::*)(const Aabr&, float, float, const Color&, const Color&)) & Painter::BoxGradient, DOCSTR("Creates a box gradient paint."), py::arg("box"), py::arg("radius"), py::arg("feather"), py::arg("inner_color"), py::arg("outer_color"));
    Py_Painter.def("RadialGradient", (NVGpaint(Painter::*)(float, float, float, float, const Color&, const Color&)) & Painter::RadialGradient, DOCSTR("Creates a radial gradient paint."), py::arg("cx"), py::arg("cy"), py::arg("inr"), py::arg("outr"), py::arg("inner_color"), py::arg("outer_color"));
    Py_Painter.def("RadialGradient", (NVGpaint(Painter::*)(const Vector2&, float, float, const Color&, const Color&)) & Painter::RadialGradient, DOCSTR("Creates a radial gradient paint."), py::arg("center"), py::arg("inr"), py::arg("outr"), py::arg("inner_color"), py::arg("outer_color"));
    Py_Painter.def("ImagePattern", (NVGpaint(Painter::*)(const std::shared_ptr<Texture2>&, float, float, float, float, float)) & Painter::ImagePattern, DOCSTR("Creates an image paint."), py::arg("texture"), py::arg("offset_x") = 0.f, py::arg("offset_y") = 0.f, py::arg("width") = -1.f, py::arg("height") = -1.f, py::arg("angle") = 0.f);
    Py_Painter.def("ImagePattern", (NVGpaint(Painter::*)(const std::shared_ptr<Texture2>&, const Vector2&, Size2f size, float)) & Painter::ImagePattern, DOCSTR("Creates an image paint."), py::arg("texture"), py::arg("offset") = Vector2::zero(), py::arg("size") = Size2f({0, 0}), py::arg("angle") = 0.f);
    Py_Painter.def("ImagePattern", (NVGpaint(Painter::*)(const std::shared_ptr<Texture2>&, const Aabr&, float)) & Painter::ImagePattern, DOCSTR("Creates an image paint."), py::arg("texture"), py::arg("aabr"), py::arg("angle") = 0.f);

    Py_Painter.def("reset_transform", &Painter::reset_transform, DOCSTR("Resets the coordinate system to its identity."));
    Py_Painter.def("translate", (void (Painter::*)(float, float)) & Painter::translate, DOCSTR("Translates the coordinate system."), py::arg("x"), py::arg("y"));
    Py_Painter.def("translate", (void (Painter::*)(const Vector2&)) & Painter::translate, DOCSTR("Translates the coordinate system."), py::arg("position"));
    Py_Painter.def("rotate", &Painter::rotate, DOCSTR("Rotates the coordinate system `angle` radians in a clockwise direction."), py::arg("angle"));
    Py_Painter.def("scale", (void (Painter::*)(float)) & Painter::scale, DOCSTR("Scales the coordinate system uniformly in both x- and y."), py::arg("factor"));
    Py_Painter.def("scale", (void (Painter::*)(float, float)) & Painter::scale, DOCSTR("Scales the coorindate system in x- and y independently."), py::arg("x"), py::arg("x"));
    Py_Painter.def("skew_x", &Painter::skew_x, DOCSTR("Skews the coordinate system along x for `angle` radians."), py::arg("angle"));
    Py_Painter.def("skew_y", &Painter::skew_y, DOCSTR("Skews the coordinate system along y for `angle` radians."), py::arg("angle"));

    Py_Painter.def("set_scissor", (void (Painter::*)(float, float, float, float)) & Painter::set_scissor, DOCSTR("Limits all painting to the inside of the given (transformed) rectangle."), py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"));
    Py_Painter.def("set_scissor", (void (Painter::*)(const Aabr&)) & Painter::set_scissor, DOCSTR("Limits all painting to the inside of the given (transformed) rectangle."), py::arg("aabr"));
    Py_Painter.def("intersect_scissor", (void (Painter::*)(float, float, float, float)) & Painter::intersect_scissor, DOCSTR("Intersects the current scissor with the given rectangle, both in the same (transformed) coordinate system."), py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"));
    Py_Painter.def("intersect_scissor", (void (Painter::*)(const Aabr&)) & Painter::intersect_scissor, DOCSTR("Intersects the current scissor with the given rectangle, both in the same (transformed) coordinate system."), py::arg("aabr"));
    Py_Painter.def("reset_scissor", &Painter::reset_scissor, DOCSTR("Resets the scissor rectangle and disables scissoring."));

    Py_Painter.def("begin", &Painter::begin, DOCSTR("Clears the current path and sub-paths and begins a new one."));
    Py_Painter.def("set_winding", &Painter::set_winding, DOCSTR("Sets the current sub-path winding."), py::arg("winding"));
    Py_Painter.def("move_to", (void (Painter::*)(float, float)) & Painter::move_to, DOCSTR("Starts new sub-path with specified point as first point."), py::arg("x"), py::arg("x"));
    Py_Painter.def("move_to", (void (Painter::*)(const Vector2&)) & Painter::move_to, DOCSTR("Starts new sub-path with specified point as first point."), py::arg("position"));
    Py_Painter.def("close", &Painter::close, DOCSTR("Closes current sub-path with a line segment."));
    Py_Painter.def("fill", &Painter::fill, DOCSTR("Fills the current path with current fill style."));
    Py_Painter.def("stroke", &Painter::stroke, DOCSTR("Fills the current path with current stroke style."));

    Py_Painter.def("line_to", (void (Painter::*)(float, float)) & Painter::line_to, DOCSTR("Adds line segment from the last point in the path to the specified point."), py::arg("x"), py::arg("x"));
    Py_Painter.def("line_to", (void (Painter::*)(const Vector2&)) & Painter::line_to, DOCSTR("Adds line segment from the last point in the path to the specified point."), py::arg("position"));
    Py_Painter.def("bezier_to", (void (Painter::*)(float, float, float, float, float, float)) & Painter::bezier_to, DOCSTR("Adds cubic bezier segment from last point in the path via two control points to the specified point."), py::arg("c1x"), py::arg("c1y"), py::arg("c2x"), py::arg("c2y"), py::arg("x"), py::arg("y"));
    Py_Painter.def("bezier_to", (void (Painter::*)(const Vector2&, const Vector2&, const Vector2&)) & Painter::bezier_to, DOCSTR("Adds cubic bezier segment from last point in the path via two control points to the specified point."), py::arg("ctrl_1"), py::arg("ctrl_2"), py::arg("end"));
    Py_Painter.def("quad_to", (void (Painter::*)(float, float, float, float)) & Painter::quad_to, DOCSTR("Adds quadratic bezier segment from last point in the path via a control point to the specified point."), py::arg("c1x"), py::arg("c1y"), py::arg("x"), py::arg("y"));
    Py_Painter.def("quad_to", (void (Painter::*)(const Vector2&, const Vector2&)) & Painter::quad_to, DOCSTR("Adds quadratic bezier segment from last point in the path via a control point to the specified point."), py::arg("ctrl"), py::arg("end"));
    Py_Painter.def("arc_to", (void (Painter::*)(float, float, float, float, float)) & Painter::arc_to, DOCSTR("Adds an arc segment at the corner defined by the last path point, and two specified points."), py::arg("x1"), py::arg("y1"), py::arg("x2"), py::arg("y2"), py::arg("radius"));
    Py_Painter.def("arc_to", (void (Painter::*)(const Vector2&, const Vector2&, float)) & Painter::arc_to, DOCSTR("Adds an arc segment at the corner defined by the last path point, and two specified points."), py::arg("ctrl"), py::arg("end"), py::arg("radius"));
    Py_Painter.def("arc", (void (Painter::*)(float, float, float, float, float, Painter::Winding)) & Painter::arc, DOCSTR("Creates new circle arc shaped sub-path."), py::arg("cx"), py::arg("cy"), py::arg("r"), py::arg("a0"), py::arg("a1"), py::arg("winding"));
    Py_Painter.def("arc", (void (Painter::*)(const Vector2&, float, float, float, Painter::Winding)) & Painter::arc, DOCSTR("Creates new circle arc shaped sub-path."), py::arg("pos"), py::arg("r"), py::arg("a0"), py::arg("a1"), py::arg("winding"));
    Py_Painter.def("arc", (void (Painter::*)(const Circle&, float, float, Painter::Winding)) & Painter::arc, DOCSTR("Creates new circle arc shaped sub-path."), py::arg("circle"), py::arg("a0"), py::arg("a1"), py::arg("winding"));
    Py_Painter.def("rect", (void (Painter::*)(float, float, float, float)) & Painter::rect, DOCSTR("Creates new rectangle shaped sub-path."), py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"));
    Py_Painter.def("rect", (void (Painter::*)(const Aabr&)) & Painter::rect, DOCSTR("Creates new rectangle shaped sub-path."), py::arg("aabr"));
    Py_Painter.def("rounded_rect", (void (Painter::*)(float, float, float, float, float)) & Painter::rounded_rect, DOCSTR("Creates new rounded rectangle shaped sub-path."), py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"), py::arg("radius"));
    Py_Painter.def("rounded_rect", (void (Painter::*)(float, float, float, float, float, float, float, float)) & Painter::rounded_rect, DOCSTR("Creates new rounded rectangle shaped sub-path."), py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"), py::arg("rad_nw"), py::arg("rad_ne"), py::arg("rad_se"), py::arg("rad_sw"));
    Py_Painter.def("rounded_rect", (void (Painter::*)(const Aabr&, float)) & Painter::rounded_rect, DOCSTR("Creates new rounded rectangle shaped sub-path."), py::arg("aabr"), py::arg("radius"));
    Py_Painter.def("rounded_rect", (void (Painter::*)(const Aabr&, float, float, float, float)) & Painter::rounded_rect, DOCSTR("Creates new rounded rectangle shaped sub-path."), py::arg("aabr"), py::arg("rad_nw"), py::arg("rad_ne"), py::arg("rad_se"), py::arg("rad_sw"));
    Py_Painter.def("ellipse", (void (Painter::*)(float, float, float, float)) & Painter::ellipse, DOCSTR("Creates new ellipse shaped sub-path."), py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"));
    Py_Painter.def("circle", (void (Painter::*)(float, float, float)) & Painter::circle, DOCSTR("Creates new circle shaped sub-path."), py::arg("x"), py::arg("y"), py::arg("radius"));
    Py_Painter.def("circle", (void (Painter::*)(const Vector2&, float)) & Painter::circle, DOCSTR("Creates new circle shaped sub-path."), py::arg("pos"), py::arg("radius"));
    Py_Painter.def("circle", (void (Painter::*)(const Circle&)) & Painter::circle, DOCSTR("Creates new circle shaped sub-path."), py::arg("circle"));

    Py_Painter.def("set_font_size", &Painter::set_font_size, DOCSTR("Sets the font size of the current text style."), py::arg("size"));
    Py_Painter.def("set_font_blur", &Painter::set_font_blur, DOCSTR("Sets the font blur of the current text style."), py::arg("blur"));
    Py_Painter.def("set_letter_spacing", &Painter::set_letter_spacing, DOCSTR("Sets the letter spacing of the current text style."), py::arg("spacing"));
    Py_Painter.def("set_line_height", &Painter::set_line_height, DOCSTR("Sets the proportional line height of the current text style."), py::arg("height"));
    Py_Painter.def("set_text_align", &Painter::set_text_align, DOCSTR("Sets the text align of the current text style."), py::arg("align"));
    Py_Painter.def("set_font", &Painter::set_font, DOCSTR("Sets the font of the current text style."), py::arg("font"));
    Py_Painter.def("text", (float (Painter::*)(float, float, const std::string&, size_t)) & Painter::text, DOCSTR("Draws a text at the specified location up to `length` characters long."), py::arg("x"), py::arg("y"), py::arg("string"), py::arg("length") = 0);
    Py_Painter.def("text", (float (Painter::*)(const Vector2&, const std::string&, size_t)) & Painter::text, DOCSTR("Draws a text at the specified location up to `length` characters long."), py::arg("position"), py::arg("string"), py::arg("length") = 0);
    Py_Painter.def("text_box", (void (Painter::*)(float, float, float, const std::string&, size_t)) & Painter::text_box, DOCSTR("Draws a multi-line text box at the specified location, wrapped at `width`, up to `length` characters long."), py::arg("x"), py::arg("y"), py::arg("width"), py::arg("string"), py::arg("length") = 0);
    Py_Painter.def("text_box", (void (Painter::*)(const Vector2&, float, const std::string&, size_t)) & Painter::text_box, DOCSTR("Draws a multi-line text box at the specified location, wrapped at `width`, up to `length` characters long."), py::arg("position"), py::arg("width"), py::arg("string"), py::arg("length") = 0);
    Py_Painter.def("text_box", (void (Painter::*)(const Aabr&, const std::string&, size_t)) & Painter::text_box, DOCSTR("Draws a multi-line text box at the specified location, wrapped at `width`, up to `length` characters long."), py::arg("rect"), py::arg("string"), py::arg("length") = 0);
    Py_Painter.def("text_bounds", (Aabr(Painter::*)(float, float, const std::string&, size_t)) & Painter::text_bounds, DOCSTR("Returns the bounding box of the specified text in local space."), py::arg("x"), py::arg("y"), py::arg("string"), py::arg("length") = 0);
    Py_Painter.def("text_bounds", (Aabr(Painter::*)(const Vector2&, const std::string&, size_t)) & Painter::text_bounds, DOCSTR("Returns the bounding box of the specified text in local space."), py::arg("position"), py::arg("string"), py::arg("length") = 0);
    Py_Painter.def("text_bounds", (Aabr(Painter::*)(const std::string&, size_t)) & Painter::text_bounds, DOCSTR("Returns the bounding box of the specified text in local space."), py::arg("string"), py::arg("length") = 0);
    Py_Painter.def("text_box_bounds", (Aabr(Painter::*)(float, float, float, const std::string&, size_t)) & Painter::text_box_bounds, DOCSTR("Returns the bounding box of the specified text box in local space."), py::arg("x"), py::arg("y"), py::arg("width"), py::arg("string"), py::arg("length") = 0);
    Py_Painter.def("text_box_bounds", (Aabr(Painter::*)(const Vector2&, float, const std::string&, size_t)) & Painter::text_box_bounds, DOCSTR("Returns the bounding box of the specified text box in local space."), py::arg("position"), py::arg("width"), py::arg("string"), py::arg("length") = 0);
    Py_Painter.def("text_box_bounds", (Aabr(Painter::*)(const Aabr&, const std::string&, size_t)) & Painter::text_box_bounds, DOCSTR("Returns the bounding box of the specified text box in local space."), py::arg("rect"), py::arg("string"), py::arg("length") = 0);
    Py_Painter.def("text_box_bounds", (Aabr(Painter::*)(float, const std::string&, size_t)) & Painter::text_box_bounds, DOCSTR("Returns the bounding box of the specified text box in local space."), py::arg("width"), py::arg("string"), py::arg("length") = 0);
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
