#pragma once

#include "pybind11/pybind11.h"
namespace py = pybind11;

namespace notf {

/** Returns the notf cache dict associated with a Python object.
 * If the cache does not yet exist, it is created.
 * @param host  The Python object containing the
 * @return The cache dict as a generic Python object.
 * @throw std::runtime_error, if the given object does not have a __dict__.
 */
py::dict get_notf_cache(py::object host);

/** Returns a dictionary with a given name from a Python dictionary.
 * Creates a new dict, if the key does not yet name an item.
 * @param dict  Dictionary containing the requested item.
 * @param key   Key in the dictionary for the requested item.
 * @return      The requested item as raw Python object.
 * @throw       std::runtime_error if the requested item is not a dict.
 */
py::dict get_dict(py::object dict, const char* key);

/** Returns a set with a given name from a Python dictionary.
 * Creates a new set, if the key does not yet name an item.
 * @param dict  Dictionary containing the requested item.
 * @param key   Key in the dictionary for the requested item.
 * @return      The requested item as raw Python object.
 * @throw       std::runtime_error if the requested item is not a set.
 */
py::set get_set(py::object dict, const char* key);

/** Returns a list with a given name from a Python dictionary.
 * Creates a new list, if the key does not yet name an item.
 * @param dict  Dictionary containing the requested item.
 * @param key   Key in the dictionary for the requested item.
 * @return      The requested item as raw Python object.
 * @throw       std::runtime_error if the requested item is not a list.
 */
py::list get_list(py::object dict, const char* key);

} // namespace notf
