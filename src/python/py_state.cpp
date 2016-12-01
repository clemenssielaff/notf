#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/state.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

void produce_state(pybind11::module& module)
{
    using StateStudy = StateMachineFactory::StateStudy;

    py::class_<StateMachineFactory> Py_StateMachineFactory(module, "StateMachineFactory");
    py::class_<StateStudy, std::shared_ptr<StateStudy>> Py_StateStudy(module, "StateStudy");
    py::class_<StateMachine, std::shared_ptr<StateMachine>> (module, "StateMachine");

    Py_StateMachineFactory.def(py::init<>());
    Py_StateMachineFactory.def("add_state", &StateMachineFactory::add_state, "Creates a new state with the given name.", py::arg("name"));
    Py_StateMachineFactory.def("get_state", &StateMachineFactory::get_state, "Returns an existing state by name, returns empty if the name is unknown.", py::arg("name"));
    Py_StateMachineFactory.def("remove_all_transitions_to", &StateMachineFactory::remove_all_transitions_to, "Removes all Transitions into a given state.", py::arg("state"));
    Py_StateMachineFactory.def("produce", &StateMachineFactory::produce, "Produces a valid StateMachine instance.", py::arg("start_state"));

    Py_StateStudy.def("transition_to", &StateStudy::transition_to, "Adds a Transition to another state.", py::arg("state"));
    Py_StateStudy.def("remove_transition_to", &StateStudy::remove_transition_to, "Removes the Transition to another state, if there is one.", py::arg("state"));
    Py_StateStudy.def("remove_all_transitions", &StateStudy::remove_all_transitions, "Removes all Transitions out of this state.");
    Py_StateStudy.def("remove_all_transitions", &StateStudy::remove_all_transitions, "Removes all Transitions out of this state.");
    Py_StateStudy.def("attach_component", &StateStudy::attach_component, "Attaches a new Component to this state, replaces an old Component of the same kind.", py::arg("component"));
    Py_StateStudy.def("remove_component", (void (StateStudy::*)(std::shared_ptr<Component>)) & StateStudy::remove_component, "Removes a certain Component from this state.", py::arg("component"));
    Py_StateStudy.def("remove_component", (void (StateStudy::*)(const Component::KIND)) & StateStudy::remove_component, "Removes the Component of a certain type from this state.", py::arg("kind"));
    Py_StateStudy.def("remove_all_components", &StateStudy::remove_all_components, "Removes all Components from this state.");
    Py_StateStudy.def("set_claim", &StateStudy::set_claim, "Sets the Claim of this state.", py::arg("claim"));
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
