#include "python/pypainter.hpp"
#include "graphics/painter.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#endif

/** "Trampoline" painter specialization that can be subclassed from Python. */
class PyPainter : public Painter {
    friend py::class_<Painter, PyPainter> produce_painter(pybind11::module& module);

public:
    using Painter::Painter;

    PyPainter(Painter*&) {}

    virtual void paint() override { PYBIND11_OVERLOAD(void, Painter, paint,); }
};

py::class_<Painter, PyPainter> produce_painter(pybind11::module& module)
{
    py::class_<Painter, PyPainter> Py_Painter(module, "Painter");

    py::enum_<PyPainter::Winding>(Py_Painter, "Winding")
        .value("CCW", PyPainter::Winding::CCW)
        .value("CW", PyPainter::Winding::CW);

    py::enum_<PyPainter::LineCap>(Py_Painter, "LineCap")
        .value("BUTT", PyPainter::LineCap::BUTT)
        .value("ROUND", PyPainter::LineCap::ROUND)
        .value("SQUARE", PyPainter::LineCap::SQUARE);

    py::enum_<PyPainter::LineJoin>(Py_Painter, "LineJoin")
        .value("ROUND", PyPainter::LineJoin::ROUND)
        .value("BEVEL", PyPainter::LineJoin::BEVEL)
        .value("MITER", PyPainter::LineJoin::MITER);

    py::enum_<PyPainter::Align>(Py_Painter, "Align")
        .value("LEFT", PyPainter::Align::LEFT)
        .value("CENTER", PyPainter::Align::CENTER)
        .value("RIGHT", PyPainter::Align::RIGHT)
        .value("TOP", PyPainter::Align::TOP)
        .value("MIDDLE", PyPainter::Align::MIDDLE)
        .value("BOTTOM", PyPainter::Align::BOTTOM)
        .value("BASELINE", PyPainter::Align::BASELINE);

    py::enum_<PyPainter::Composite>(Py_Painter, "Composite")
        .value("SOURCE_OVER", PyPainter::Composite::SOURCE_OVER)
        .value("SOURCE_IN", PyPainter::Composite::SOURCE_IN)
        .value("SOURCE_OUT", PyPainter::Composite::SOURCE_OUT)
        .value("ATOP", PyPainter::Composite::ATOP)
        .value("DESTINATION_OVER", PyPainter::Composite::DESTINATION_OVER)
        .value("DESTINATION_IN", PyPainter::Composite::DESTINATION_IN)
        .value("DESTINATION_OUT", PyPainter::Composite::DESTINATION_OUT)
        .value("DESTINATION_ATOP", PyPainter::Composite::DESTINATION_ATOP)
        .value("LIGHTER", PyPainter::Composite::LIGHTER)
        .value("COPY", PyPainter::Composite::COPY)
        .value("XOR", PyPainter::Composite::XOR);

    py::enum_<PyPainter::ImageFlags>(Py_Painter, "ImageFlags")
        .value("GENERATE_MIPMAPS", PyPainter::ImageFlags::GENERATE_MIPMAPS)
        .value("REPEATX", PyPainter::ImageFlags::REPEATX)
        .value("REPEATY", PyPainter::ImageFlags::REPEATY)
        .value("FLIPY", PyPainter::ImageFlags::FLIPY)
        .value("PREMULTIPLIED", PyPainter::ImageFlags::PREMULTIPLIED);

    Py_Painter.def(py::init<>());

    Py_Painter.def("save_state", &PyPainter::save_state, "Saves the current render state onto a stack.");
    Py_Painter.def("restore_state", &PyPainter::restore_state, "Pops and restores current render state.");
    Py_Painter.def("reset_state", &PyPainter::save_state, "Resets current render state to default values. Does not affect the render state stack.");

    Py_Painter.def("set_composite", &PyPainter::set_composite, "Determines how incoming (source) pixels are combined with existing (destination) pixels.", py::arg("composite"));
    Py_Painter.def("set_alpha", &PyPainter::set_alpha, "Sets the global transparency of all rendered shapes.", py::arg("alpha"));

    Py_Painter.def("set_stroke", &PyPainter::set_stroke, "Sets the current stroke style to a solid color.", py::arg("color"));
    Py_Painter.def("set_fill", &PyPainter::set_fill, "Sets the current fill style to a solid color.", py::arg("color"));
    Py_Painter.def("set_stroke_width", &PyPainter::set_stroke_width, "Sets the width of the stroke.", py::arg("width"));
    Py_Painter.def("set_line_cap", &PyPainter::set_line_cap, "Sets how the end of the line (cap) is drawn - default is LineCap::BUTT.", py::arg("cap"));
    Py_Painter.def("set_line_join", &PyPainter::set_line_join, "Sets how sharp path corners are drawn - default is LineJoin::MITER.", py::arg("join"));
    Py_Painter.def("set_miter_limit", &PyPainter::set_miter_limit, "Sets the miter limit of the stroke.", py::arg("limit"));

    Py_Painter.def("reset_transform", &PyPainter::reset_transform, "Resets the coordinate system to its identity.");
    Py_Painter.def("translate", (void (PyPainter::*)(float, float)) & PyPainter::translate, "Translates the coordinate system.", py::arg("x"), py::arg("y"));
    Py_Painter.def("rotate", &PyPainter::rotate, "Rotates the coordinate system `angle` radians in a clockwise direction.", py::arg("angle"));
    Py_Painter.def("scale", (void (PyPainter::*)(float)) & PyPainter::scale, "Scales the coordinate system uniformly in both x- and y.", py::arg("factor"));
    Py_Painter.def("scale", (void (PyPainter::*)(float, float)) & PyPainter::scale, "Scales the coorindate system in x- and y independently.", py::arg("x"), py::arg("x"));
    Py_Painter.def("skew_x", &PyPainter::skew_x, "Skews the coordinate system along x for `angle` radians.", py::arg("angle"));
    Py_Painter.def("skew_y", &PyPainter::skew_y, "Skews the coordinate system along y for `angle` radians.", py::arg("angle"));

    Py_Painter.def("set_scissor", (void (PyPainter::*)(float, float, float, float)) & PyPainter::set_scissor, "Limits all painting to the inside of the given (transformed) rectangle.", py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"));
    Py_Painter.def("intersect_scissor", (void (PyPainter::*)(float, float, float, float)) & PyPainter::intersect_scissor, "Intersects the current scissor with the given rectangle, both in the same (transformed) coordinate system.", py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"));
    Py_Painter.def("reset_scissor", &PyPainter::reset_scissor, "Resets the scissor rectangle and disables scissoring.");

    Py_Painter.def("start_path", &PyPainter::start_path, "Clears the current path and sub-paths.");
    Py_Painter.def("set_winding", &PyPainter::set_winding, "Sets the current sub-path winding.", py::arg("winding"));
    Py_Painter.def("move_to", (void (PyPainter::*)(float, float)) & PyPainter::move_to, "Starts new sub-path with specified point as first point.", py::arg("x"), py::arg("x"));
    Py_Painter.def("line_to", (void (PyPainter::*)(float, float)) & PyPainter::line_to, "Adds line segment from the last point in the path to the specified point.", py::arg("x"), py::arg("x"));
    Py_Painter.def("bezier_to", (void (PyPainter::*)(float, float, float, float, float, float)) & PyPainter::bezier_to, "Adds cubic bezier segment from last point in the path via two control points to the specified point.", py::arg("c1x"), py::arg("c1y"), py::arg("c2x"), py::arg("c2y"), py::arg("x"), py::arg("y"));
    Py_Painter.def("quad_to", (void (PyPainter::*)(float, float, float, float)) & PyPainter::quad_to, "Adds quadratic bezier segment from last point in the path via a control point to the specified point.", py::arg("c1x"), py::arg("c1y"), py::arg("x"), py::arg("y"));
    Py_Painter.def("arc_to", (void (PyPainter::*)(float, float, float, float, float)) & PyPainter::arc_to, "Adds an arc segment at the corner defined by the last path point, and two specified points.", py::arg("x1"), py::arg("y1"), py::arg("x2"), py::arg("y2"), py::arg("radius"));
    Py_Painter.def("arc", (void (PyPainter::*)(float, float, float, float, float, PyPainter::Winding)) & PyPainter::arc, "Creates new circle arc shaped sub-path.", py::arg("cx"), py::arg("cy"), py::arg("r"), py::arg("a0"), py::arg("a1"), py::arg("winding"));
    Py_Painter.def("rect", (void (PyPainter::*)(float, float, float, float)) & PyPainter::rect, "Creates new rectangle shaped sub-path.", py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"));
    Py_Painter.def("rounded_rect", (void (PyPainter::*)(float, float, float, float, float, float, float, float)) & PyPainter::rounded_rect, "Creates new rounded rectangle shaped sub-path.", py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"), py::arg("rad_nw"), py::arg("rad_ne"), py::arg("rad_se"), py::arg("rad_sw"));
    Py_Painter.def("rounded_rect", (void (PyPainter::*)(float, float, float, float, float)) & PyPainter::rounded_rect, "Creates new rounded rectangle shaped sub-path.", py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"), py::arg("radius"));
    Py_Painter.def("ellipse", (void (PyPainter::*)(float, float, float, float)) & PyPainter::ellipse, "Creates new ellipse shaped sub-path.", py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"));
    Py_Painter.def("circle", (void (PyPainter::*)(float, float, float)) & PyPainter::circle, "Creates new circle shaped sub-path.", py::arg("x"), py::arg("y"), py::arg("radius"));
    Py_Painter.def("close_path", &PyPainter::close_path, "Closes current sub-path with a line segment.");
    Py_Painter.def("fill", &PyPainter::fill, "Fills the current path with current fill style.");
    Py_Painter.def("stroke", &PyPainter::stroke, "Fills the current path with current stroke style.");

    return Py_Painter;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
