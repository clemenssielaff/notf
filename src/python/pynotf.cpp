#include "python/pynotf.hpp"

#include <exception>

#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "common/string_utils.hpp"
#include "common/vector2.hpp"
#include "core/application.hpp"
#include "core/components/texture_component.hpp"
#include "core/layout_root.hpp"
#include "core/resource_manager.hpp"
#include "core/widget.hpp"
#include "core/window.hpp"
#include "dynamic/layout/stack_layout.hpp"
#include "dynamic/render/sprite.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture2.hpp"
#include "utils/enum_to_number.hpp"
using namespace notf;

const char* python_notf_module_name = "notf";

/**********************************************************************************************************************/

/** Python wrapper for notf::Shader. */
struct PyShader {

    std::shared_ptr<Shader> data;

    explicit PyShader(const std::string& name)
        : data(Application::get_instance().get_resource_manager().get_shader(name))
    {
        if (!data) {
            throw std::runtime_error(string_format("Cannot load unknown Shader named '%s'", name));
        }
    }

    explicit PyShader(const std::string& shader_name,
                      const std::string& vertex_shader_path,
                      const std::string& fragment_shader_path)
        : data(Application::get_instance().get_resource_manager().build_shader(
              shader_name, vertex_shader_path, fragment_shader_path))
    {
        if (!data) {
            throw std::runtime_error(string_format("Failed to build Shader '%s' from the folloing sources:\n"
                                                   "\tvertex shader:   '%s'\n"
                                                   "\tfragment shader: '%s'",
                                                   shader_name, vertex_shader_path, fragment_shader_path));
        }
    }
};

/**********************************************************************************************************************/

/** Python wrapper for notf::Texture2. */
struct PyTexture {

    std::shared_ptr<Texture2> data;

    explicit PyTexture(const std::string& texture_path)
        : data(Application::get_instance().get_resource_manager().get_texture(texture_path))
    {
        if (!data) {
            throw std::runtime_error(string_format("Cannot load unknown Texture '%s'", texture_path));
        }
    }
};

/**********************************************************************************************************************/

/** Python wrapper for notf::TextureComponent. */
struct PyTextureComponent {

    std::shared_ptr<TextureComponent> data;

    explicit PyTextureComponent(const PyTexture& texture)
        : data(make_component<TextureComponent>(TextureChannels{{0, texture.data}}))
    {
        if (!data) {
            throw std::runtime_error(string_format("Failed to produce valid TextureComponent from Texture ID %u",
                                                   texture.data->get_id()));
        }
    }
};

/**********************************************************************************************************************/

/** Python wrapper for notf::SpriteRenderer. */
struct PySpriteRenderer {

    std::shared_ptr<SpriteRenderer> data;

    explicit PySpriteRenderer(const PyShader& shader)
        : data(make_component<SpriteRenderer>(shader.data))
    {
        if (!data) {
            throw std::runtime_error(string_format("Failed to produce valid SpriteRenderer from Shader '%s'",
                                                   shader.data->get_name()));
        }
    }
};

/**********************************************************************************************************************/

/** Python wrapper for notf::Widget. */
struct PyWidget {

    std::shared_ptr<Widget> data;

    explicit PyWidget(const Handle handle)
        : data(Widget::create(handle))
    {
        if (!data) {
            std::string message;
            if (handle) {
                message = string_format("Failed to create Widget with requested Handle %u", handle);
            }
            else {
                message = "Failed to allocate new Handle for Widget";
            }
            throw std::runtime_error(message);
        }
    }

    explicit PyWidget()
        : PyWidget(BAD_HANDLE)
    {
    }

    Handle get_handle() const { return data->get_handle(); }

    void add_component(const PySpriteRenderer& component) { data->add_component(component.data); }
    void add_component(const PyTextureComponent& component) { data->add_component(component.data); }
};

/**********************************************************************************************************************/

/** Python wrapper for notf::StackLayout. */
struct PyStackLayout {

    std::shared_ptr<StackLayout> data;

    explicit PyStackLayout(unsigned char direction)
        : data()
    {
        if (direction > to_number(StackLayout::DIRECTION::BOTTOM_TO_TOP)) {
            throw std::runtime_error(string_format("Invalid StackLayout direction: %u. Valid values are:\n"
                                                   "\t0 : Left->Right\n",
                                                   "\t1 : Top->Bottom\n",
                                                   "\t2 : Right->Left\n",
                                                   "\t3 : Bottom->Top",
                                                   direction));
        }
        data = StackLayout::create(static_cast<StackLayout::DIRECTION>(direction));
        if (!data) {
            throw std::runtime_error(string_format("Failed to create StackLayout with direction: %u", direction));
        }
    }

    void add_item(const PyWidget& widget) { data->add_item(widget.data); }
    void add_item(const PyStackLayout& layout) { data->add_item(layout.data); }
};

/**********************************************************************************************************************/

struct PyLayoutRoot {

    friend struct PyWindow;

    std::shared_ptr<LayoutRoot> data;

    void set_item(const PyStackLayout& layout) { data->set_item(layout.data); }
    void set_item(const PyWidget& widget) { data->set_item(widget.data); }

private:
    PyLayoutRoot(std::shared_ptr<LayoutRoot> layout_root)
        : data(layout_root)
    {
    }
};

/**********************************************************************************************************************/

/** Python wrapper for notf::StackLayout. */
struct PyWindow {

    std::shared_ptr<Window> data;

    explicit PyWindow()
        : data(Application::get_instance().get_current_window())
    {
    }

    PyLayoutRoot get_layout_root() { return PyLayoutRoot(data->get_layout_root()); }
};

/**********************************************************************************************************************/

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

PyObject* produce_pynotf_module()
{
    py::module module(python_notf_module_name, "NoTF Python bindings");

    // Shader
    {
        py::class_<PyShader>(module, "Shader")
            .def(py::init<const std::string&>())
            .def(py::init<const std::string&, const std::string&, const std::string&>());
    }

    // Texture2
    {
        py::class_<PyTexture>(module, "Texture2")
            .def(py::init<const std::string&>());
    }

    // TextureComponent
    {
        py::class_<PyTextureComponent>(module, "TextureComponent")
            .def(py::init<const PyTexture&>());
    }

    // SpriteRenderer
    {
        py::class_<PySpriteRenderer>(module, "SpriteRenderer")
            .def(py::init<const PyShader&>());
    }

    // Widget
    {
        py::class_<PyWidget>(module, "Widget")
            .def(py::init<>())
            .def(py::init<Handle>())
            .def("get_handle", &PyWidget::get_handle, "The Application-unique Handle of this Widget.")
            .def("add_component", (void (PyWidget::*)(const PySpriteRenderer&)) & PyWidget::add_component)
            .def("add_component", (void (PyWidget::*)(const PyTextureComponent&)) & PyWidget::add_component);
    }

    // StackLayout
    {
        py::class_<PyStackLayout>(module, "StackLayout")
            .def(py::init<unsigned char>())
            .def("add_item", (void (PyStackLayout::*)(const PyWidget&)) & PyStackLayout::add_item)
            .def("add_item", (void (PyStackLayout::*)(const PyStackLayout&)) & PyStackLayout::add_item);
    }

    // LayoutRoot
    {
        py::class_<PyLayoutRoot>(module, "LayoutRoot")
            .def("set_item", (void (PyLayoutRoot::*)(const PyWidget&)) & PyLayoutRoot::set_item)
            .def("set_item", (void (PyLayoutRoot::*)(const PyStackLayout&)) & PyLayoutRoot::set_item);
    }

    // Window
    {
        py::class_<PyWindow>(module, "Window")
            .def(py::init<>())
            .def("get_layout_root", &PyWindow::get_layout_root);
    }

    // Vector2
    {
        py::class_<Vector2>(module, "Vector2")

            // constructors
            .def(py::init<>())
            .def(py::init<Real, Real>())
            .def(py::init<Vector2>())

            // static constructors
            .def_static("fill", &Vector2::fill, "Returns a Vector2 with both components set to the given value", py::arg("value"))
            .def_static("x_axis", &Vector2::x_axis, "Returns an unit Vector2 along the x-axis.")
            .def_static("y_axis", &Vector2::y_axis, "Returns an unit Vector2 along the y-axis.")

            // properties
            .def_readwrite("x", &Vector2::x)
            .def_readwrite("y", &Vector2::y)

            // inspections
            .def("is_zero", (bool (Vector2::*)() const) & Vector2::is_zero, "Checks if this Vector2 is the zero vector.")
            .def("is_zero", (bool (Vector2::*)(Real) const) & Vector2::is_zero, "Checks if this Vector2 is the zero vector.", py::arg("epsilon"))
            .def("is_unit", &Vector2::is_unit, "Checks whether this Vector2 is of unit magnitude.")
            .def("is_parallel_to", &Vector2::is_parallel_to, "Checks whether this Vector2 is parallel to other.", py::arg("other"))
            .def("is_orthogonal_to", &Vector2::is_orthogonal_to, "Checks whether this Vector2 is orthogonal to other.", py::arg("other"))
            .def("angle", &Vector2::angle, "The angle in radians between the positive x-axis and the point given by this Vector2.")
            .def("angle_to", &Vector2::angle_to, "Calculates the smallest angle between two Vector2s in radians.", py::arg("other"))
            .def("direction_to", &Vector2::direction_to, "Tests if the other Vector2 is collinear (1) to this, opposite (-1) or something in between.", py::arg("other"))
            .def("is_horizontal", &Vector2::is_horizontal, "Tests if this Vector2 is parallel to the x-axis.")
            .def("is_vertical", &Vector2::is_vertical, "Tests if this Vector2 is parallel to the y-axis.")
            .def("is_approx", &Vector2::is_approx, "Returns True, if other and self are approximately the same Vector2.", py::arg("other"))
            .def("is_not_approx", &Vector2::is_not_approx, "Returns True, if other and self are NOT approximately the same Vector2.", py::arg("other"))
            .def("slope", &Vector2::slope, "Returns the slope of this Vector2.")
            .def("magnitude_sq", &Vector2::magnitude_sq, "Returns the squared magnitude of this Vector2.")
            .def("magnitude", &Vector2::magnitude, "Returns the magnitude of this Vector2.")
            .def("is_real", &Vector2::is_real, "Checks, if this Vector2 contains only real values.")
            .def("contains_zero", &Vector2::contains_zero, "Checks, if any component of this Vector2 is a zero.")

            // operators
            .def(py::self == py::self)
            .def(py::self != py::self)
            .def(py::self + py::self)
            .def(py::self += py::self)
            .def(py::self - py::self)
            .def(py::self -= py::self)
            .def(py::self * py::self)
            .def(py::self *= py::self)
            .def(py::self / py::self)
            .def(py::self /= py::self)
            .def(py::self * Real())
            .def(py::self *= Real())
            .def(py::self / Real())
            .def(py::self /= Real())
            .def(-py::self)

            // modifiers
            .def("set_zero", &Vector2::set_zero, "Sets all components of the Vector to zero.")
            .def("inverted", &Vector2::inverted, "Returns an inverted copy of this Vector2.")
            .def("invert", &Vector2::invert, "Inverts this Vector2 in-place.")
            .def("dot", &Vector2::dot, "Vector2 dot product.", py::arg("other"))
            .def("normalized", &Vector2::normalized, "Returns a normalized copy of this Vector2.")
            .def("normalize", &Vector2::normalize, "Inverts this Vector2 in-place.")
            .def("projected_on", &Vector2::projected_on, "Creates a projection of this Vector2 onto an infinite line whose direction is specified by other.", py::arg("other"))
            .def("project_on", &Vector2::project_on, "Projects this Vector2 onto an infinite line whose direction is specified by other.", py::arg("other"))
            .def("orthogonal", &Vector2::orthogonal, "Creates an orthogonal 2D Vector to this one by rotating it 90 degree counter-clockwise.")
            .def("orthogonalize", &Vector2::orthogonalize, "In-place rotation of this Vector2 90 degrees counter-clockwise.")
            .def("rotated", &Vector2::rotated, "Returns a copy of this 2D Vector rotated counter-clockwise around its origin by a given angle in radians.", py::arg("angle"))
            .def("rotate", &Vector2::rotate, "Rotates this Vector2 counter-clockwise in-place around its origin by a given angle in radians.", py::arg("angle"))
            .def("side_of", &Vector2::side_of, "The side of the other 2D Vector relative to direction of this 2D Vector (+1 = left, -1 = right).", py::arg("other"))

            // representation
            .def("__repr__", [](const Vector2& vec) { return string_format("notf.Vector2(%f, %f)", vec.x, vec.y); });

        // free functions
        module.def("orthonormal_basis", (void (*)(Vector2&, Vector2&)) & orthonormal_basis, "Constructs a duo of mutually orthogonal, normalized vectors with 'a' as the reference vector", py::arg("a"), py::arg("b"));
        module.def("lerp", (Vector2(*)(const Vector2&, const Vector2&, Real)) & lerp, "Linear interpolation between two Vector2s.", py::arg("from"), py::arg("to"), py::arg("blend"));
        module.def("nlerp", (Vector2(*)(const Vector2&, const Vector2&, Real)) & nlerp, "Normalized linear interpolation between two Vector2s.", py::arg("from"), py::arg("to"), py::arg("blend"));
    }

    //    // Circle
    //    {
    //        py::class_<Circle>(module, "Circle")

    //            // constructors
    //            .def(py::init<>())
    //            .def(py::init<Vector2, Real>())
    //            .def(py::init<Circle>())

    //            // properties
    //            .def_readwrite("position", &Circle::position)
    //            .def_readwrite("radius", &Circle::radius)

    //            // inspections
    //            .def("distance", &Circle::distance, "The distance between this Circle and another.", py::arg("other"))
    //            .def("intersects", &Circle::intersects, "Checks if two circles are intersecting.", py::arg("other"))

    //            // representation
    //            .def("__repr__", [](const Circle& circle) { return formatString("notf.Circle((%f, %f), %f)", circle.position.x, circle.position.y, circle.radius); });
    //    }

    //    // Line2
    //    {
    //        py::class_<Line2>(module, "Line2")

    //            // constructors
    //            .def(py::init<>())
    //            .def(py::init<Vector2, Vector2>())
    //            .def(py::init<Line2>())

    //            // properties
    //            .def_property("start", &Line2::start, &Line2::set_start)
    //            .def_property("end", &Line2::end, &Line2::set_end)
    //            .def_property_readonly("delta", &Line2::delta)
    //            .def_property_readonly("length", &Line2::length)
    //            .def_property_readonly("length_sq", &Line2::length_sq)
    //            .def_property_readonly("slope", &Line2::slope)

    //            // inspections
    //            .def("is_zero", (bool (Line2::*)() const) & Line2::is_zero, "Checks if this line has zero length.")
    //            .def("is_zero", (bool (Line2::*)(Real) const) & Line2::is_zero, "Checks if this line has zero length.", py::arg("epsilon"))
    //            .def("is_unit", &Line2::is_unit, "Checks if the line has a unit length.")
    //            .def("is_horizontal", &Line2::is_horizontal, "Checks if this Line2 is parallel to the x-axis.")
    //            .def("is_vertical", &Line2::is_vertical, "Checks if this Line2 is parallel to the y-axis.")
    //            .def("is_parallel", (bool (Line2::*)(const Line2&) const) & Line2::is_parallel, "Checks whether this Line2 is parallel to another.", py::arg("other"))
    //            .def("is_parallel", (bool (Line2::*)(const Vector2&) const) & Line2::is_parallel, "Checks whether this Line2 is parallel to a given Vector2.", py::arg("other"))
    //            .def("is_orthogonal", (bool (Line2::*)(const Line2&) const) & Line2::is_orthogonal, "Checks whether this Line2 is orthogonal to another.", py::arg("other"))
    //            .def("is_orthogonal", (bool (Line2::*)(const Vector2&) const) & Line2::is_orthogonal, "Checks whether this Line2 is orthogonal to a given Vector2.", py::arg("other"))
    //            .def("bounding_rect", &Line2::bounding_rect, "Returns the axis-aligned bounding rect of this Line.")

    //            // representation
    //            .def("__repr__", [](const Line2& line) { return formatString("notf.Line2((%f, %f), (%f, %f))", line.start().x, line.start().y, line.end().x, line.end().y); });
    //    }

    //    // Aabr
    //    {
    //        py::class_<Aabr>(module, "Aabr")

    //            // constructors
    //            .def(py::init<>())
    //            .def(py::init<Vector2, Real, Real>())
    //            .def(py::init<Real, Real>())
    //            .def(py::init<Vector2, Vector2>())
    //            .def(py::init<Aabr>())

    //            // properties
    //            .def_property("x", &Aabr::x, &Aabr::set_x)
    //            .def_property("y", &Aabr::y, &Aabr::set_y)
    //            .def_property("position", &Aabr::position, &Aabr::set_position)

    //            .def_property("left", &Aabr::left, &Aabr::set_left)
    //            .def_property("right", &Aabr::right, &Aabr::set_right)
    //            .def_property("top", &Aabr::top, &Aabr::set_top)
    //            .def_property("bottom", &Aabr::bottom, &Aabr::set_bottom)
    //            .def_property("top_left", &Aabr::top_left, &Aabr::set_top_left)
    //            .def_property("top_right", &Aabr::top_right, &Aabr::set_top_right)
    //            .def_property("bottom_left", &Aabr::bottom_left, &Aabr::set_bottom_left)
    //            .def_property("bottom_right", &Aabr::bottom_right, &Aabr::set_bottom_right)

    //            .def_property("width", &Aabr::width, &Aabr::set_width)
    //            .def_property("height", &Aabr::height, &Aabr::set_height)

    //            .def_property_readonly("area", &Aabr::area)

    //            // inspections
    //            .def("is_null", &Aabr::is_null, "Test, if this rectangle is the null rectangle.")
    //            .def("contains", &Aabr::contains, "Checks if this rectangle contains a given point.", py::arg("point"))
    //            .def("intersects", &Aabr::intersects, "Checks if two rectangles intersect.", py::arg("other"))

    //            // modifiers
    //            .def("grow", &Aabr::grow, "Moves each edge of the rectangle a given amount towards the outside.", py::arg("amount"))
    //            .def("shrink", &Aabr::shrink, "Moves each edge of the rectangle a given amount towards the inside.", py::arg("amount"))
    //            .def("intersection", &Aabr::intersection, "Intersection of this Aabr with other.", py::arg("other"))
    //            .def("union", &Aabr::union_, "The union of this Aabr with other.", py::arg("other"))

    //            // representation
    //            .def("__repr__", [](const Aabr& aabr) { return formatString("notf.Aabr((%f, %f), (%f, %f))",
    //                                                        aabr.bottom_left().x, aabr.bottom_left().y,
    //                                                        aabr.top_right().x, aabr.top_right().y); });
    //    }

    return module.ptr();
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
