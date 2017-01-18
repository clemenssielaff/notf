#include "python/py_dict_utils.hpp"

#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "common/log.hpp"
#include "common/string_utils.hpp"
#include "python/py_fwd.hpp"

namespace notf {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

py::dict get_notf_cache(py::object host)
{
    static const char* cache_name = "__notf_cache";
    int success = 0;
    py::object dict(PyObject_GenericGetDict(host.ptr(), nullptr), false);
    if(!dict.check()){
        PyErr_Print();
        std::string msg = string_format("Python object has no __dict__: %s", py_print(host.ptr()));
        throw std::runtime_error(msg);
    }
    py::object notf_cache(PyDict_GetItemString(dict.ptr(), cache_name), true);
    if (!notf_cache.check()) {
        py::object new_cache(PyDict_New(), false);
        success += PyDict_SetItemString(dict.ptr(), cache_name, new_cache.ptr());
        assert(success == 0);
        notf_cache = std::move(new_cache);
    }
    assert(notf_cache.check());
    assert(PyDict_Check(notf_cache.ptr()));
    return notf_cache;
}

pybind11::dict get_dict(py::object dict, const char* key)
{
    py::object item(PyDict_GetItemString(dict.ptr(), key), true);
    if (!item.check()) {
        py::object new_item(PyDict_New(), false);
        int success = PyDict_SetItemString(dict.ptr(), key, new_item.ptr());
        assert(success == 0);
        item = std::move(new_item);
    }
    else if (!PyDict_Check(item.ptr())) {
        std::string msg = string_format("Requested notf cache item `%s` is not a dict", key);
        log_critical << msg;
        throw std::runtime_error(msg);
    }
    assert(item.check());
    assert(PyDict_Check(item.ptr()));
    return item;
}

py::set get_set(py::object dict, const char* key)
{
    py::object item(PyDict_GetItemString(dict.ptr(), key), true);
    if (!item.check()) {
        py::object new_item(PySet_New(nullptr), false);
        int success = PyDict_SetItemString(dict.ptr(), key, new_item.ptr());
        assert(success == 0);
        item = std::move(new_item);
    }
    else if (!PySet_Check(item.ptr())) {
        std::string msg = string_format("Requested notf cache item `%s` is not a set", key);
        log_critical << msg;
        throw std::runtime_error(msg);
    }
    assert(item.check());
    assert(PySet_Check(item.ptr()));
    return item;
}

py::list get_list(py::object dict, const char* key)
{
    py::object item(PyDict_GetItemString(dict.ptr(), key), true);
    if (!item.check()) {
        py::object new_item(PyList_New(0), false);
        int success = PyDict_SetItemString(dict.ptr(), key, new_item.ptr());
        assert(success == 0);
        item = std::move(new_item);
    }
    else if (!PyList_Check(item.ptr())) {
        std::string msg = string_format("Requested notf cache item `%s` is not a list", key);
        log_critical << msg;
        throw std::runtime_error(msg);
    }
    assert(item.check());
    assert(PyList_Check(item.ptr()));
    return item;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

} // namespace notf
