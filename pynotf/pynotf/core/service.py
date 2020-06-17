from abc import ABC, abstractmethod
from enum import Enum, unique, auto

from pynotf.data import Value
import pynotf.core as core
from pynotf.core.logic import OperatorRowDescription, OperatorIndex, create_flags


# FACT #################################################################################################################

@unique
class FactState(Enum):
    # TODO: I don't actually see myself using the FactState anywhere ...
    # To the best of our knowledge, the Fact is current and correct.
    KNOWN = auto()

    # The Fact has never been determined.
    UNKNOWN = auto()

    # The app was unable to receive updates for a while and and the most recent known value might have changed.
    STALE = auto()

    # The Fact was updated from the app but did not get confirmation back that the change was successful.
    SPECULATIVE = auto()


# TODO: maybe move Fact back into core.logic?
class Fact:
    State = FactState

    def __init__(self, initial: Value):
        self._operator: core.Operator = core.Operator.create(
            OperatorRowDescription(
                operator_index=OperatorIndex.RELAY,
                initial_value=initial,
                input_schema=Value.Schema(),
                args=Value(),
                data=Value(),
                flags=create_flags(multicast=True, generator=True)
            ))
        self._state: FactState = FactState.UNKNOWN

    def __del__(self):
        if self._operator.is_valid():
            self._operator.remove()

    def is_valid(self) -> bool:
        return self._operator.is_valid()

    def get_value(self) -> Value:
        return self._operator.get_value()

    def get_state(self) -> FactState:
        return self._state

    def get_schema(self) -> Value.Schema:
        return self.get_value().get_schema()

    def get_operator(self) -> core.Operator:
        return self._operator

    def update(self, value: Value) -> None:
        assert value.get_schema() == self.get_schema()
        core.get_app().schedule_event(lambda: self._operator.update(value))

    def fail(self, error: Value) -> None:
        core.get_app().schedule_event(lambda: self._operator.fail(error))

    def complete(self) -> None:
        core.get_app().schedule_event(lambda: self._operator.complete())


# SERVICE ##############################################################################################################

class Service(ABC):
    """
    Manages Facts about a certain aspect of the world that the UI interacts with.
    Example for Services are:
        "os.input" -> Provides input from Human interface devices through the operating system (mouse, keyboard, etc.).
        "os.filesystem" -> Watches files or folders for changes.
        "web.rest" -> Single response from a REST web query.
        "db.sqlite" -> Visibility into sqlite queries and responses.
    """

    @abstractmethod
    def initialize(self) -> None:
        raise NotImplementedError()

    @abstractmethod
    def get_fact(self, query: str) -> core.Operator:
        """
        Either returns a valid Fact Operator or raises an exception.
        """
        raise NotImplementedError()

    @abstractmethod
    def shutdown(self) -> None:
        raise NotImplementedError()
