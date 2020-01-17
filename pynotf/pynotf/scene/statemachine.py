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
        """
        Raised when a State Machine Description failed to validate in the State Machine constructor.
        """
        pass

    class TransitionError(Exception):
        """
        Raised when a Transition it not allowed.
        """
        pass

    def __init__(self, description: Description):
        """
        Constructor.
        Validates the State Machine Description and initializes a State Machine instance from it.
        :param description: State Machine Description to initialize from.
        :raise ConsistencyError: If the Description fails to validate.
        """
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
        self._widget: Optional[Widget.Handle] = None
        self._current_state_name: str = ''
        self._current_state: Optional[State] = None

    def initialize(self, widget: 'Widget'):
        """
        Enter the initial State.
        Is not part of the constructor because the Widget needs to be fully set up before any StateMachineCallback
        can safely be called.
        :param widget: Widget to manage.
        """
        if self._current_state is None:
            self._widget = Widget.Handle(widget)
            self._current_state_name = self._initial_state
            self._current_state = self._states[self._initial_state]
            self._current_state.enter(self._widget)

    def transition_into(self, target_name: str):
        """
        Transitions from the current into another State with the given name.
        Re-transitioning into the current State has to be allowed explicitly in the State Machine Description, otherwise
        it is considered a TransitionError.
        :param target_name: Name of the State to transition into.
        :raise NameError: If the State Machine does not have a State with the given name.
        :raise TransitionError: If you cannot enter the target State from the current one.
        """
        assert self._current_state is not None

        # does the target name identify a state in this state machine?
        target_state: State = self._states.get(target_name)
        if target_state is None:
            raise NameError(f'No State named "{target_name}" in State Machine')

        # can we transition from this state into the target state?
        if target_name not in self._transitions[self._current_state_name]:
            if target_name == self._current_state_name:
                raise self.TransitionError(f'Re-transition into State "{self._current_state_name}" '
                                           f'has not been allowed in the State Machine Description.')
            else:
                raise self.TransitionError(f'Transition from current State "{self._current_state_name}" to '
                                           f'"{target_name}" has not been allowed in the State Machine Description.')

        # perform the transition
        if self._current_state.exit is not None:
            self._current_state.exit(self._widget)
        self._current_state_name = target_name
        self._current_state = target_state
        self._current_state.enter(self._widget)


#######################################################################################################################

from .widget import Widget
