from typing import Optional, Dict, List, NamedTuple, Set

from pynotf.callback import Callback as CallbackBase


#######################################################################################################################

class StateMachineCallback(CallbackBase):
    """
    Callbacks called when a State is entered/exited.
    """

    def __init__(self, source: str = ''):
        """
        Constructor.
        :param source: Callback source.
        """
        CallbackBase.__init__(self,
                              signature=dict(widget=Widget.Handle),
                              source=source,
                              environment=dict(WidgetHandle=Widget.Handle))


########################################################################################################################

class State(NamedTuple):
    """
    A State consists of nothing more than two Callbacks: a mandatory one that is called when the State is entered, and
    an optional second one that is called when the State is exited.
    """
    enter: StateMachineCallback
    exit: Optional[StateMachineCallback] = None


########################################################################################################################

class StateMachineDescription(NamedTuple):
    # all states by name (>= 1 entries)
    states: Dict[str, State]

    # name of the initial state, must be present in `states`
    initial_state: str

    # reachable states by source state (entries <= len(states))
    transitions: Dict[str, List[str]] = {}


########################################################################################################################

class StateMachine:
    Callback = StateMachineCallback
    State = State
    Description = StateMachineDescription

    class ConsistencyError(Exception):
        pass

    def __init__(self, description: Description):
        if len(description.states) == 0:
            raise self.ConsistencyError("Cannot construct a StateMachine with zero States")
        if description.initial_state not in description.states:
            raise self.ConsistencyError(f'Initial State "{description.initial_state}" not found in the Description')

        unreachable_states: Set[str] = set(description.states.keys())
        for transition_from, reachable_states in description.transitions.items():
            if not transition_from in description.states:
                raise self.ConsistencyError(f'Transition start State "{transition_from}" not found in the Description')
            for transition_to in reachable_states:
                if not transition_to in description.states:
                    raise self.ConsistencyError(f'Transition end State "{transition_to}" not found in the Description')
                unreachable_states.discard(transition_to)

        unreachable_states.discard(description.initial_state)
        if len(unreachable_states) > 0:
            raise self.ConsistencyError('Unreachable State{} found in Description: "{}"'.format(
                "s" if len(unreachable_states) > 1 else "",
                '", "'.join(unreachable_states)
            ))

        self._states: Dict[str, State] = description.states  # is constant
        self._initial_state: str = description.initial_state  # is constant
        self._transitions: Dict[str, List[str]] = description.transitions  # is constant
        self._current_state: Optional[State] = None

    def initialize(self):
        if self._current_state is None:
            self._current_state = self._states[self._initial_state]
            self._current_state.enter()

    def transition_to(self, target_name: str) -> bool:
        if self._current_state is None:
            self.initialize()

        target_state: State = self._states.get(target_name)
        if target_state is None:
            return False  # state name is not in the description

        if target_state == self._current_state:
            return True  # we are already there

        if target_name not in self._transitions[self._current_state.get_name()]:
            return False  # no transition allowed

        if self._current_state.exit is not None:
            self._current_state.exit()
        self._current_state = target_state
        self._current_state.enter()
        return True


#######################################################################################################################

from .widget import Widget
