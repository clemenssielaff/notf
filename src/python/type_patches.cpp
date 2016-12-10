#include "python/type_patches.hpp"

#include "pybind11/pybind11.h"
namespace py = pybind11;

#define NOTF_BINDINGS
#include "core/item.hpp"

namespace notf {

/** Access class to the otherwise protected `new_instance` function. */
class GenericPyItem : public py::detail::generic_type {
public:
    using py::detail::generic_type::new_instance;
};

void finalize_py_item(PyObject* object)
{
    auto instance = reinterpret_cast<py::detail::instance<Item, std::shared_ptr<Item>>*>(object);
    if (instance->holder.use_count() > 1) {
        instance->value->_set_pyobject(object);
        assert(object->ob_refcnt == 2); // one from us, one from PyObject_CallFinalizerFromDealloc
        instance->holder.reset();
    }
}

PyObject* new_py_item(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
    assert(type->tp_base->tp_finalize == finalize_py_item);
    type->tp_finalize = finalize_py_item;
    type->tp_new = GenericPyItem::new_instance; // all other instances call the correct function directly
    return GenericPyItem::new_instance(type, args, kwds);
}

void patch_type(PyObject* type_obj)
{
    PyTypeObject* type = reinterpret_cast<PyTypeObject*>(type_obj);
    type->tp_new = new_py_item;
    type->tp_finalize = finalize_py_item;
    type->tp_flags |= Py_TPFLAGS_HAVE_FINALIZE;
}

} // namespace notf
