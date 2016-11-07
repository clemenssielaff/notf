#pragma once

namespace pybind11 {
class module;
}

/** Adds bindings for the NoTF globals.
 * @param module    pybind11 module, modified by this factory.
 */
void produce_globals(pybind11::module& module);
