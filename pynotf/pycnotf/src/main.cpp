#include <pybind11/pybind11.h>

namespace py = pybind11;

void produce_aabr(pybind11::module& module);
void produce_color(pybind11::module& module);
void produce_squarebezierf(pybind11::module& module);
void produce_squarebezier2f(pybind11::module& module);
void produce_cubicbezierf(pybind11::module& module);
void produce_cubicbezier2f(pybind11::module& module);
void produce_matrix3f(pybind11::module& module);
void produce_nanovg(pybind11::module& module);
void produce_polygon2f(pybind11::module& module);
void produce_segment2f(pybind11::module& module);
void produce_size2f(pybind11::module& module);
void produce_size2i(pybind11::module& module);
void produce_trianglef(pybind11::module& module);
void produce_vector2(pybind11::module& module);

PYBIND11_MODULE(pycnotf, module) {
    produce_vector2(module);
    produce_aabr(module);
    produce_color(module);
    produce_squarebezierf(module);
    produce_squarebezier2f(module);
    produce_cubicbezierf(module);
    produce_cubicbezier2f(module);
    produce_matrix3f(module);
    produce_nanovg(module);
    produce_polygon2f(module);
    produce_segment2f(module);
    produce_size2f(module);
    produce_size2i(module);
    produce_trianglef(module);
}
