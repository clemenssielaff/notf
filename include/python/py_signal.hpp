#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "common/log.hpp"
#include "common/signal.hpp"
#include "python/py_fwd.hpp"
using namespace notf;

template <typename... SIGNATURE>
class PySignal {

public: // type
    using Signature = std::tuple<SIGNATURE...>;

private: // struct
    struct Target {
        Target(const py::object& function)
            : function(PyWeakref_NewRef(function.ptr(), nullptr), py_decref)
            , test_function()
        {
        }

        Target(const py::object& function, const py::object& test_func)
            : function(PyWeakref_NewRef(function.ptr(), nullptr), py_decref)
            , test_function(PyWeakref_NewRef(test_func.ptr(), nullptr), py_decref)
        {
        }

        std::unique_ptr<PyObject, decltype(&py_decref)> function;
        std::unique_ptr<PyObject, decltype(&py_decref)> test_function;
    };

public: // methods
    PySignal(std::string name)
        : m_name(name)
        , m_targets()
    {
    }

    void connect(py::object host, py::object callback, py::object test_func = {})
    {
        { // store the callback into a cache in the object's __dict__ so it doesn't get lost
            int success = 0;
            const std::string cache_name = "__notf_signal_" + m_name;
            py::object dict(PyObject_GenericGetDict(host.ptr(), nullptr), false);
            py::object cache(PyDict_GetItemString(dict.ptr(), cache_name.c_str()), true);
            if (!cache.check()) {
                py::object new_cache(PySet_New(nullptr), false);
                success += PyDict_SetItemString(dict.ptr(), cache_name.c_str(), new_cache.ptr());
                assert(success == 0);
                cache = std::move(new_cache);
            }
            assert(cache.check());
            success += PySet_Add(cache.ptr(), callback.ptr());
            if (test_func.check()) {
                success += PySet_Add(cache.ptr(), test_func.ptr());
            }
            assert(success == 0);
        }

        if (test_func.check()) {
            m_targets.emplace_back(callback, test_func);
        }
        else {
            m_targets.emplace_back(callback);
        }
    }

    void fire(SIGNATURE... args)
    {
        auto arguments = std::make_tuple<SIGNATURE...>(std::move(args...));
        for (Target& target : m_targets) {

            // execute test function
            if (target.test_function) {
                py::function test_func(PyWeakref_GetObject(target.test_function.get()), /* borrowed = */ true);
                if (test_func.check()) {
                    py::object result = test_func(arguments);
                    if (PyObject_IsTrue(result.ptr()) != 1) {
                        continue;
                    }
                }
                else {
                    log_critical << "Invalid weakref of test function of in signal: \"" << m_name << "\"";
                }
            }

            // execute callback function
            py::function callback(PyWeakref_GetObject(target.function.get()), /* borrowed = */ true);
            if (callback.check()) {
                callback(arguments);
            }
            else {
                log_critical << "Invalid weakref of callback function of in signal: \"" << m_name << "\"";
            }
        }
        log_info << "SIGNAL FIRED"; // TODO: test only
    }

private: // fields
    std::string m_name;

    std::vector<Target> m_targets;
};
