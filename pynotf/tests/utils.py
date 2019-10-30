from typing import List, ClassVar, Optional, Tuple

from pynotf.logic import Operator, Subscriber, Publisher
from pynotf.value import Value


class NumberPublisher(Publisher):
    def __init__(self):
        Publisher.__init__(self, Value(0).schema)
        self.exceptions: List[Tuple[Subscriber, Exception]] = []

    def _handle_exception(self, subscriber: 'Subscriber', exception: Exception):
        Publisher._handle_exception(self, subscriber, exception)  # for coverage
        self.exceptions.append((subscriber, exception))


class Recorder(Subscriber):
    def __init__(self, schema: Value.Schema):
        super().__init__(schema)
        self.values: List[Value] = []
        self.signals: List[Publisher.Signal] = []
        self.completed: List[int] = []
        self.errors: List[Exception] = []

    def on_next(self, signal: Publisher.Signal, value: Value):
        self.values.append(value)
        self.signals.append(signal)

    def on_error(self, signal: Publisher.Signal, exception: Exception):
        Subscriber.on_error(self, signal, exception)  # just for coverage
        self.errors.append(exception)

    def on_complete(self, signal: Publisher.Signal):
        Subscriber.on_complete(self, signal)  # just for coverage
        self.completed.append(signal.source)


def record(publisher: [Publisher, Operator]) -> Recorder:
    recorder = Recorder(publisher.output_schema)
    recorder.subscribe_to(publisher)
    return recorder


class AddConstantOperation(Operator.Operation):
    _schema: ClassVar[Value.Schema] = Value(0).schema

    def __init__(self, addition: float):
        self._constant = addition

    @property
    def input_schema(self) -> Value.Schema:
        return self._schema

    @property
    def output_schema(self) -> Value.Schema:
        return self._schema

    def _perform(self, value: Value) -> Value:
        return value.modified().set(value.as_number() + self._constant)


class GroupTwoOperation(Operator.Operation):
    _input_schema: ClassVar[Value.Schema] = Value(0).schema
    _output_prototype: ClassVar[Value] = Value({"x": 0, "y": 0})

    def __init__(self):
        self._last_value: Optional[float] = None

    @property
    def input_schema(self) -> Value.Schema:
        return self._input_schema

    @property
    def output_schema(self) -> Value.Schema:
        return self._output_prototype.schema

    def _perform(self, value: Value) -> Optional[Value]:
        if self._last_value is None:
            self._last_value = value.as_number()
        else:
            result = self._output_prototype.modified().set({"x": self._last_value, "y": value.as_number()})
            self._last_value = None
            return result


class ErrorOperation(Operator.Operation):
    """
    An Operation that raises a ValueError if a certain number is passed.
    """
    _schema: ClassVar[Value.Schema] = Value(0).schema

    def __init__(self, err_on_number: float):
        self._err_on_number: float = err_on_number

    @property
    def input_schema(self) -> Value.Schema:
        return self._schema

    @property
    def output_schema(self) -> Value.Schema:
        return self._schema

    def _perform(self, value: Value) -> Optional[Value]:
        if value.as_number() == self._err_on_number:
            raise ValueError("The error condition has occurred")
        return value


class ClampOperation(Operator.Operation):
    """
    Clamps a numeric Value to a certain range.
    """
    _schema: ClassVar[Value.Schema] = Value(0).schema

    def __init__(self, min_value: float, max_value: float):
        self._min: float = min_value
        self._max: float = max_value

    @property
    def input_schema(self) -> Value.Schema:
        return self._schema

    @property
    def output_schema(self) -> Value.Schema:
        return self._schema

    def _perform(self, value: Value) -> Value:
        return value.modified().set(max(self._min, min(self._max, value.as_number())))


class StringifyOperation(Operator.Operation):
    """
    Converts a numeric Value into a string representation.
    """

    @property
    def input_schema(self) -> Value.Schema:
        return Value(0).schema

    @property
    def output_schema(self) -> Value.Schema:
        return Value("").schema

    def _perform(self, value: Value) -> Value:
        return Value(str(value.as_number()))
