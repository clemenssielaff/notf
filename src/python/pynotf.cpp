#include "python/pynotf.hpp"

#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
#include "pybind11/functional.h"
namespace py = pybind11;

PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);

#include "common/string_utils.hpp"
#include "common/vector2.hpp"
#include "core/widget.hpp"
#include "python/interpreter.hpp"
#include "graphics/painter.hpp"
#include "core/application.hpp"
#include "core/window.hpp"
#include "core/layout_root.hpp"
#include "core/layout_item.hpp"
#include "dynamic/layout/stack_layout.hpp"
#include "core/components/canvas_component.hpp"
using namespace notf;

const char* python_notf_module_name = "notf";

PyObject* produce_pynotf_module()
{
    py::module module(python_notf_module_name, "NoTF Python bindings");

    // common
    py::enum_<STACK_DIRECTION>(module, "STACK_DIRECTION")
        .value("LEFT_TO_RIGHT", STACK_DIRECTION::LEFT_TO_RIGHT)
        .value("TOP_TO_BOTTOM", STACK_DIRECTION::TOP_TO_BOTTOM)
        .value("RIGHT_TO_LEFT", STACK_DIRECTION::RIGHT_TO_LEFT)
        .value("BOTTOM_TO_TOP", STACK_DIRECTION::BOTTOM_TO_TOP);

    // vector2
    {
        py::class_<Vector2> Py_Vector2(module, "Vector2");

        // constructors
        Py_Vector2.def(py::init<>());
        Py_Vector2.def(py::init<Real, Real>());
        Py_Vector2.def(py::init<Vector2>());

        // static constructors
        Py_Vector2.def_static("fill", &Vector2::fill, "Returns a Vector2 with both components set to the given value", py::arg("value"));
        Py_Vector2.def_static("x_axis", &Vector2::x_axis, "Returns an unit Vector2 along the x-axis.");
        Py_Vector2.def_static("y_axis", &Vector2::y_axis, "Returns an unit Vector2 along the y-axis.");

        // properties
        Py_Vector2.def_readwrite("x", &Vector2::x);
        Py_Vector2.def_readwrite("y", &Vector2::y);

        // inspections
        Py_Vector2.def("is_zero", (bool (Vector2::*)() const) & Vector2::is_zero, "Checks if this Vector2 is the zero vector.");
        Py_Vector2.def("is_zero", (bool (Vector2::*)(Real) const) & Vector2::is_zero, "Checks if this Vector2 is the zero vector.", py::arg("epsilon"));
        Py_Vector2.def("is_unit", &Vector2::is_unit, "Checks whether this Vector2 is of unit magnitude.");
        Py_Vector2.def("is_parallel_to", &Vector2::is_parallel_to, "Checks whether this Vector2 is parallel to other.", py::arg("other"));
        Py_Vector2.def("is_orthogonal_to", &Vector2::is_orthogonal_to, "Checks whether this Vector2 is orthogonal to other.", py::arg("other"));
        Py_Vector2.def("angle", &Vector2::angle, "The angle in radians between the positive x-axis and the point given by this Vector2.");
        Py_Vector2.def("angle_to", &Vector2::angle_to, "Calculates the smallest angle between two Vector2s in radians.", py::arg("other"));
        Py_Vector2.def("direction_to", &Vector2::direction_to, "Tests if the other Vector2 is collinear (1) to this, opposite (-1) or something in between.", py::arg("other"));
        Py_Vector2.def("is_horizontal", &Vector2::is_horizontal, "Tests if this Vector2 is parallel to the x-axis.");
        Py_Vector2.def("is_vertical", &Vector2::is_vertical, "Tests if this Vector2 is parallel to the y-axis.");
        Py_Vector2.def("is_approx", &Vector2::is_approx, "Returns True, if other and self are approximately the same Vector2.", py::arg("other"));
        Py_Vector2.def("is_not_approx", &Vector2::is_not_approx, "Returns True, if other and self are NOT approximately the same Vector2.", py::arg("other"));
        Py_Vector2.def("slope", &Vector2::slope, "Returns the slope of this Vector2.");
        Py_Vector2.def("magnitude_sq", &Vector2::magnitude_sq, "Returns the squared magnitude of this Vector2.");
        Py_Vector2.def("magnitude", &Vector2::magnitude, "Returns the magnitude of this Vector2.");
        Py_Vector2.def("is_real", &Vector2::is_real, "Checks, if this Vector2 contains only real values.");
        Py_Vector2.def("contains_zero", &Vector2::contains_zero, "Checks, if any component of this Vector2 is a zero.");

        // operators
        Py_Vector2.def(py::self == py::self);
        Py_Vector2.def(py::self != py::self);
        Py_Vector2.def(py::self + py::self);
        Py_Vector2.def(py::self += py::self);
        Py_Vector2.def(py::self - py::self);
        Py_Vector2.def(py::self -= py::self);
        Py_Vector2.def(py::self * py::self);
        Py_Vector2.def(py::self *= py::self);
        Py_Vector2.def(py::self / py::self);
        Py_Vector2.def(py::self /= py::self);
        Py_Vector2.def(py::self * Real());
        Py_Vector2.def(py::self *= Real());
        Py_Vector2.def(py::self / Real());
        Py_Vector2.def(py::self /= Real());
        Py_Vector2.def(-py::self);

        // modifiers
        Py_Vector2.def("set_zero", &Vector2::set_zero, "Sets all components of the Vector to zero.");
        Py_Vector2.def("inverted", &Vector2::inverted, "Returns an inverted copy of this Vector2.");
        Py_Vector2.def("invert", &Vector2::invert, "Inverts this Vector2 in-place.");
        Py_Vector2.def("dot", &Vector2::dot, "Vector2 dot product.", py::arg("other"));
        Py_Vector2.def("normalized", &Vector2::normalized, "Returns a normalized copy of this Vector2.");
        Py_Vector2.def("normalize", &Vector2::normalize, "Inverts this Vector2 in-place.");
        Py_Vector2.def("projected_on", &Vector2::projected_on, "Creates a projection of this Vector2 onto an infinite line whose direction is specified by other.", py::arg("other"));
        Py_Vector2.def("project_on", &Vector2::project_on, "Projects this Vector2 onto an infinite line whose direction is specified by other.", py::arg("other"));
        Py_Vector2.def("orthogonal", &Vector2::orthogonal, "Creates an orthogonal 2D Vector to this one by rotating it 90 degree counter-clockwise.");
        Py_Vector2.def("orthogonalize", &Vector2::orthogonalize, "In-place rotation of this Vector2 90 degrees counter-clockwise.");
        Py_Vector2.def("rotated", &Vector2::rotated, "Returns a copy of this 2D Vector rotated counter-clockwise around its origin by a given angle in radians.", py::arg("angle"));
        Py_Vector2.def("rotate", &Vector2::rotate, "Rotates this Vector2 counter-clockwise in-place around its origin by a given angle in radians.", py::arg("angle"));
        Py_Vector2.def("side_of", &Vector2::side_of, "The side of the other 2D Vector relative to direction of this 2D Vector (+1 = left, -1 = right).", py::arg("other"));

        // representation
        Py_Vector2.def("__repr__", [](const Vector2& vec) { return string_format("notf.Vector2(%f, %f)", vec.x, vec.y); });

        // free functions
        module.def("orthonormal_basis", (void (*)(Vector2&, Vector2&)) & orthonormal_basis, "Constructs a duo of mutually orthogonal, normalized vectors with 'a' as the reference vector", py::arg("a"), py::arg("b"));
        module.def("lerp", (Vector2(*)(const Vector2&, const Vector2&, Real)) & lerp, "Linear interpolation between two Vector2s.", py::arg("from"), py::arg("to"), py::arg("blend"));
        module.def("nlerp", (Vector2(*)(const Vector2&, const Vector2&, Real)) & nlerp, "Normalized linear interpolation between two Vector2s.", py::arg("from"), py::arg("to"), py::arg("blend"));

    }

    // core

    // window
    {
        py::class_<Window, std::shared_ptr<Window>> Py_Window(module, "_Window");

        module.def("Window", []() -> std::shared_ptr<Window> {
            return Application::get_instance().get_current_window();
        }, "Reference to the current Window of the Application.");

        Py_Window.def("get_layout_root", &Window::get_layout_root, "The invisible root Layout of this Window.");
    }

    auto Py_Component = py::class_<Component, std::shared_ptr<Component>>(module, "_Component");



    // canvas component
    {
        py::class_<CanvasComponent, std::shared_ptr<CanvasComponent>> Py_CanvasComponent (module, "_CanvasComponent", Py_Component);

        module.def("CanvasComponent", []() -> std::shared_ptr<CanvasComponent> {
            return make_component<CanvasComponent>();
        }, "Creates a new CanvasComponent.");

        // TODO: it would be nice if we could inspect the method here and see if it fits the signature
        Py_CanvasComponent.def("set_paint_function", &CanvasComponent::set_paint_function, "Sets a new function for painting the canvas.", py::arg("paint_function"));
    }




    auto Py_LayoutItem = py::class_<LayoutItem, std::shared_ptr<LayoutItem>>(module, "_LayoutItem");

    {
        py::class_<Widget, std::shared_ptr<Widget>> Py_Widget(module, "_Widget", Py_LayoutItem);

        module.def("Widget", []() -> std::shared_ptr<Widget> {
            return Widget::create();
        }, "Creates a new Widget with an automatically assigned Handle.");

        module.def("Widget", [](const Handle handle) -> std::shared_ptr<Widget> {
            return Widget::create(handle);
        }, "Creates a new Widget with an explicitly assigned Handle.", py::arg("handle"));

        Py_Widget.def("get_handle", &Widget::get_handle, "The Application-unique Handle of this Widget.");
        Py_Widget.def("add_component", &Widget::add_component, "Attaches a new Component to this Widget.", py::arg("component"));

    }

    // layout root
    {
        py::class_<LayoutRoot, std::shared_ptr<LayoutRoot>> Py_LayoutRoot(module, "_LayoutRoot", Py_LayoutItem);

        Py_LayoutRoot.def("set_item", &LayoutRoot::set_item, "Sets a new Item at the LayoutRoot.", py::arg("item"));
    }

    // stack layout
    {
        py::class_<StackLayout, std::shared_ptr<StackLayout>> Py_StackLayout(module, "_StackLayout", Py_LayoutItem);

        module.def("StackLayout", [](const STACK_DIRECTION direction) -> std::shared_ptr<StackLayout> {
            return StackLayout::create(direction);
        }, "Creates a new StackLayout with an automatically assigned Handle.", py::arg("direction"));

        module.def("StackLayout", [](const STACK_DIRECTION direction, const Handle handle) -> std::shared_ptr<StackLayout> {
            return StackLayout::create(direction, handle);
        }, "Creates a new StackLayout with an explicitly assigned Handle.", py::arg("direction"), py::arg("handle"));

        Py_StackLayout.def("add_item", &StackLayout::add_item, "Adds a new LayoutItem into the Layout.", py::arg("item"));

    }

    // graphics

    // painter
    {
        py::class_<Painter> Py_Painter(module, "Painter");

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

        Py_Painter.def("set_stroke", &Painter::set_stroke, "Sets the current stroke style to a solid color.", py::arg("color"));
        Py_Painter.def("set_fill", &Painter::set_fill, "Sets the current fill style to a solid color.", py::arg("color"));
        Py_Painter.def("set_stroke_width", &Painter::set_stroke_width, "Sets the width of the stroke.", py::arg("width"));
        Py_Painter.def("set_line_cap", &Painter::set_line_cap, "Sets how the end of the line (cap) is drawn - default is LineCap::BUTT.", py::arg("cap"));
        Py_Painter.def("set_line_join", &Painter::set_line_join, "Sets how sharp path corners are drawn - default is LineJoin::MITER.", py::arg("join"));
        Py_Painter.def("set_miter_limit", &Painter::set_miter_limit, "Sets the miter limit of the stroke.", py::arg("limit"));

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

    return module.ptr();
}
