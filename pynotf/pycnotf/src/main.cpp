#include <pybind11/pybind11.h>

namespace py = pybind11;

void produce_aabr(pybind11::module& module);
void produce_cubicbezierf(pybind11::module& module);
void produce_cubicbezier2f(pybind11::module& module);
void produce_size2f(pybind11::module& module);
void produce_size2i(pybind11::module& module);
void produce_vector2(pybind11::module& module);

PYBIND11_MODULE(pycnotf, module) {
    produce_aabr(module);
    produce_cubicbezierf(module);
    produce_cubicbezier2f(module);
    produce_size2f(module);
    produce_size2i(module);
    produce_vector2(module);
}
