#pragma once

#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "common/log.hpp"
#include "common/signal.hpp"
#include "common/warnings.hpp"
#include "ext/python/py_dict_utils.hpp"
#include "ext/python/py_fwd.hpp"
#include "utils/apply_tuple.hpp"

namespace notf {

namespace detail {
extern const char* s_signal_cache_name;
}

/** Python implementation of the notf Signal object.
 * All specializations of this class must be explicitly created in `py_signal.cpp` using the `define_pysignal` macro.
 */
template <typename... SIGNATURE>
class PySignal {

public: // type
    using Signature = std::tuple<SIGNATURE...>;

private: // struct
    struct Target {
        /** Constructor without optional test function. */
        Target(ConnectionID id, const py::object& function, bool is_enabled = true)
            : id(id)
            , callback(PyWeakref_NewRef(function.ptr(), nullptr), py_decref)
            , test(nullptr, py_decref)
            , is_enabled(is_enabled)
        {
        }

        /** Constructor with test function. */
        Target(ConnectionID id, const py::object& function, const py::object& test_func, bool is_enabled = true)
            : id(id)
            , callback(PyWeakref_NewRef(function.ptr(), nullptr), py_decref)
            , test(PyWeakref_NewRef(test_func.ptr(), nullptr), py_decref)
            , is_enabled(is_enabled)
        {
        }

        /** ID of the Connection, is from the same pool as notf::detail::Connection IDs. */
        ConnectionID id;

        /** Weakref to the Python callback function for this Target. */
        std::unique_ptr<PyObject, decltype(&py_decref)> callback;

        /** Weakref to the Python test function for this Target. */
        std::unique_ptr<PyObject, decltype(&py_decref)> test;

        /** Is the Target currently enabled? */
        bool is_enabled;
    };

public: // methods
    /**
     * @param host  Python host object, providing the cache for storing the signal weakref targets.
     * @param name  Name of this Signal, used to identify its cache.
     */
    PySignal(py::object host, std::string name)
        : m_host(PyWeakref_NewRef(host.ptr(), nullptr), py_decref)
        , m_name(name)
        , m_targets()
    {
    }

    /** Connects a new target to this Signal.
     * @param callback  Callback function that is excuted when this Signal is triggered.
     * @param test      (optional) Test function, the `callback` is only executed if this function returns true.
     * @throw std::runtime_error If PyController messed up to contruct this PySignal (should never happen...)
     */
    ConnectionID connect(py::object callback, py::function test)
    {
        py::object host(PyWeakref_GetObject(m_host.get()), /* borrowed = */ true);
        if (!host.check()) {
            std::string msg = "Invalid weakref of host in signal: \"" + m_name + "\"";
            log_critical << msg;
            throw std::runtime_error(msg);
        }

        { // store the callback and test into a cache in the object's __dict__ so they don't get lost
            py::dict notf_cache        = get_notf_cache(host);
            py::dict signal_cache_dict = get_dict(notf_cache, detail::s_signal_cache_name);
            py::list cache             = get_list(signal_cache_dict, m_name.c_str());
            py::object handler;
            if (test.check()) {
                handler = py::object(PyTuple_Pack(2, callback.ptr(), test.ptr()), /* borrowed = */ false);
            }
            else {
                handler = py::object(PyTuple_Pack(1, callback.ptr()), /* borrowed = */ false);
            }

            // if the handler is an exact copy of one that is already known, return that one instead
            for (size_t i = 0; i < cache.size(); ++i) {
                int result = PyObject_RichCompareBool(
                    handler.ptr(), PyList_GetItem(cache.ptr(), static_cast<Py_ssize_t>(i)), Py_EQ);
                assert(result != -1);
                if (result == 1) {
                    assert(i < m_targets.size());
                    return m_targets[i].id;
                }
            }

            int success = PyList_Append(cache.ptr(), handler.ptr());
            assert(success == 0);
            UNUSED(success);
        }

        // create the new target
        ConnectionID id = detail::Connection::_get_next_id();
        if (test.check()) {
            m_targets.emplace_back(id, callback, test);
        }
        else {
            m_targets.emplace_back(id, callback);
        }
        return id;
    }

    /** Triggers the Signal to call all of its targets.
     * @throw std::runtime_error    (one per target) In case the call fails,
     */
    void fire(SIGNATURE... args)
    {
        // we need to filter for all enabled targets before executing the callbacks
        // since they might dis/enable other targets
        std::vector<Target*> enabled_targets;
        for (Target& target : m_targets) {
            if (target.is_enabled) {
                enabled_targets.push_back(&target);
            }
        }

        auto arguments = std::make_tuple<SIGNATURE...>(std::forward<SIGNATURE>(args)...);

        for (Target* target : enabled_targets) {

            // execute test function
            if (target->test) {
                py::function test_func(PyWeakref_GetObject(target->test.get()), /* borrowed = */ true);
                if (test_func.check()) {
                    try {
                        py::object result = apply(test_func, arguments);
                        if (PyObject_IsTrue(result.ptr()) != 1) {
                            continue;
                        }
                    } catch (std::runtime_error error) {
                        log_warning << error.what();
                        continue;
                    }
                }
                else {
                    log_critical << "Invalid weakref of test function in signal: \"" << m_name << "\"";
                }
            }

            // execute callback function
            py::function callback(PyWeakref_GetObject(target->callback.get()), /* borrowed = */ true);
            if (callback.check()) {
                try {
                    apply(callback, arguments);
                } catch (std::runtime_error error) {
                    log_warning << error.what();
                }
            }
            else {
                log_critical << "Invalid weakref of callback function in signal: \"" << m_name << "\"";
            }
        }
    }

    /** Operator overload for `fire`. */
    void operator()(SIGNATURE... args)
    {
        fire(std::forward<SIGNATURE>(args)...);
    }

    /** Checks if a particular Connection is connected to this Signal. */
    bool has_connection(const ConnectionID id) const
    {
        if (!id) {
            return false;
        }
        for (const Target& target : m_targets) {
            if (target.id == id) {
                return true;
            }
        }
        return false;
    }

    /** Returns all (connected) Connections.*/
    std::vector<ConnectionID> get_connections() const
    {
        std::vector<ConnectionID> result;
        result.reserve(m_targets.size());
        for (const Target& target : m_targets) {
            result.push_back(target.id);
        }
        return result;
    }

    /** Temporarily disables all Connections of this Signal. */
    void disable()
    {
        for (Target& target : m_targets) {
            target.is_enabled = false;
        }
    }

    /** Disables a specific Connection of this Signal.
     * @param connection            ID of the Connection.
     * @throw std:runtime_error     If there is no Connection with the given ID connected to this Signal.
     */
    void disable(const ConnectionID id)
    {
        for (Target& target : m_targets) {
            if (id == target.id) {
                target.is_enabled = false;
                return;
            }
        }
        throw std::runtime_error("Cannot disable unknown connection");
    }

    /** (Re-)Enables all Connections of this Signal. */
    void enable()
    {
        for (Target& target : m_targets) {
            target.is_enabled = true;
        }
    }

    /** Enables a specific Connection of this Signal.
     * @param connection            ID of the Connection.
     * @throw std:runtime_error     If there is no Connection with the given ID connected to this Signal.
     */
    void enable(const ConnectionID id)
    {
        for (Target& target : m_targets) {
            if (id == target.id) {
                target.is_enabled = true;
                return;
            }
        }
        throw std::runtime_error("Cannot enable unknown connection");
    }

    /** Disconnect all Connections from this Signal. */
    void disconnect()
    {
        m_targets.clear();

        // clear the cache as well
        py::object host(PyWeakref_GetObject(m_host.get()), /* borrowed = */ true);
        py::dict notf_cache        = get_notf_cache(host);
        py::dict signal_cache_dict = get_dict(notf_cache, detail::s_signal_cache_name);
        py::list cache             = get_list(signal_cache_dict, m_name.c_str());
        PySequence_DelSlice(cache.ptr(), 0, PySequence_Length(cache.ptr()));
        assert(PySequence_Length(cache.ptr()) == 0);
    }

    /** Disconnects a specific Connection of this Signal.
     * @param connection            ID of the Connection.
     * @throw std:runtime_error     If there is no Connection with the given ID connected to this Signal.
     */
    void disconnect(const ConnectionID id)
    {
        using std::swap;
        bool success           = false;
        Py_ssize_t cache_index = 0;
        for (Target& target : m_targets) {
            if (id == target.id) {
                swap(target, m_targets.back());
                success = true;
            }
            else {
                ++cache_index;
            }
        }
        if (!success) {
            throw std::runtime_error("Cannot disconnect unknown connection");
        }
        m_targets.pop_back();

        // delete the target from the cache as well
        py::object host(PyWeakref_GetObject(m_host.get()), /* borrowed = */ true);
        py::dict notf_cache        = get_notf_cache(host);
        py::dict signal_cache_dict = get_dict(notf_cache, detail::s_signal_cache_name);
        py::list cache             = get_list(signal_cache_dict, m_name.c_str());
        assert(cache_index < PyList_Size(cache.ptr()));
        PySequence_DelSlice(cache.ptr(), cache_index, cache_index + 1);
    }

    /** Restores the targets after the Python object has been finalized an all weakrefs have been destroyed.
     * @param host  Python object providing the cache.
     */
    void restore(py::object host)
    {
        using std::swap;

        // restore the host
        m_host.reset(PyWeakref_NewRef(host.ptr(), nullptr));

        // get the host's signal cache
        py::dict notf_cache        = get_notf_cache(host);
        py::dict signal_cache_dict = get_dict(notf_cache, detail::s_signal_cache_name);
        py::list cache             = get_list(signal_cache_dict, m_name.c_str());
        assert(cache.size() == m_targets.size());

        // ... and use it to restore the targets
        std::vector<Target> new_targets;
        new_targets.reserve(m_targets.size());
        for (size_t i = 0; i < m_targets.size(); ++i) {
            const Target& target = m_targets[i];
            py::tuple handlers   = py::object(cache[i]);
            assert(handlers.check());
            if (handlers.size() == 1) {
                new_targets.emplace_back(target.id, py::object(handlers[0]), target.is_enabled);
            }
            else {
                assert(handlers.size() == 2);
                new_targets.emplace_back(target.id, py::object(handlers[0]), py::object(handlers[1]), target.is_enabled);
            }
        }
        std::swap(m_targets, new_targets);
    }

private: // fields
    /** Weakref to the host providing the cache for the target functions (see `connect` for details). */
    std::unique_ptr<PyObject, decltype(&py_decref)> m_host;

    /** Name of this signal, used to identify its field in the cache. */
    std::string m_name;

    /** All targets of this Signal. */
    std::vector<Target> m_targets;
};

} // namespace notf
