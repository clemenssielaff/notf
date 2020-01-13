from typing import Optional, Dict, List

from pynotf.callback import Callback as CallbackBase


#######################################################################################################################

class StateMachineCallback(CallbackBase):
    """
    Callbacks called when a State is entered/exited.
    """
    def __init__(self, source: str):
        CallbackBase.__init__(self, dict(widget=Widget.Self), source)


class State:
    """
    A State consists of nothing more than two Callbacks: a mandatory one that is called when the State is entered, and
    an optional second one that is called when the State is exited.
    """
    def __init__(self, name: str, entry_func: StateMachineCallback, exit_func: Optional[StateMachineCallback] = None):
        self._name: str = name  # is constant
        self._entry: StateMachineCallback = entry_func
        self._exit: Optional[StateMachineCallback] = exit_func

    def get_name(self) -> str:
        """
        The StateMachine-unique name of this State.
        """
        return self._name

    def enter(self) -> None:
        """
        Enter this State.
        """
        self._entry()

    def exit(self) -> None:
        """
        Exit this State.
        """
        if self._exit is not None:
            self._exit()


class StateMachineDescription:
    # TODO: replace StateMachineDescription with a single NamedTuple that is validated by the StateMachine constructor
    #       like a Widget.Type and .Definition

    class ConsistencyError(Exception):
        pass

    def __init__(self, initial_state: State):
        """
        Every State Machine must at least have one initial State.
        :param initial_state: The initial State.
        """
        self._states: Dict[str, State] = {initial_state.get_name(): initial_state}  # all states by name (>= 1 entries)
        self._transitions: Dict[str, List[str]] = {}  # reachable states by source state (entries <= len(states))
        self._initial_state: str = initial_state.get_name()  # name of the initial state, must be present in `states`

    def add_state(self, state: State, accessible_from: str) -> None:
        """
        Adds a new State to the State Machine.
        Every new State requires at least one Transition into it.
        :param state: New State.
        :param accessible_from: A State from which the new State can be reached.
        :raise ConsistencyError: If a State with the same name already exist, or a the `accessible_from` State does not.
        """
        # store the new state
        if state.get_name() in self._states:
            raise self.ConsistencyError(f'The State Machine already contains a State named "{state.get_name()}"')
        else:
            self._states[state.get_name()] = state

        # add the transition into the new state
        try:
            self.add_transition(accessible_from, state.get_name())
        except self.ConsistencyError:
            del self._states[state.get_name()]
            raise

    def add_transition(self, from_state: str, to_state: str):
        """
        Adds a new Transition to the State Machine Description.
        :param from_state: State being transitioned out of.
        :param to_state: State to transition into.
        :raise ConsistencyError: If `from_state` or `to_state` are not the name of a State in the Description.
        """
        if from_state not in self._states:
            raise self.ConsistencyError(f'No State named "{from_state}" in State Machine')
        if to_state not in self._states:
            raise self.ConsistencyError(f'No State named "{to_state}" in State Machine')

        # store the new transition
        if from_state in self._transitions:
            self._transitions[from_state].append(to_state)
        else:
            self._transitions[from_state] = [to_state]


class StateMachine:

    Callback = StateMachineCallback
    State = State
    Description = StateMachineDescription

    def __init__(self, description: Description):
        self._description: StateMachine.Description = description
        self._current_state: Optional[State] = None

    def initialize(self):
        if self._current_state is None:
            self._current_state = self._description._states[self._description._initial_state]
            self._current_state.enter()

    def transition_to(self, target_name: str) -> bool:
        if self._current_state is None:
            self.initialize()

        target_state: State = self._description._states.get(target_name)
        if target_state is None:
            return False  # state name is not in the description

        if target_state == self._current_state:
            return True  # we are already there

        if target_name not in self._description._transitions[self._current_state.get_name()]:
            return False  # no transition allowed

        self._current_state.exit()
        self._current_state = target_state
        self._current_state.enter()
        return True


#######################################################################################################################

from .widget import Widget
