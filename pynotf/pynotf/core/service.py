from typing import Dict
from enum import Enum, unique, auto

from pynotf.data import Value
import pynotf.core as core


# FACT #################################################################################################################

@unique
class FactState(Enum):
    # To the best of our knowledge, the Fact is current and correct.
    KNOWN = auto()

    # The Fact has never been determined.
    UNKNOWN = auto()

    # The app was unable to receive updates for a while and and the most recent known value might have changed.
    STALE = auto()

    # The Fact was updated from the app but did not get confirmation back that the change was successful.
    SPECULATIVE = auto()


class Fact:
    State = FactState

    def __init__(self, operator: core.Operator):
        self._operator: core.Operator = operator
        self._state: FactState = FactState.UNKNOWN

    def get_value(self) -> Value:
        return self._operator.get_value()

    def get_state(self) -> FactState:
        return self._state

    def get_schema(self) -> Value.Schema:
        return self.get_value().get_schema()

    def update(self, value: Value) -> None:
        assert value.get_schema() == self.get_schema()
        core.get_app().schedule_event(lambda: self._operator.update(value))

    def fail(self, error: Value) -> None:
        core.get_app().schedule_event(lambda: self._operator.fail(error))

    def complete(self) -> None:
        core.get_app().schedule_event(lambda: self._operator.complete())

    def subscribe(self, downstream: core.Operator):
        self._operator.subscribe(downstream)


# SERVICE ##############################################################################################################

class Service:
    """
    Provides a service.
    """
    pass


# WORLD ################################################################################################################

class World:
    """
    The World is the outside that the UI interacts with.
    What is visible / accessible in the World is determined by what Services are available to the application.
    """

    def __init__(self):
        self._services: Dict[str, Service] = {}
