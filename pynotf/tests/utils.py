from typing import List, ClassVar, Optional, Tuple
import sys

from pynotf.logic import Operator, Subscriber, Publisher
from pynotf.value import Value


def count_live_subscribers(publisher: Publisher) -> int:
    return len([sub for sub in publisher._subscribers if sub() is not None])


class NumberPublisher(Publisher):
    def __init__(self):
        Publisher.__init__(self, Value(0).schema)
        self.exceptions: List[Tuple[Subscriber, Exception]] = []

    def _handle_exception(self, subscriber: 'Subscriber', exception: Exception):
        Publisher._handle_exception(self, subscriber, exception)  # for coverage
        self.exceptions.append((subscriber, exception))


class NoopSubscriber(Subscriber):
    def __init__(self, schema: Value.Schema = Value(0).schema):
        super().__init__(schema)

    def on_next(self, signal: Publisher.Signal, value: Value):
        pass

    def on_error(self, signal: Publisher.Signal, exception: Exception):
        pass

    def on_complete(self, signal: Publisher.Signal):
        pass


class ExceptionOnCompleteSubscriber(Subscriber):
    def __init__(self, schema: Value.Schema = Value(0).schema):
        super().__init__(schema)

    def on_next(self, signal: Publisher.Signal, value: Value):
        pass

    def on_error(self, signal: Publisher.Signal, exception: Exception):
        raise RuntimeError("I also want to err")

    def on_complete(self, signal: Publisher.Signal):
        raise RuntimeError("I am now also complete")


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


class AddAnotherSubscriberSubscriber(Subscriber):
    """
    Whenever one of the three methods is called, this Subscriber creates a new (Noop) Subscriber and subscibes it to
    the given Publisher.
    """

    def __init__(self, publisher: Publisher, modulus: int = sys.maxsize, schema: Value.Schema = Value(0).schema):
        """
        :param publisher: Publisher to subscribe new Subscribers to.
        :param modulus: With n = number of subscribers, every time n % modulus = 0, delete all subscribers.
        :param schema: Schema of the Subscriber.
        """
        super().__init__(schema)
        self._publisher: Publisher = publisher
        self._modulus: int = modulus
        self._subscribers: List[NoopSubscriber] = []  # to keep them alive

    def _add_another(self):
        if (len(self._subscribers) + 1) % self._modulus == 0:
            self._subscribers.clear()
        else:
            subscriber: NoopSubscriber = NoopSubscriber(self.input_schema)
            self._subscribers.append(subscriber)
            subscriber.subscribe_to(self._publisher)

    def on_next(self, signal: Publisher.Signal, value: Value):
        self._add_another()

    def on_error(self, signal: Publisher.Signal, exception: Exception):
        self._add_another()

    def on_complete(self, signal: Publisher.Signal):
        self._add_another()


class UnsubscribeSubscriber(Subscriber):
    """
    A Subscriber that unsubscribes whenever it gets a value.
    """

    def __init__(self, publisher: Publisher, schema: Value.Schema = Value(0).schema):
        """
        :param publisher: Publisher to subscribe to.
        :param schema: Schema of the Subscriber.
        """
        super().__init__(schema)
        self._publisher: Publisher = publisher
        self.subscribe_to(self._publisher)

    def on_next(self, signal: Publisher.Signal, value: Value):
        self.unsubscribe_from(self._publisher)


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
