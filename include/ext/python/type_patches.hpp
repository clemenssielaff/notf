#pragma once

#include "python/py_fwd.hpp"

namespace notf {

/**
 * If the Python instance is about to be destroyed, this function is called with a last chance of resurrecting it again.
 * We do that, because we need the instances of Python subtypes of the Item class to stick around for their state.
 * However, we only do it if, at the time of the Python object's destruction, the Item itself has become part of the
 * Item hierarchy and some other Item is owning it as well.
 * In that case, the Python instance is saved and ends up being owned by the Item, effectively reversing ownership.
 */
void finalize_py_item(PyObject* object);

/**
 * It seems as if Python does not want Python types that inherit from Item to have the `tp_finalize` field set without
 * also defining the `__del__` function and always sets it to 0 in the process of creating the type.
 * However, if we do define `__del__` some other function still overwrites the `tp_finalize` field not with null but
 * some other function.
 * In order to bypass all Python funkyness, we wait until the first time a new instance of our custom type is created,
 * and then monkeypatch the type object.
 * Afterwards, the type object never calls this function again and everything works just fine...
 */
PyObject* new_py_item(PyTypeObject* type, PyObject* args, PyObject* kwds);

/**
 * Convenience function for patching a given Python type.
 * The type must be a subclass of Python bindings for Item.
 */
void patch_type(PyObject* type_obj);

} // namespace notf
